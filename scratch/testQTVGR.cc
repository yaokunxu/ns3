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
#include "ns3/aqua-sim-mobility-ug2.h"
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

extern Vector allNodePositionQTVGR[18000][28];

void printallNodePositionQTVGR(NodeContainer node)
{
	int t = Simulator::Now().GetSeconds();
	/*for (int i = 0; i < 27; i++)
	{
		std::cout << "first loop"
				  << "t:" << t << " id:" << i << " allNodePos:" << allNodePositionQTVGR[t][i] << std::endl;
	}*/
	std::ofstream outfile("Nodeposition.txt", std::ios::app);
	for (NodeContainer::Iterator i = node.Begin(); i != node.End(); ++i)
	{
		Ptr<Object> object = *i;
		int id = (*i)->GetId();
		// std::cout << "second loop" << "t:" << t << " id:" << id << " NodePos:" << (*i)->GetObject<MobilityModel>()->GetPosition()<< std::endl;
		outfile << "t:" << t << " id:" << id << " Pos:" << (*i)->GetObject<MobilityModel>()->GetPosition() << "\n";
	}
	outfile.close();
}

void judgecloseenough()
{
	std::cout << std::endl
			  << "judgeclosrenough ";
	std::cout << Simulator::Now().GetSeconds() << std::endl;
	int t = Simulator::Now().GetSeconds();
	double dis[27][27];
	for (int i = 0; i < 27; i++)
	{
		Vector p = allNodePositionQTVGR[t][i];
		for (int j = i + 1; j < 27; j++)
		{
			Vector q = allNodePositionQTVGR[t][j];
			double d = sqrt((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y) + (p.z - q.z) * (p.z - q.z));
			dis[i][j] = d;
		}
	}
	for (int i = 0; i < 27; i++)
	{
		std::cout << i << " ";
	}
	std::cout << std::endl;
	for (int i = 0; i < 27; i++)
	{
		for (int j = i + 1; j < 27; j++)
		{
			std::cout << dis[i][j] << " ";
		}
		std::cout << std::endl;
	}
	for (int i = 0; i < 27; i++)
	{
		std::cout << i << " ";
	}
	std::cout << std::endl;
	for (int i = 0; i < 27; i++)
	{
		std::cout << i << " ";
		for (int j = i + 1; j < 27; j++)
		{
			if (dis[i][j] > 1500)
			{
				std::cout << "N ";
			}
			else
			{
				std::cout << "Y ";
			}
		}
		std::cout << std::endl;
	}

	return;
}

void DoWalk(NodeContainer node, int delayLeft) // Predict
{
	// std::cout << "DoWalk" << std::endl;
	for (NodeContainer::Iterator i = node.Begin(); i != node.End(); ++i)
	{
		// std::cout << "dowalkstart" << std::endl;

		Ptr<Object> object = *i;
		Ptr<MobilityModel> model = object->GetObject<MobilityModel>();
		// Ptr<ConstantVelocityHelper> m_helper = object->GetObject<ConstantVelocityHelper>();
		Ptr<UnderwaterGilderMobilityModel2> m_model = object->GetObject<UnderwaterGilderMobilityModel2>();
		/*std::cout << "now=" << Simulator::Now().GetSeconds()
				  << " node=" << (*i)->GetId() << std::endl;*/

		int id = (*i)->GetId();
		int t = Simulator::Now().GetSeconds();
		ConstantVelocityHelper m_helper = m_model->DoGetMHelper();
		Rectangle m_bounds = m_model->DoGetMBounds();
		m_helper.UpdateWithBounds(m_bounds);
		double m_direction = m_model->DoGetMDirection();
		double m_speed = m_model->DoGetMSpeed();
		// std::cout << "before the loop" << std::endl;
		Vector position = m_helper.GetCurrentPosition();
		Vector velocity = m_helper.GetVelocity();
		Vector nextPos = position;
		for (int i = 0; i <= delayLeft; i += 1)
		{
			// std::cout << "strat of the loop" << i << std::endl;
			nextPos.x += velocity.x;
			nextPos.y += velocity.y;
			// nextPos.x = std::min(m_bounds.xMax, nextPos.x);
			// nextPos.x = std::max(m_bounds.xMin, nextPos.x);
			// nextPos.y = std::min(m_bounds.yMax, nextPos.y);
			// nextPos.y = std::max(m_bounds.yMin, nextPos.y);
			nextPos.x = abs(nextPos.x);
			nextPos.y = abs(nextPos.y);
			allNodePositionQTVGR[t + i + 1][id] = nextPos;
			// std::cout << "position: " << position << " velocity: " << velocity << " nextPos:" << nextPos << " allNodePositionQTVGR:" << allNodePositionQTVGR[t + i + 1][id] << std::endl;

			// Make sure that the position by the next time step is still within the boundary.
			// If out of bounds, then alter the velocity vector and average direction to keep the position in bounds

			bool keepOriginalState = true;
			if (!m_bounds.IsInside(nextPos))
			{
				keepOriginalState = false;
				if (nextPos.x > m_bounds.xMax || nextPos.x < m_bounds.xMin)
				{
					velocity.x = -velocity.x;
					m_direction = M_PI - m_direction;
					if (m_direction < 0)
						m_direction += 2 * M_PI;
				}
				if (nextPos.y > m_bounds.yMax || nextPos.y < m_bounds.yMin)
				{
					velocity.y = -velocity.y;
					m_direction = -m_direction;
					if (m_direction < 0)
						m_direction += 2 * M_PI;
				}
			}
			// Check whether the node is out of depth(random change)
		}
	}
}

static void DectConnection(NodeContainer node)
{
	// std::cout << "start" << std::endl;
	// std::cout << Simulator::Now().GetSeconds() << std::endl;
	// std::cout << "GetN " << node.GetN() << std::endl;

	for (NodeContainer::Iterator i = node.Begin(); i != node.End(); ++i)
	{
		Ptr<Object> object = *i;
		Ptr<MobilityModel> model = object->GetObject<MobilityModel>();
		int t = Simulator::Now().GetSeconds();
		// std::cout << "now=" << Simulator::Now().GetSeconds()
		//<< " node=" << (*i)->GetId() << std::endl;
		Vector pos = model->GetPosition();
		uint32_t id = (*i)->GetId();
		// std::cout << "pos" << pos << std::endl;
		if (t == 0)
		{
			allNodePositionQTVGR[0][id] = pos;
		}
		/*if (pos == allNodePositionQTVGR[t][id])
		{
			std::cout << "YE,t:" << t << " id:" << id << " pos:" << pos << " allNodePositionQTVGR:" << allNodePositionQTVGR[t][id] << std::endl;
		}
		else
		{
			std::cout << "NE,t:" << t << "id:" << id << " pos:" << pos << " allNodePositionQTVGR:" << allNodePositionQTVGR[t][id] << std::endl;
		}*/
		// std::cout << "pos:" << pos << "allNodePositionQTVGR:" << allNodePositionQTVGR[t][id] << std::endl;
		allNodePositionQTVGR[t][id] = pos;
		// if (t != 0)
		//{
		//	std::cout << allNodePositionQTVGR[t - 1][id] << std::endl;
		// }
		// std::cout << allNodePositionQTVGR[t][id] << std::endl;
	}
	DoWalk(node, 500);
}

static void
CourseChange(std::string foo, Ptr<const MobilityModel> mobility)
{
	Vector pos = mobility->GetPosition();
	Vector vel = mobility->GetVelocity();
	// std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
	//           << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
	//           << ", z=" << vel.z << std::endl;
}

NS_LOG_COMPONENT_DEFINE("QTVGR");

int main(int argc, char *argv[])
{
	Config::SetDefault("ns3::UnderwaterGilderMobilityModel2::Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"));
	Config::SetDefault("ns3::UnderwaterGilderMobilityModel2::Bounds", StringValue("0|5000|0|5000"));

	double simStop = 17000; // seconds
	int nodes = 25;			// pos
	// int nodes=125;//pos1
	// int nodes=225;//pos2
	// int nodes=315;//pos3
	// int nodes=360;//pos4
	int sinks = 1;	// sink node num
	int source = 1; // source node num
	uint32_t m_dataRate = 80;
	uint32_t m_packetSize = 32;
	double range = 1500;

	LogComponentEnable("AquaSimAloha", LOG_LEVEL_DEBUG);
	LogComponentEnable("AquaSimPhyCmn", LOG_LEVEL_DEBUG);
	// LogComponentEnable("OnOffNDApplication", LOG_LEVEL_DEBUG);
	// LogComponentEnable("AquaSimQDTR", LOG_LEVEL_ALL);

	// to change on the fly
	CommandLine cmd;
	cmd.AddValue("simStop", "Length of simulation", simStop);
	cmd.AddValue("nodes", "Amount of regular underwater nodes", nodes);
	cmd.AddValue("sinks", "Amount of underwater sinks", sinks);
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
	// asHelper.SetRouting("ns3::AquaSimStaticRouting");
	// asHelper.SetRouting("ns3::AquaSimDBR");
	// asHelper.SetRouting("ns3::AquaSimVBF", "Width", DoubleValue(600), "TargetPos", Vector3DValue(Vector(1000,1000,4200)));
	// asHelper.SetRouting("ns3::AquaSimDBR");
	// asHelper.SetRouting("ns3::AquaSimFloodingRouting");
	asHelper.SetRouting("ns3::AquaSimQTVGR", "Nodenum", IntegerValue(27));
	// asHelper.SetRouting("ns3::AquaSimVBVA");
	// asHelper.SetRouting("ns3::AquaSimDynamicRouting","N",IntegerValue(nodes));
	// asHelper.SetMac("ns3::AquaSimTDMA");
	asHelper.SetMac("ns3::AquaSimAloha");
	// asHelper.SetMac("ns3::AquaSimBroadcastMac");

	MobilityHelper mobility;
	// MobilityHelper nodeMobility;
	NetDeviceContainer devices;
	// Vector boundry = Vector((rand() % 4001),(rand() % 4001),0);
	// Vector boundry = Vector((1000), (1010), 100); //2800/100

	std::cout << "Creating Nodes\n";
	// source node create
	Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
	// todo 修改源节点坐标
	// position->Add(Vector(1000,1000,0));
	devices.Add(asHelper.Create(senderCon.Get(0), newDevice));
	newDevice->GetPhy()->SetTransRange(range);
	/*std::cout<<"source node Add:"<<newDevice->GetAddress()/*<<",getposition:"<<newDevice->GetPosition().x
		<<","<<newDevice->GetPosition().y<<","<<newDevice->GetPosition().z<<"\n";*/
	// comman nodes
	// comman nodes
	for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End();
		 i++)
	{
		// sleep(5);
		// std::cout<<*i<<"\n";
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		devices.Add(asHelper.Create(*i, newDevice));
		newDevice->GetPhy()->SetTransRange(range);
		std::cout << "the ndoeid " << (*i)->GetId() << std::endl;
		/*std::cout<<"newDevice Address:"<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<".position.x:"
		<<boundry.x<<".position.y:"<<boundry.y<<".position.z:"<<boundry.z<<"\n";*/
	}
	// sink node create
	for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
	{
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		// todo 修改目标节点坐标
		// position->Add(Vector(1000,1500,4200));
		devices.Add(asHelper.Create(*i, newDevice));
		newDevice->GetPhy()->SetTransRange(range);
		/*std::cout<<"sink node Add:"<<newDevice->GetAddress()/*<<",getposition:"<<newDevice->GetPosition().x
		<<","<<newDevice->GetPosition().y<<","<<newDevice->GetPosition().z<<"\n";*/
	}

	std::cout << "Creating Nodes End\n";

	mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
								  "X", StringValue("1000"),
								  "Y", StringValue("1000"),
								  "Rho", StringValue("ns3::UniformRandomVariable[Min=500|Max=1000]"));
	mobility.SetMobilityModel("ns3::UnderwaterGilderMobilityModel2",
							  "Speed", StringValue("ns3::UniformRandomVariable[Min=2.0|Max=10.0]"),
							  "Bounds", StringValue("0|5000|0|5000"));

	// mobility.SetPositionAllocator(position);
	//  移动模型,以后有机会再考虑移动的
	/*mobility.SetMobilityModel("ns3::RandomWalk3dMobilityModel", "Mode",
							  StringValue("Time"), "Time", StringValue("2s"), "Speed",
							  //StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("0|2100|0|2100"));*/
	// mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(senderCon);
	mobility.Install(nodesCon);
	mobility.Install(sinksCon);
	// PacketSocketAddress socket;
	AquaSimSocketAddress socket;
	socket.SetAllDevices();

	socket.SetDestinationAddress(devices.Get(26)->GetAddress()); // Set dest to first sink (nodes+1 device)

	socket.SetProtocol(0);

	// OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
	OnOffNdHelper app("ns3::AquaSimSocketFactory", Address(socket));
	app.SetAttribute("OnTime",
					 StringValue("ns3::ExponentialRandomVariable[Mean=100|Bound=0.0]"));
	app.SetAttribute("OffTime",
					 StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	app.SetAttribute("DataRate", DataRateValue(m_dataRate));
	app.SetAttribute("PacketSize", UintegerValue(m_packetSize));
	// app.SetAttribute ("Interval", StringValue ("ns3::ExponentialRandomVariable[Mean=0.01|Bound=0.0]"));

	// ApplicationContainer apps = app.Install(nodesCon);
	ApplicationContainer apps = app.Install(senderCon);
	apps.Start(Seconds(1400));
	apps.Stop(Seconds(simStop)); // how to set stop time

	Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
					MakeCallback(&CourseChange));

	// Simulator::Stop(Seconds(1000.0));
	for (int i = 0; i < 17000; i++)
	{
		Simulator::Schedule(Seconds((double)i), &DectConnection, nodesCon);
		Simulator::Schedule(Seconds((double)i), &DectConnection, sinksCon);
		Simulator::Schedule(Seconds((double)i), &DectConnection, senderCon);
		// Simulator::Schedule(Seconds((double)i), &printallNodePositionQTVGR, senderCon);
		// Simulator::Schedule(Seconds((double)i), &printallNodePositionQTVGR, nodesCon);
		// Simulator::Schedule(Seconds((double)i), &printallNodePositionQTVGR, sinksCon);
		//   Simulator::Schedule(Seconds((double)i), &judgecloseenough);
	}

	Packet::EnablePrinting(); // for debugging purposes
	std::cout << "-----------Running Simulation-----------\n";
	Simulator::Stop(Seconds(simStop));
	AnimationInterface anim("roadQTVGR.xml");
	Simulator::Run();
	Simulator::Destroy();

	std::cout << "fin.\n";
	return 0;
}