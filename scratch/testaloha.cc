
/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *
 * Date:2021.4.7
 * Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include <fstream>
#include <unistd.h>
#include <iostream>

/*
 * PMAC
 *
 * String topology:
 * S---->  N  -----> N -----> N -----> D
 *
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Pmac");

int main(int argc, char *argv[])
{
	double simStop = 1700; //seconds
	int nodes = 6;
	uint32_t m_dataRate = 80;
	uint32_t m_packetSize = 32;
	//double range = 20;

	LogComponentEnable("AquaSimAloha", LOG_LEVEL_DEBUG);
	LogComponentEnable("AquaSimPhyCmn", LOG_LEVEL_DEBUG);

	//to change on the fly
	CommandLine cmd;
	cmd.AddValue("simStop", "Length of simulation", simStop);
	cmd.AddValue("nodes", "Amount of regular underwater nodes", nodes);
	cmd.Parse(argc, argv);

	// GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

	std::cout << "-----------Initializing simulation-----------\n";

	NodeContainer nodesCon;
	nodesCon.Create(nodes);

	AquaSimSocketHelper socketHelper;
	socketHelper.Install(nodesCon);

	AquaSimChannelHelper channel = AquaSimChannelHelper::Default();

	AquaSimHelper asHelper = AquaSimHelper::Default();
	asHelper.SetChannel(channel.Create());
	asHelper.SetRouting("ns3::AquaSimStaticRouting");
	//asHelper.SetRouting("ns3::AquaSimDBR");
	//asHelper.SetRouting("ns3::AquaSimVBF", "Width", DoubleValue(100), "TargetPos", Vector3DValue(Vector(190,190,0)));
	//asHelper.SetRouting("ns3::AquaSimQLRPS", "Width", DoubleValue(100), "TargetPos", Vector3DValue(Vector(190,190,0)));
	//asHelper.SetRouting("ns3::AquaSimVBVA");
	//asHelper.SetRouting("ns3::AquaSimDynamicRouting","N",IntegerValue(nodes));
	
	asHelper.SetMac("ns3::AquaSimAloha");

	MobilityHelper mobility;
	NetDeviceContainer devices;
	Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();
	//Vector boundry = Vector((rand() % 4001),(rand() % 4001),0);
	//Vector boundry = Vector((1000), (1010), 100); //2800/100

	Vector boundry = Vector((1000), (1010), 100);

	std::cout << "Creating Nodes\n";
	int k = 0;
    
	for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End();
		 i++)
	{
		//sleep(5);
		std::cout<<*i<<"\n";
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		position->Add(boundry);
		devices.Add(asHelper.Create(*i, newDevice));
		NS_LOG_DEBUG(
			"Node:" << newDevice->GetAddress() << " position(x):" << boundry.x << "  position(y):" << boundry.y << "  position(z):" << boundry.z);
			std::cout<<"newDevice Address:"<<"position.x:"<<boundry.x<<"position.y:"<<boundry.y<<"position.z:"<<boundry.z<<"\n";
		boundry.x = 1000;
		boundry.y = boundry.y + 1000;
	}

	std::cout << "Creating Nodes End\n";

	mobility.SetPositionAllocator(position);
	mobility.SetMobilityModel("ns3::RandomWalk3dMobilityModel", "Mode",
							  StringValue("Time"), "Time", StringValue("2s"), "Speed",
							  //StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("ns3::UniformRandomVariable[Min=0|Max=5]"), "Bounds",
							  StringValue("0|100000|0|100000"));
	
	mobility.Install(nodesCon);

	//PacketSocketAddress socket;
	AquaSimSocketAddress socket;
	socket.SetAllDevices();
	socket.SetDestinationAddress(devices.Get(4)->GetAddress()); //Set dest to first sink (nodes+1 device)
	socket.SetProtocol(0);

	//OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
	OnOffNdHelper app("ns3::AquaSimSocketFactory", Address(socket));
	app.SetAttribute("OnTime",
					 StringValue("ns3::ExponentialRandomVariable[Mean=100|Bound=0.0]"));
	app.SetAttribute("OffTime",
					 StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	app.SetAttribute("DataRate", DataRateValue(m_dataRate));
	app.SetAttribute("PacketSize", UintegerValue(m_packetSize));
	// app.SetAttribute ("Interval", StringValue ("ns3::ExponentialRandomVariable[Mean=0.01|Bound=0.0]"));

	ApplicationContainer apps = app.Install(nodesCon);
	apps.Start(Seconds(1400));
	apps.Stop(Seconds(simStop));


	Packet::EnablePrinting(); //for debugging purposes
	std::cout << "-----------Running Simulation-----------\n";
	Simulator::Stop(Seconds(simStop));
	Simulator::Run();
	Simulator::Destroy();

	std::cout << "fin.\n";
	return 0;
}
