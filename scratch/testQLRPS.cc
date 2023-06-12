
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

NS_LOG_COMPONENT_DEFINE("QLRPS");

int main(int argc, char *argv[])
{
	double simStop = 1700; //seconds
	int nodes = 45;
     int sinks = 1;//sink node num
     int source=1;//source node num
	uint32_t m_dataRate = 80;
	uint32_t m_packetSize = 32;
	double range = 1000;
    
	std::ofstream outfile("info.txt", std::ios::app);

	LogComponentEnable("AquaSimAloha", LOG_LEVEL_DEBUG);
	LogComponentEnable("AquaSimPhyCmn", LOG_LEVEL_DEBUG);

	//to change on the fly
	CommandLine cmd;
	cmd.AddValue("simStop", "Length of simulation", simStop);
	cmd.AddValue("nodes", "Amount of regular underwater nodes", nodes);
    cmd.AddValue ("sinks", "Amount of underwater sinks", sinks);
	cmd.Parse(argc, argv);

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
	//asHelper.SetRouting("ns3::AquaSimStaticRouting");
	//asHelper.SetRouting("ns3::AquaSimDBR");
	//asHelper.SetRouting("ns3::AquaSimVBF", "Width", DoubleValue(100), "TargetPos", Vector3DValue(Vector(190,190,0)));
	asHelper.SetRouting("ns3::AquaSimQLRPS", "Width", DoubleValue(100), "TargetPos", Vector3DValue(Vector(1000,1000,4200)));
	//asHelper.SetRouting("ns3::AquaSimVBVA");
	//asHelper.SetRouting("ns3::AquaSimDynamicRouting","N",IntegerValue(nodes));
	//asHelper.SetMac("ns3::AquaSimTDMA");
	asHelper.SetMac("ns3::AquaSimAloha");

	MobilityHelper mobility;
    //MobilityHelper nodeMobility;
	NetDeviceContainer devices;
	Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();
	//Vector boundry = Vector((rand() % 4001),(rand() % 4001),0);
	//Vector boundry = Vector((1000), (1010), 100); //2800/100

  std::cout << "Creating Nodes\n";
   //source node create
  Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
  position->Add(Vector(1000,1000,0));
  devices.Add(asHelper.Create(senderCon.Get(0),newDevice));
  newDevice->GetPhy()->SetTransRange(range);
  std::cout<<"source node Add:"<<newDevice->GetAddress()/*<<",getposition:"<<newDevice->GetPosition().x
	  <<","<<newDevice->GetPosition().y<<","<<newDevice->GetPosition().z*/<<"\n";
  outfile<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<" "<<1000<<" "<<1000<<" "<<0<<"\n";
  //comman nodes
	/*Vector boundry = Vector((500), (500), 700);

	//int num = rand() % n +a;其中的a是起始值，n-1+a是终止值，n是整数的范围
	int k = 1;
    int level=1;
	for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End();
		 i++)
	{
		//sleep(5);
		std::cout<<*i<<"\n";
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		position->Add(boundry);
		devices.Add(asHelper.Create(*i, newDevice));
		newDevice->GetPhy()->SetTransRange(range);
		NS_LOG_DEBUG(
			"Node:" << newDevice->GetAddress() << " position(x):" << boundry.x << "  position(y):" << boundry.y << "  position(z):" << boundry.z);
			std::cout<<"newDevice Address:"<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<".position.x:"
			<<boundry.x<<".position.y:"<<boundry.y<<".position.z:"<<boundry.z<<"\n";
		outfile<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<" "<<boundry.x<<" "<<boundry.y<<" "<<boundry.z<<"\n";	
		boundry.x = rand()%1500+500;
        boundry.y=rand()%1500+500;
		boundry.z=700*level;
		
        k++;
     if(k==20||k==40||k==60||k==80||k==100){
        level++;
    }
	}*/
		Vector boundry = Vector((0), (0), 700);
    int pos[8][2]={{0,1000},{0,2000},{1000,0},{1000,1000},{1000,2000},{2000,0},{2000,1000},{2000,2000}};
    int index=0;
	int k = 1;
    int level=1;
	for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End();
		 i++)
	{
		//sleep(5);
		//std::cout<<*i<<"\n";
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		position->Add(boundry);
		devices.Add(asHelper.Create(*i, newDevice));
		newDevice->GetPhy()->SetTransRange(range);
		NS_LOG_DEBUG(
			"Node:" << newDevice->GetAddress() << " position(x):" << boundry.x << "  position(y):" << boundry.y << "  position(z):" << boundry.z);
			/*std::cout<<"newDevice Address:"<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<".position.x:"
			<<boundry.x<<".position.y:"<<boundry.y<<".position.z:"<<boundry.z<<"\n";*/
		outfile<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<" "<<boundry.x<<" "<<boundry.y<<" "<<boundry.z<<"\n";	
		boundry.x = pos[index][0];
        boundry.y=pos[index][1];
		boundry.z=700*level;
		index++;
        if(index==8){index=0;}
        k++;
     if(k==9||k==18||k==27||k==36||k==45){
        level++;
    }
	}
//sink node create
  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(Vector(1000,1000,4200));
      devices.Add(asHelper.Create(*i, newDevice));
      newDevice->GetPhy()->SetTransRange(range);
	  std::cout<<"sink node Add:"<<newDevice->GetAddress()/*<<",getposition:"<<newDevice->GetPosition().x
	  <<","<<newDevice->GetPosition().y<<","<<newDevice->GetPosition().z*/<<"\n";
	  outfile<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<" "<<1000<<" "<<1000<<" "<<4200<<"\n";
    }
	
	outfile.close();
    std::cout << "Creating Nodes End\n";

	mobility.SetPositionAllocator(position);
	/*mobility.SetMobilityModel("ns3::RandomWalk3dMobilityModel", "Mode",
							  StringValue("Time"), "Time", StringValue("2s"), "Speed",
							  //StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("0|100000|0|100000"));*/
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(senderCon);
	mobility.Install(nodesCon);
    mobility.Install(sinksCon);
	//PacketSocketAddress socket;
	AquaSimSocketAddress socket;
	socket.SetAllDevices();
	socket.SetDestinationAddress(devices.Get(51)->GetAddress()); //Set dest to first sink (nodes+1 device)
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

	//ApplicationContainer apps = app.Install(nodesCon);
	ApplicationContainer apps=app.Install(senderCon);
	apps.Start(Seconds(1400));
	apps.Stop(Seconds(simStop));//how to set stop time


	Packet::EnablePrinting(); //for debugging purposes
	std::cout << "-----------Running Simulation-----------\n";
	Simulator::Stop(Seconds(simStop));
	Simulator::Run();
	Simulator::Destroy();

	std::cout << "fin.\n";
	return 0;
}