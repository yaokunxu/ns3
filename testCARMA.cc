/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *
 * Date:2022.4.7
 * Author: Hancheng <827569146@qq.com>
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aqua-sim-mobility-ug.h"
#include "ns3/aqua-sim-mobility-ug2.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "ns3/netanim-module.h"
/*
 * PMAC
 *
 * String topology:
 * S---->  N  -----> N -----> N -----> D
 *
 */
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CARMA");

int main(int argc, char *argv[])
{
	double simStop = 17000; // seconds
	int nodes = 27;			// pos
	// int nodes=125;//pos1
	// int nodes=225;//pos2
	// int nodes=315;//pos3
	// int nodes=360;//pos4
	int sinks = 1;	// sink node num
	int source = 3; // source node num
	int allNodes = 30;
	uint32_t m_dataRate = 80;
	uint32_t m_packetSize = 32;
	double range = 1500;
	double percent = 0.9;

	std::ofstream outfile("info.txt", std::ios::app);

	LogComponentEnable("AquaSimAloha", LOG_LEVEL_DEBUG);
	LogComponentEnable("AquaSimPhyCmn", LOG_LEVEL_DEBUG);

	// to change on the fly
	CommandLine cmd;
	cmd.AddValue("simStop", "Length of simulation", simStop);
	cmd.AddValue("nodes", "Amount of regular underwater nodes", nodes);
	cmd.AddValue("sinks", "Amount of underwater sinks", sinks);
	cmd.AddValue("percent", "percent of dynamic nodes", percent);
	cmd.AddValue("total", "sum num of all nodes", allNodes);
	cmd.Parse(argc, argv);
	nodes = allNodes * percent;
	source = allNodes - nodes;

	// GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

	std::cout << "-----------Initializing simulation-----------\n";

	NodeContainer nodesCon;
	NodeContainer sinksCon;
	NodeContainer senderCon;
	senderCon.Create(source);
	nodesCon.Create(nodes);
	sinksCon.Create(sinks);

	AquaSimSocketHelper socketHelper;
	socketHelper.Install(senderCon);
	socketHelper.Install(nodesCon);
	socketHelper.Install(sinksCon);

	AquaSimChannelHelper channel = AquaSimChannelHelper::Default();

	AquaSimHelper asHelper = AquaSimHelper::Default();
	asHelper.SetChannel(channel.Create());
	// asHelper.SetRouting("ns3::AquaSimStaticRouting");
	// asHelper.SetRouting("ns3::AquaSimDBR");
	// asHelper.SetRouting("ns3::AquaSimVBF", "Width", DoubleValue(600), "TargetPos", Vector3DValue(Vector(1000,1000,4200)));
	// asHelper.SetRouting("ns3::AquaSimDBR");
	// asHelper.SetRouting("ns3::AquaSimFloodingRouting");
	asHelper.SetRouting("ns3::AquaSimCARMA", "TargetID", IntegerValue(allNodes + 1));
	// asHelper.SetRouting("ns3::AquaSimVBVA");
	// asHelper.SetRouting("ns3::AquaSimDynamicRouting","N",IntegerValue(nodes));
	// asHelper.SetMac("ns3::AquaSimTDMA");
	asHelper.SetMac("ns3::AquaSimAloha");
	// asHelper.SetMac("ns3::AquaSimBroadcastMac");

	MobilityHelper mobility;
	MobilityHelper mobilitymove;
	// MobilityHelper nodeMobility;
	NetDeviceContainer devices;
	Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();
	Ptr<ListPositionAllocator> positionnode = CreateObject<ListPositionAllocator>();
	// Vector boundry = Vector((rand() % 4001),(rand() % 4001),0);
	// Vector boundry = Vector((1000), (1010), 100); //2800/100

	std::cout << "Creating Nodes\n";
	// source node create
	// todo 修改源节点坐标
	// position->Add(Vector(1000,1000,0));
	/*std::cout<<"source node Add:"<<newDevice->GetAddress()/*<<",getposition:"<<newDevice->GetPosition().x
		<<","<<newDevice->GetPosition().y<<","<<newDevice->GetPosition().z<<"\n";*/

	for (NodeContainer::Iterator i = senderCon.Begin(); i != senderCon.End(); i++)
	{
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		position->Add(Vector(rand() % 5000, rand() % 5000, rand() % 500 + 3000));
		devices.Add(asHelper.Create(*i, newDevice));
		newDevice->GetPhy()->SetTransRange(range);
	}
	int level = 1;
	int count = (double)nodes / 2;
	for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
	{
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		if (level == 1)
		{
			positionnode->Add(Vector(rand() % 5000, rand() % 5000, rand() % 500 + 1000));
			count--;
			if (count <= 0)
			{
				level = 2;
			}
		}
		if (level == 2)
		{
			positionnode->Add(Vector(rand() % 5000, rand() % 5000, rand() % 500 + 2000));
		}
		devices.Add(asHelper.Create(*i, newDevice));
		newDevice->GetPhy()->SetTransRange(range);
	}
	for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
	{
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		position->Add(Vector(2500, 2500, 10));
		devices.Add(asHelper.Create(*i, newDevice));
		newDevice->GetPhy()->SetTransRange(range);
	}

	std::cout << "Creating Nodes End\n";

	// 移动模型,以后有机会再考虑移动的
	/*mobility.SetMobilityModel("ns3::RandomWalk3dMobilityModel", "Mode",
							  StringValue("Time"), "Time", StringValue("2s"), "Speed",
							  //StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("0|2100|0|2100"));*/
	mobility.SetPositionAllocator(position);
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(senderCon);
	mobility.Install(sinksCon);

	mobilitymove.SetPositionAllocator(positionnode);
	// mobilitymove.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilitymove.SetMobilityModel("ns3::UnderwaterGilderMobilityModel2",
								  "Speed", StringValue("ns3::UniformRandomVariable[Min=2.0|Max=3.0]"),
								  "Bounds", StringValue("0|5000|0|5000"));
	mobilitymove.Install(nodesCon);

	AquaSimSocketAddress socket;
	socket.SetAllDevices();
	socket.SetDestinationAddress(devices.Get(allNodes)->GetAddress()); // Set dest to first sink (nodes+1 device)
	socket.SetProtocol(0);
	// OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
	OnOffNdHelper app("ns3::AquaSimSocketFactory", Address(socket));
	app.SetAttribute("OnTime", StringValue("ns3::ExponentialRandomVariable[Mean=100|Bound=0.0]"));
	app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	app.SetAttribute("DataRate", DataRateValue(m_dataRate));
	app.SetAttribute("PacketSize", UintegerValue(m_packetSize));
	// app.SetAttribute ("Interval", StringValue ("ns3::ExponentialRandomVariable[Mean=0.01|Bound=0.0]"));

	// ApplicationContainer apps = app.Install(nodesCon);
	ApplicationContainer apps = app.Install(senderCon);
	apps.Start(Seconds(1400));
	apps.Stop(Seconds(simStop)); // how to set stop time

	Packet::EnablePrinting(); // for debugging purposes
	std::cout << "-----------Running Simulation-----------\n";
	Simulator::Stop(Seconds(simStop));
	AnimationInterface anim("carma.xml");
	Simulator::Run();
	Simulator::Destroy();

	std::cout << "fin.\n";
	return 0;
}