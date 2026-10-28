#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"

#include <iostream>
#include <vector>

using namespace ns3;

// Topology:
// Vehicle0..4 -- RoadsideRouter(r1) -- Internet(r2) -- TrafficServer
void RunSim(std::string variant)
{
    std::cout << "\n=========== " << variant << " ===========\n";

    // ---- TCP settings (before any sockets/apps) ----
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(variant));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1024));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 20));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 20));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(true)); // needed for BBR

    // ---- Nodes ----
    NodeContainer vehicles;
    vehicles.Create(5);
    NodeContainer routers;
    routers.Create(2); // r1 = roadside router (0), r2 = internet router (1)
    NodeContainer server;
    server.Create(1);

    // ---- Links ----
    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));
    bottleneck.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("50p"));

    PointToPointHelper egress;
    egress.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    egress.SetChannelAttribute("Delay", StringValue("2ms"));

    std::vector<NetDeviceContainer> accDev(5);
    for (uint32_t i = 0; i < 5; i++)
    {
        accDev[i] = access.Install(vehicles.Get(i), routers.Get(0));
    }
    NetDeviceContainer devBtl = bottleneck.Install(routers.Get(0), routers.Get(1));
    NetDeviceContainer devSrv = egress.Install(routers.Get(1), server.Get(0));

    // ---- Remove default queue-disc so the 50p DropTail queue actually drops ----
    TrafficControlHelper tch;
    tch.Uninstall(devBtl);

    // ---- Internet stack ----
    InternetStackHelper internet;
    internet.Install(vehicles);
    internet.Install(routers);
    internet.Install(server);

    // ---- IP addressing (one subnet per link) ----
    Ipv4AddressHelper ipv4;
    for (uint32_t i = 0; i < 5; i++)
    {
        std::ostringstream subnet;
        subnet << "10.1." << (i + 1) << ".0";
        ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
        ipv4.Assign(accDev[i]);
    }
    ipv4.SetBase("10.2.1.0", "255.255.255.0");
    ipv4.Assign(devBtl);
    ipv4.SetBase("10.3.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceSrv = ipv4.Assign(devSrv);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // ---- Applications ----
    // TCP sink on the traffic control server, port 8080
    PacketSinkHelper tcpSink("ns3::TcpSocketFactory",
                             InetSocketAddress(Ipv4Address::GetAny(), 8080));
    ApplicationContainer sinkApp = tcpSink.Install(server.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(20.0));

    // One greedy BulkSend flow per vehicle -> server (index 1 = server side)
    for (uint32_t i = 0; i < 5; i++)
    {
        BulkSendHelper tcpSource("ns3::TcpSocketFactory",
                                 InetSocketAddress(ifaceSrv.GetAddress(1), 8080));
        tcpSource.SetAttribute("SendSize", UintegerValue(1024));
        tcpSource.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer app = tcpSource.Install(vehicles.Get(i));
        app.Start(Seconds(1.0));
        app.Stop(Seconds(20.0));
    }

    // ---- FlowMonitor ----
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = mon->GetFlowStats();

    double sumX = 0, sumX2 = 0, totalMbps = 0, totalDelay = 0;
    double totalTx = 0, totalRx = 0;
    uint32_t n = 0;

    for (const auto& kv : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        const auto& s = kv.second;

        // ignore reverse ACK flows
        if (t.destinationPort != 8080)
        {
            continue;
        }

        double dur = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double mbps = (dur > 0) ? (s.rxBytes * 8.0 / dur / 1e6) : 0;
        double delay = (s.rxPackets > 0) ? (s.delaySum.GetSeconds() / s.rxPackets * 1000.0) : 0;
        double loss = (s.txPackets > 0) ? (100.0 * (s.txPackets - s.rxPackets) / s.txPackets) : 0;
        double pdr = (s.txPackets > 0) ? (100.0 * s.rxPackets / s.txPackets) : 0;

        std::cout << "Flow " << kv.first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << " Port " << t.destinationPort << "):\n"
                  << "  Throughput: " << mbps << " Mbps\n"
                  << "  Avg Delay:  " << delay << " ms\n"
                  << "  Loss:       " << loss << " %\n"
                  << "  PDR:        " << pdr << " %\n";

        sumX += mbps;
        sumX2 += mbps * mbps;
        totalMbps += mbps;
        totalDelay += delay;
        totalTx += s.txPackets;
        totalRx += s.rxPackets;
        n++;
    }

    double jain = (n > 0 && sumX2 > 0) ? (sumX * sumX) / (n * sumX2) : 0;
    std::cout << "--- " << variant << " SUMMARY ---\n"
              << "  Aggregate Throughput: " << totalMbps << " Mbps\n"
              << "  Mean Delay:           " << (n ? totalDelay / n : 0) << " ms\n"
              << "  Overall Loss:         " << (totalTx > 0 ? 100.0 * (totalTx - totalRx) / totalTx : 0) << " %\n"
              << "  Jain's Fairness:      " << jain << "\n";

    mon->SerializeToXmlFile("flow-" + variant.substr(5) + ".xml", true, true);

    Simulator::Destroy();
}

int main(int argc, char* argv[])
{
    RunSim("ns3::TcpNewReno");
    RunSim("ns3::TcpCubic");
    RunSim("ns3::TcpBbr");
    return 0;
}