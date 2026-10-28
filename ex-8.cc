
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"

#include <iostream>
using namespace ns3;

int main()
{
    // 1. Create nodes
    NodeContainer nodes;
    nodes.Create(4);

    NodeContainer routers;
    routers.Create(2);

    Ptr<Node> n0 = nodes.Get(0);
    Ptr<Node> n1 = nodes.Get(1);
    Ptr<Node> n2 = nodes.Get(2);
    Ptr<Node> n3 = nodes.Get(3);
    Ptr<Node> r0 = routers.Get(0);
    Ptr<Node> r1 = routers.Get(1);

    // 2. Access links
    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));
    access.SetQueue("ns3::DropTailQueue<Packet>",
                    "MaxSize", StringValue("50p"));

    NetDeviceContainer d0r0 = access.Install(n0, r0);
    NetDeviceContainer d1r0 = access.Install(n1, r0);
    NetDeviceContainer dr1n2 = access.Install(r1, n2);
    NetDeviceContainer dr1n3 = access.Install(r1, n3);

    // 3. Bottleneck link
    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));
    bottleneck.SetQueue("ns3::DropTailQueue<Packet>",
                        "MaxSize", StringValue("50p"));

    NetDeviceContainer dr0r1 = bottleneck.Install(r0, r1);

    // Remove default queue-disc on bottleneck
    TrafficControlHelper tch;
    tch.Uninstall(dr0r1);

    // 4. Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);
    internet.Install(routers);

    // 5. Assign IP addresses
    Ipv4AddressHelper ip;

    ip.SetBase("10.0.1.0", "255.255.255.0");
    ip.Assign(d0r0);

    ip.SetBase("10.0.2.0", "255.255.255.0");
    ip.Assign(d1r0);

    ip.SetBase("10.0.3.0", "255.255.255.0");
    ip.Assign(dr0r1);

    ip.SetBase("10.0.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ir1n2 = ip.Assign(dr1n2);

    ip.SetBase("10.0.5.0", "255.255.255.0");
    Ipv4InterfaceContainer ir1n3 = ip.Assign(dr1n3);

    // 6. Routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 7. TCP: Node 0 -> Node 3
    uint16_t tcpPort = 5000;

    PacketSinkHelper tcpSink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), tcpPort));

    ApplicationContainer ts = tcpSink.Install(n3);
    ts.Start(Seconds(0.0));
    ts.Stop(Seconds(20.0));

    BulkSendHelper tcpSource(
        "ns3::TcpSocketFactory",
        InetSocketAddress(ir1n3.GetAddress(1), tcpPort));

    tcpSource.SetAttribute("SendSize", UintegerValue(1024));
    tcpSource.SetAttribute("MaxBytes", UintegerValue(0));

    ApplicationContainer tc = tcpSource.Install(n0);
    tc.Start(Seconds(1.0));
    tc.Stop(Seconds(20.0));

    // 8. UDP: Node 1 -> Node 2
    uint16_t udpPort = 6000;

    PacketSinkHelper udpSink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), udpPort));

    ApplicationContainer us = udpSink.Install(n2);
    us.Start(Seconds(0.0));
    us.Stop(Seconds(20.0));

    OnOffHelper udpSource(
        "ns3::UdpSocketFactory",
        InetSocketAddress(ir1n2.GetAddress(1), udpPort));

    udpSource.SetAttribute("DataRate", StringValue("1Mbps"));
    udpSource.SetAttribute("PacketSize", UintegerValue(512));
    udpSource.SetAttribute("OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpSource.SetAttribute("OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer uc = udpSource.Install(n1);
    uc.Start(Seconds(1.0));
    uc.Stop(Seconds(20.0));

    // 9. Tracing
    access.EnablePcapAll("exp8-access");
    bottleneck.EnablePcapAll("exp8-bottleneck");

    // 10. FlowMonitor
    FlowMonitorHelper fm;
    Ptr<FlowMonitor> monitor = fm.InstallAll();

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(fm.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    uint64_t tcpTx = 0, tcpRx = 0;
    uint64_t udpTx = 0, udpRx = 0;

    for (const auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t =
            classifier->FindFlow(flow.first);

        const auto &s = flow.second;

        double duration =
            (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();

        double throughput = duration > 0
            ? s.rxBytes * 8.0 / duration / 1000000.0 : 0;

        double delay = s.rxPackets > 0
            ? s.delaySum.GetSeconds() / s.rxPackets * 1000.0 : 0;

        double loss = s.txPackets > 0
            ? 100.0 * (s.txPackets - s.rxPackets) / s.txPackets : 0;

        double pdr = s.txPackets > 0
            ? 100.0 * s.rxPackets / s.txPackets : 0;

        if (t.destinationPort == tcpPort)
        {
            tcpTx += s.txPackets;
            tcpRx += s.rxPackets;
            std::cout << "\nTCP Flow\n";
        }
        else if (t.destinationPort == udpPort)
        {
            udpTx += s.txPackets;
            udpRx += s.rxPackets;
            std::cout << "\nUDP Flow\n";
        }
        else
            continue;

        std::cout << "Throughput: " << throughput << " Mbps\n";
        std::cout << "Average Delay: " << delay << " ms\n";
        std::cout << "Packet Loss: " << loss << " %\n";
        std::cout << "PDR: " << pdr << " %\n";
    }

    std::cout << "\nEstimated TCP retransmissions: "
              << (tcpTx > tcpRx ? tcpTx - tcpRx : 0) << "\n";

    Ptr<PacketSink> tcpPtr = DynamicCast<PacketSink>(ts.Get(0));
    Ptr<PacketSink> udpPtr = DynamicCast<PacketSink>(us.Get(0));

    std::cout << "TCP bytes received: "
              << tcpPtr->GetTotalRx() << "\n";
    std::cout << "UDP bytes received: "
              << udpPtr->GetTotalRx() << "\n";

    Simulator::Destroy();
    return 0;
}
