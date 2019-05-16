/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "build/ns3/command-line.h"
#include "ns3/drop-tail-queue.h"
using namespace ns3;

//NS_LOG_COMPONENT_DEFINE ("SixthScriptExample");
NS_LOG_COMPONENT_DEFINE ("Lab-4");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                1 Mbps, 10 ms
//
//
// We want to look at changes in the ns-3 TCP congestion window.  We need
// to crank up a flow and hook the CongestionWindow attribute on the socket
// of the sender.  Normally one would use an on-off application to generate a
// flow, but this has a couple of problems.  First, the socket of the on-off
// application is not created until Application Start time, so we wouldn't be
// able to hook the socket (now) at configuration time.  Second, even if we
// could arrange a call after start time, the socket is not public so we
// couldn't get at it.
//
// So, we can cook up a simple version of the on-off application that does what
// we want.  On the plus side we don't need all of the complexity of the on-off
// application.  On the minus side, we don't have a helper, so we have to get
// a little more involved in the details, but this is trivial.
//
// So first, we create a socket and do the trace connect on it; then we pass
// this socket into the constructor of our simple application which we then
// install in the source node.
// ===========================================================================
//
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

Ptr<PacketSink> cbrSinks[5],tcpSink;
uint32_t total_drops=0;
uint32_t totalVal=0;
uint32_t totalValue=1;

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

static void
RxDrop (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
 *stream->GetStream ()<<Simulator::Now ().GetSeconds ()<<" "<<++total_drops<<std::endl;
}

static void
TotalRx(Ptr<OutputStreamWrapper> stream)
{
    totalVal = tcpSink->GetTotalRx();

    for(int i=0; i<5; i++)
    {
        totalVal += cbrSinks[i]->GetTotalRx();
    }

    *stream->GetStream()<<Simulator::Now ().GetSeconds ()<<" " <<totalVal<<std::endl;

    Simulator::Schedule(Seconds(0.0001),&TotalRx, stream);
}

int
main (int argc, char *argv[])
{
  total_drops=0;
  bool flowmonitor=false;

  //creating two nodes
  NodeContainer nodes;
  nodes.Create (2);  

  //creating point to point link as per given attributes
  PointToPointHelper pointToPoint;      
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
  pointToPoint.SetQueue("ns3::DropTailQueue","MaxSize",StringValue("19p"));

  NetDeviceContainer devices;
  
  //installing the p2p link on nodes
  devices = pointToPoint.Install (nodes);       
 
  //Type of TCP-protocol
  std::string prot = "TcpWestwood";

  //using command-line options to set protocol
  CommandLine cmd;                                              
  cmd.AddValue ("prot", "Transport protocol to use: TcpNewReno, "
                "TcpHybla,TcpVegas, TcpScalable,"
                "TcpWestwood", prot);

  cmd.Parse (argc, argv);

  //configuring the Simulator to the corresponding Tcp Protocol
  if (prot.compare ("TcpNewReno") == 0)
  {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
  }
  else if (prot.compare ("TcpHybla") == 0)
  {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHybla::GetTypeId ()));
  }
  else if (prot.compare ("TcpVegas") == 0)
  {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
  }
  else if (prot.compare ("TcpScalable") == 0)
  {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpScalable::GetTypeId ()));
  }
  else if (prot.compare ("TcpWestwood") == 0)
  {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
  }
  else
  {
      NS_LOG_DEBUG ("Invalid TCP version");
      exit (1);
  }
  //Setting up Error Rate Model
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00005));
  devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  //installing the tcp/ip protocol stack on the nodes
  InternetStackHelper stack;
  stack.Install (nodes);

  //Assigning Ip Addresses
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  //Creating Tcp Sink on second node
  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0));
  sinkApps.Stop (Seconds (1.8));

  tcpSink = DynamicCast<PacketSink> (sinkApps.Get (0));
  
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 512, 1000, DataRate ("1Mbps"));
  //Creating FTP Source on First Node
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (0));
  app->SetStopTime (Seconds (1.8));

  
  double startTimes[5] = {0.2, 0.4, 0.6, 0.8, 1.0};
  double endTimes[5]   = {1.8, 1.8, 1.2, 1.4, 1.6};
  //Setting up the 5 Constant Bit Rate(CBR) Traffic Agents
  uint16_t cbrPort = 12345;

  for(int i=0; i<5; i++)
  {
    ApplicationContainer cbrApps;
    ApplicationContainer cbrSinkApps;
    //Setting attributes to the CBR Traffic
    OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (interfaces.GetAddress (1), cbrPort+i));
    onOffHelper.SetAttribute ("PacketSize", UintegerValue (1024));
    onOffHelper.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onOffHelper.SetAttribute ("DataRate", StringValue ("300Kbps"));
    onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (startTimes[i])));
    onOffHelper.SetAttribute ("StopTime", TimeValue (Seconds (endTimes[i])));
    //Installing CBR Source on first node
    cbrApps.Add (onOffHelper.Install (nodes.Get (0)));

    PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), cbrPort+i));
    //Installing CBR Sink on the second node
    cbrSinkApps = sink.Install (nodes.Get (1));
    cbrSinkApps.Start (Seconds (0.0));
    cbrSinkApps.Stop (Seconds (1.8));
    cbrSinks[i] = DynamicCast<PacketSink> (cbrSinkApps.Get (0));
  }
  //output file names
  std::string code_dropped= prot+"_dropped";
  std::string code_cwnd= prot+"_cwnd";
  std::string code_total=prot+"_total";

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (code_dropped);
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (code_cwnd);
  Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream (code_total);

  //For recording packet drop into a file
  devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, stream1));

  //For recording cwnd size changes into a file
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream2));

  //For recording Total bytes into a file
  Simulator::Schedule(Seconds(0.00001),&TotalRx, stream3);

  //set flowmoniter bit to true if flow monitor needed
  if(flowmonitor){
    FlowMonitorHelper flowHelper;
    flowHelper.InstallAll();
    flowHelper.SerializeToXmlFile (prot + ".flowmonitor", true, true);
  }

  Simulator::Stop (Seconds (1.8));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

