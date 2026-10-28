
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <cmath>

using namespace ns3;

void RunExperiment(double speed)
{
    std::cout << "\nSpeed: " << speed << " m/s\n";

    // 1. Create six nodes
    NodeContainer nodes;
    nodes.Create(6);

    // 2. Configure Wi-Fi ad hoc network
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    YansWifiChannelHelper channel =
        YansWifiChannelHelper::Default();

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices =
        wifi.Install(phy, mac, nodes);

    // 3. Mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel(
        "ns3::ConstantVelocityMobilityModel");

    mobility.Install(nodes);

    nodes.Get(0)->GetObject<MobilityModel>()
        ->SetPosition(Vector(0, 0, 0));
    nodes.Get(1)->GetObject<MobilityModel>()
        ->SetPosition(Vector(20, 0, 0));

    nodes.Get(2)->GetObject<MobilityModel>()
        ->SetPosition(Vector(0, 20, 0));
    nodes.Get(3)->GetObject<MobilityModel>()
        ->SetPosition(Vector(10, 20, 0));

    nodes.Get(4)->GetObject<MobilityModel>()
        ->SetPosition(Vector(0, 40, 0));
    nodes.Get(5)->GetObject<MobilityModel>()
        ->SetPosition(Vector(10, 40, 0));

    nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()
        ->SetVelocity(Vector(speed, 0, 0));
    nodes.Get(3)->GetObject<ConstantVelocityMobilityModel>()
        ->SetVelocity(Vector(speed, 0, 0));
    nodes.Get(5)->GetObject<ConstantVelocityMobilityModel>()
        ->SetVelocity(Vector(speed, 0, 0));

    // 4. Internet stack and IP addresses
    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ip;

    ip.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiIp =
        ip.Assign(NetDeviceContainer(
            devices.Get(0), devices.Get(1)));

    ip.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer btIp =
        ip.Assign(NetDeviceContainer(
            devices.Get(2), devices.Get(3)));

    ip.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer zigbeeIp =
        ip.Assign(NetDeviceContainer(
            devices.Get(4), devices.Get(5)));

    // 5. Wi-Fi-like UDP flow
    PacketSinkHelper sink1(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 5000));

    ApplicationContainer s1 = sink1.Install(nodes.Get(1));
    s1.Start(Seconds(0.0));
    s1.Stop(Seconds(10.0));

    OnOffHelper app1(
        "ns3::UdpSocketFactory",
        InetSocketAddress(wifiIp.GetAddress(1), 5000));

    app1.SetConstantRate(DataRate("1Mbps"), 512);
    ApplicationContainer a1 = app1.Install(nodes.Get(0));
    a1.Start(Seconds(1.0));
    a1.Stop(Seconds(10.0));

    // 6. Bluetooth-like UDP flow
    PacketSinkHelper sink2(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 6000));

    ApplicationContainer s2 = sink2.Install(nodes.Get(3));
    s2.Start(Seconds(0.0));
    s2.Stop(Seconds(10.0));

    OnOffHelper app2(
        "ns3::UdpSocketFactory",
        InetSocketAddress(btIp.GetAddress(1), 6000));

    app2.SetConstantRate(DataRate("500Kbps"), 256);
    ApplicationContainer a2 = app2.Install(nodes.Get(2));
    a2.Start(Seconds(1.0));
    a2.Stop(Seconds(10.0));

    // 7. Zigbee-like UDP flow
    PacketSinkHelper sink3(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 7000));

    ApplicationContainer s3 = sink3.Install(nodes.Get(5));
    s3.Start(Seconds(0.0));
    s3.Stop(Seconds(10.0));

    OnOffHelper app3(
        "ns3::UdpSocketFactory",
        InetSocketAddress(zigbeeIp.GetAddress(1), 7000));

    app3.SetConstantRate(DataRate("100Kbps"), 100);
    ApplicationContainer a3 = app3.Install(nodes.Get(4));
    a3.Start(Seconds(1.0));
    a3.Stop(Seconds(10.0));

    // 8. FlowMonitor
    FlowMonitorHelper fm;
    Ptr<FlowMonitor> monitor = fm.InstallAll();

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    monitor->CheckForLostPackets();

    double tx = 0, rx = 0, delay = 0, rxBytes = 0;

    std::map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    for (const auto &flow : stats)
    {
        tx += flow.second.txPackets;
        rx += flow.second.rxPackets;
        rxBytes += flow.second.rxBytes;
        delay += flow.second.delaySum.GetSeconds();
    }

    double loss = 0;
    if (tx > 0)
        loss = (tx - rx) * 100.0 / tx;

    double throughput = rxBytes * 8.0 / 9.0 / 1000000.0;

    double avgDelay = 0;
    if (rx > 0)
        avgDelay = delay * 1000.0 / rx;

    // Simple estimated received power
    double distance = 20 + speed * 5;
    double power = 20 - 20 * log10(distance);

    std::cout << "Estimated Received Power: "
              << power << " dBm\n";
    std::cout << "Throughput: "
              << throughput << " Mbps\n";
    std::cout << "Packet Loss: "
              << loss << " %\n";
    std::cout << "Average Delay: "
              << avgDelay << " ms\n";

    Simulator::Destroy();
}

int main()
{
    RunExperiment(1);
    RunExperiment(5);
    RunExperiment(10);

    return 0;
}
