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

NS_LOG_COMPONENT_DEFINE("SFamaTest");

int main(int argc, char *argv[])
{
	double simStop = 200000; //seconds
	int nodes = 5;
	uint32_t m_dataRate = 80;
	uint32_t m_packetSize = 32;
	//double range = 20;
	//LogComponentEnable ("AquaSimSFama", LOG_LEVEL_DEBUG);
	LogComponentEnable("SFamaTest", LOG_LEVEL_DEBUG);

	//to change on the fly
	CommandLine cmd;
	cmd.AddValue("simStop", "Length of simulation", simStop);
	cmd.AddValue("nodes", "Amount of regular underwater nodes", nodes);
	cmd.Parse(argc, argv);

	// GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

	std::cout << "-----------Initializing simulation-----------\n";

	NodeContainer nodesCon;
	nodesCon.Create(nodes);

	//PacketSocketHelper socketHelper;
	AquaSimSocketHelper socketHelper;
	socketHelper.Install(nodesCon);

	//establish layers using helper's pre-build settings
	AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
	//channel.SetPropagation("ns3::AquaSimRangePropagation");
	AquaSimHelper asHelper = AquaSimHelper::Default();
	asHelper.SetChannel(channel.Create());
	asHelper.SetRouting("ns3::AquaSimStaticRouting");
	asHelper.SetMac("ns3::AquaSimSFama");

	MobilityHelper mobility;
	NetDeviceContainer devices;
	Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();

	Vector boundry = Vector((1000), (1000), 0);

	std::cout << "Creating Nodes\n";
	for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End();
	     i++)
	{
		//sleep(5);
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		position->Add(boundry);
		devices.Add(asHelper.Create(*i, newDevice));
		NS_LOG_DEBUG(
		    "Node:" << newDevice->GetAddress() << " position(x):" << boundry.x << "  position(y):" << boundry.y);
		boundry.x = (boundry.x + 4000);
		boundry.y = (boundry.y);
		//newDevice->GetPhy()->SetTransRange(range);
	}

	mobility.SetPositionAllocator(position);
	mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
				  "Mode", StringValue("Time"),
				  "Time", StringValue("2s"),
				  "Speed", StringValue("ns3::UniformRandomVariable[Min=0.5|Max=1]"),
				  "Bounds", StringValue("0|100000|0|100000"));
	mobility.Install(nodesCon);

	//PacketSocketAddress socket;
	AquaSimSocketAddress socket;
	socket.SetAllDevices();
	socket.SetDestinationAddress(devices.Get(4)->GetAddress()); //Set dest to first sink (nodes+1 device)
	socket.SetProtocol(0);
	// socket.SetDestinationAddress(devices.Get(nodes)->GetAddress());

	//OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
	OnOffNdHelper app("ns3::AquaSimSocketFactory", Address(socket));
	app.SetAttribute("OnTime", StringValue("ns3::ExponentialRandomVariable[Mean=100|Bound=0.0]"));
	app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	app.SetAttribute("DataRate", DataRateValue(m_dataRate));
	app.SetAttribute("PacketSize", UintegerValue(m_packetSize));
	// app.SetAttribute ("Interval", StringValue ("ns3::ExponentialRandomVariable[Mean=0.01|Bound=0.0]"));

	ApplicationContainer apps = app.Install(nodesCon);
	apps.Start(Seconds(0.5));
	apps.Stop(Seconds(simStop));

	/*
 *  For channel trace driven simulation
 */
	/*
  AquaSimTraceReader tReader;
  tReader.SetChannel(asHelper.GetChannel());
  if (tReader.ReadFile("channelTrace.txt")) NS_LOG_DEBUG("Trace Reader Success");
  else NS_LOG_DEBUG("Trace Reader Failure");
*/

	Packet::EnablePrinting(); //for debugging purposes
	std::cout << "-----------Running Simulation-----------\n";
	Simulator::Stop(Seconds(simStop));
	Simulator::Run();
	Simulator::Destroy();

	std::cout << "fin.\n";
	return 0;
}
