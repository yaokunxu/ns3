/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Connecticut
 *
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
 *
 * Author: Robert Martin <robert.martin@engr.uconn.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include <fstream>
#include <iostream>


/*
 * BroadCastMAC
 *
 * String topology:
 * N ---->  N  -----> N -----> N* -----> S
 *
 */

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("ASBroadcastMac");

int
main (int argc, char *argv[])
{
//  double simStop = 120; //seconds  20
//  double simStop = 158; //"todo"end time
	double simStop = 3000; //"todo"end time
  int nodes = 20;
  int sinks = 1;
  uint32_t m_dataRate = 128;
  uint32_t m_packetSize = 40;
  //double range = 20;

  LogComponentEnable ("ASBroadcastMac", LOG_LEVEL_INFO);

  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", nodes);
  cmd.AddValue ("sinks", "Amount of underwater sinks", sinks);
  cmd.Parse(argc,argv);

  std::cout << "-----------Initializing simulation-----------\n";

  NodeContainer nodesCon;
  NodeContainer sinksCon;
  nodesCon.Create(nodes);
  sinksCon.Create(sinks);

  //PacketSocketHelper socketHelper;
  AquaSimSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetMac("ns3::AquaSimAloha");
  //asHelper.SetMac("ns3::AquaSimAloha");
  //asHelper.SetRouting("ns3::AquaSimRoutingDummy");
  //asHelper.SetRouting("ns3::AquaSimStaticRouting");
  asHelper.SetRouting("ns3::AquaSimDynamicRouting","N",IntegerValue(nodes+sinks));




  /*
   * Set up mobility model for nodes and sinks
   */
  MobilityHelper mobility;
  MobilityHelper mobility1;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> position1 = CreateObject<ListPositionAllocator> ();
  Vector boundry = Vector(0,0,0);

  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();

      position->Add(boundry);

      devices.Add(asHelper.Create(*i, newDevice));


//      NS_LOG_DEBUG("Node:" << newDevice->GetAddress() << " position(x):" << boundry.x);
      NS_LOG_DEBUG("Node:" << newDevice->GetAddress() << " position(x):" << boundry.x << " position(y):" << boundry.y);
     boundry.x += 150;


      newDevice->GetPhy()->SetTransRange(160);//RANGE
    }

  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position1->Add(boundry);
      devices.Add(asHelper.Create(*i, newDevice));

//      NS_LOG_DEBUG("Sink:" << newDevice->GetAddress() << " position(x):" << boundry.x);
      NS_LOG_DEBUG("Sink:" << newDevice->GetAddress() << " position(x):" << boundry.x);
      boundry.x += 140;



      newDevice->GetPhy()->SetTransRange(160);
    }


  mobility.SetPositionAllocator(position);
//  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

   // std::cout<<"mobility node position: x: "<< mobility;
    //std::cout<<"source position:x:"<<mobility.GetPosition().x << " y:"<<sModel->GetPosition().y<<" z:"<<sModel->GetPosition().z<< std::endl;
    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("1s"),
							//"Direction",StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=6.283184]"),
                             //"Speed", StringValue ("ns3::UniformRandomVariable[Min=0.5|Max=1]"),
							 "Direction",StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=0]"),
  						   "Speed", StringValue ("ns3::UniformRandomVariable[Min=5|Max=5]"),
							// "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             //TODO
  						   "Bounds", StringValue ("0|3000000|0|0"));
    	  	  	  	  	  	  	//"Bounds", StringValue ("0|2000|0|2000"));
   // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesCon);
  mobility1.SetPositionAllocator(position1);
  mobility1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility1.Install(sinksCon);




  //PacketSocketAddress socket;
  AquaSimSocketAddress socket;
  socket.SetAllDevices();
  socket.SetDestinationAddress (devices.Get(nodes)->GetAddress()); //Set dest to first sink (nodes+1 device)
  socket.SetProtocol (0);

  //OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
  OnOffNdHelper app ("ns3::AquaSimSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
//  app.SetAttribute("PacketType", UintegerValue(14));

  ApplicationContainer apps = app.Install (nodesCon);
  apps.Add(app.Install (sinksCon));
//  apps.Start (Seconds (100.5));
//  apps.Start (Seconds (150.5));
  /*"todo"time of send packet. if time =100.5 ,node 1 did not complete the iteration,
                                *so routing table of node 1 has  no path to node 4.*/

	  apps.Start (Seconds (150.5));

  apps.Stop (Seconds (simStop + 1));




   //app1
//  OnOffNdHelper app1 ("ns3::AquaSimSocketFactory", Address (socket));
//  app1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//  app1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//  app1.SetAttribute ("DataRate", DataRateValue (m_dataRate));
//  app1.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
//
//
//  ApplicationContainer apps1 = app1.Install (nodesCon);
//  apps1.Add(app1.Install (sinksCon));
//  apps1.Start (Seconds (5.5));
//  apps1.Stop (Seconds (simStop + 1));
//
//   //app2
//  OnOffNdHelper app2 ("ns3::AquaSimSocketFactory", Address (socket));
//    app2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//    app2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//    app2.SetAttribute ("DataRate", DataRateValue (m_dataRate));
//    app2.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
//
//    ApplicationContainer apps2 = app2.Install (nodesCon);
//    apps2.Add(app1.Install (sinksCon));
//    apps2.Start (Seconds (4.5));
//    apps2.Stop (Seconds (simStop + 1));


  //Ptr<Node> sinkNode = sinksCon.Get(0);

  //TypeId psfid = TypeId::LookupByName ("ns3::AquaSimSocketFactory");

  //Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  //sinkSocket->Bind (socket);
  //Ptr<OnOffNDApplication> onoff = DynamicCast<OnOffNDApplication> (sinkNode->GetApplication (0));
  //onoff->SetSocket(sinkSocket);
  //apps.Add(onoff);


/*
 *  For channel trace driven simulation
 */
/*
  AquaSimTraceReader tReader;
  tReader.SetChannel(asHelper.GetChannel());
  if (tReader.ReadFile("channelTrace.txt")) NS_LOG_DEBUG("Trace Reader Success");
  else NS_LOG_DEBUG("Trace Reader Failure");
*/

  Packet::EnablePrinting (); //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop));

  Simulator::Run();
  Simulator::Destroy();


  std::cout << "fin.\n";
  return 0;
}
