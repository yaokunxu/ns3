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
 * Author: Hancheng <847569146@qq.com>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/aqua-sim-mobility-ug2.h"
#include "ns3/callback.h"
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <vector>
/*
 * PMAC
 *
 * String topology:
 * S---->  N  -----> N -----> N -----> D
 *
 */

using namespace ns3;

extern Vector allNodePosition2[18000][48];

NS_LOG_COMPONENT_DEFINE("QLRPS2");

void printallNodePosition2(NodeContainer node)
{
	int t = Simulator::Now().GetSeconds();
	/*for (int i = 0; i < 47; i++)
	{
		std::cout << "first loop"
				  << "t:" << t << " id:" << i << " allNodePos:" << allNodePosition2[t][i] << std::endl;
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
	double dis[47][47];
	for (int i = 0; i < 47; i++)
	{
		Vector p = allNodePosition2[t][i];
		for (int j = i + 1; j < 47; j++)
		{
			Vector q = allNodePosition2[t][j];
			double d = sqrt((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y) + (p.z - q.z) * (p.z - q.z));
			dis[i][j] = d;
		}
	}
	for (int i = 0; i < 47; i++)
	{
		std::cout << i << " ";
	}
	std::cout << std::endl;
	for (int i = 0; i < 47; i++)
	{
		for (int j = i + 1; j < 47; j++)
		{
			std::cout << dis[i][j] << " ";
		}
		std::cout << std::endl;
	}
	for (int i = 0; i < 47; i++)
	{
		std::cout << i << " ";
	}
	std::cout << std::endl;
	for (int i = 0; i < 47; i++)
	{
		std::cout << i << " ";
		for (int j = i + 1; j < 47; j++)
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
			allNodePosition2[t + i + 1][id] = nextPos;
			// std::cout << "position: " << position << " velocity: " << velocity << " nextPos:" << nextPos << " allNodePosition2:" << allNodePosition2[t + i + 1][id] << std::endl;

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
			allNodePosition2[0][id] = pos;
		}
		/*if (pos == allNodePosition2[t][id])
		{
			std::cout << "YE,t:" << t << " id:" << id << " pos:" << pos << " allNodePosition2:" << allNodePosition2[t][id] << std::endl;
		}
		else
		{
			std::cout << "NE,t:" << t << "id:" << id << " pos:" << pos << " allNodePosition2:" << allNodePosition2[t][id] << std::endl;
		}*/
		// std::cout << "pos:" << pos << "allNodePosition2:" << allNodePosition2[t][id] << std::endl;
		allNodePosition2[t][id] = pos;
		// if (t != 0)
		//{
		//	std::cout << allNodePosition2[t - 1][id] << std::endl;
		// }
		// std::cout << allNodePosition2[t][id] << std::endl;
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

int main(int argc, char *argv[])
{
	double simStop = 17000; // seconds
	int nodes = 45;			// pos
	// int nodes=125;//pos1
	// int nodes=225;//pos2
	// int nodes=315;//pos3
	// int nodes=360;//pos4
	int sinks = 1;	// sink node num
	int source = 1; // source node num
	uint32_t m_dataRate = 80;
	uint32_t m_packetSize = 32;
	double range = 1500;

	std::ofstream outfile("info.txt", std::ios::app);

	LogComponentEnable("AquaSimAloha", LOG_LEVEL_DEBUG);
	LogComponentEnable("AquaSimPhyCmn", LOG_LEVEL_DEBUG);

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
	asHelper.SetRouting("ns3::AquaSimQLRPS2", "Width", DoubleValue(100), "TargetPos", Vector3DValue(Vector(1000, 1000, 4200)));
	// asHelper.SetRouting("ns3::AquaSimVBVA");
	// asHelper.SetRouting("ns3::AquaSimDynamicRouting","N",IntegerValue(nodes));
	// asHelper.SetMac("ns3::AquaSimTDMA");
	asHelper.SetMac("ns3::AquaSimAloha");
	// asHelper.SetMac("ns3::AquaSimBroadcastMac");

	MobilityHelper mobility;
	// MobilityHelper nodeMobility;
	NetDeviceContainer devices;
	Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();
	// Vector boundry = Vector((rand() % 4001),(rand() % 4001),0);
	// Vector boundry = Vector((1000), (1010), 100); //2800/100

	std::cout << "Creating Nodes\n";
	// source node create
	Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
	// todo 修改源节点坐标
	// position->Add(Vector(1000,1000,0));
	position->Add(Vector(0, 1000, 0));
	devices.Add(asHelper.Create(senderCon.Get(0), newDevice));
	newDevice->GetPhy()->SetTransRange(range);
	/*std::cout<<"source node Add:"<<newDevice->GetAddress()/*<<",getposition:"<<newDevice->GetPosition().x
		<<","<<newDevice->GetPosition().y<<","<<newDevice->GetPosition().z<<"\n";*/
	outfile << AquaSimAddress::ConvertFrom(newDevice->GetAddress()) << " " << 0 << " " << 1000 << " " << 0 << "\n";
	// comman nodes
	// comman nodes
	Vector boundry = Vector((0), (0), 700);
	int pos[8][2] = {{0, 1000}, {0, 2000}, {1000, 0}, {1000, 1000}, {1000, 2000}, {2000, 0}, {2000, 1000}, {2000, 2000}};
	int pos1[24][2] = {{0, 500}, {0, 1000}, {0, 1500}, {0, 2000}, {500, 0}, {500, 500}, {500, 1000}, {500, 1500}, {500, 2000}, {1000, 0}, {1000, 500}, {1000, 1000}, {1000, 1500}, {1000, 2000}, {1500, 0}, {1500, 500}, {1500, 1000}, {1500, 1500}, {1500, 2000}, {2000, 0}, {2000, 500}, {2000, 1000}, {2000, 1500}, {2000, 2000}};
	int pos2[44][2] = {{0, 250}, {0, 500}, {0, 750}, {0, 1000}, {0, 1250}, {0, 1500}, {0, 1750}, {0, 2000}, {500, 0}, {500, 250}, {500, 500}, {500, 750}, {500, 1000}, {500, 1250}, {500, 1500}, {500, 1750}, {500, 2000}, {1000, 0}, {1000, 250}, {1000, 500}, {1000, 750}, {1000, 1000}, {1000, 1250}, {1000, 1500}, {1000, 1750}, {1000, 2000}, {1500, 0}, {1500, 250}, {1500, 500}, {1500, 750}, {1500, 1000}, {1500, 1250}, {1500, 1500}, {1500, 1750}, {1500, 2000}, {2000, 0}, {2000, 250}, {2000, 500}, {2000, 750}, {2000, 1000}, {2000, 1250}, {2000, 1500}, {2000, 1750}, {2000, 2000}};
	int pos3[62][2] = {{0, 250}, {0, 500}, {0, 750}, {0, 1000}, {0, 1250}, {0, 1500}, {0, 1750}, {0, 2000}, {500, 0}, {500, 250}, {500, 500}, {500, 750}, {500, 1000}, {500, 1250}, {500, 1500}, {500, 1750}, {500, 2000}, {750, 0}, {750, 250}, {750, 500}, {750, 750}, {750, 1000}, {750, 1250}, {750, 1500}, {750, 1750}, {750, 2000}, {1000, 0}, {1000, 250}, {1000, 500}, {1000, 750}, {1000, 1000}, {1000, 1250}, {1000, 1500}, {1000, 1750}, {1000, 2000}, {1250, 0}, {1250, 250}, {1250, 500}, {1250, 750}, {1250, 1000}, {1250, 1250}, {1250, 1500}, {1250, 1750}, {1250, 2000}, {1500, 0}, {1500, 250}, {1500, 500}, {1500, 750}, {1500, 1000}, {1500, 1250}, {1500, 1500}, {1500, 1750}, {1500, 2000}, {2000, 0}, {2000, 250}, {2000, 500}, {2000, 750}, {2000, 1000}, {2000, 1250}, {2000, 1500}, {2000, 1750}, {2000, 2000}};
	int pos4[80][2] = {{0, 250}, {0, 500}, {0, 750}, {0, 1000}, {0, 1250}, {0, 1500}, {0, 1750}, {0, 2000}, {250, 0}, {250, 250}, {250, 500}, {250, 750}, {250, 1000}, {250, 1250}, {250, 1500}, {250, 1750}, {250, 2000}, {500, 0}, {500, 250}, {500, 500}, {500, 750}, {500, 1000}, {500, 1250}, {500, 1500}, {500, 1750}, {500, 2000}, {750, 0}, {750, 250}, {750, 500}, {750, 750}, {750, 1000}, {750, 1250}, {750, 1500}, {750, 1750}, {750, 2000}, {1000, 0}, {1000, 250}, {1000, 500}, {1000, 750}, {1000, 1000}, {1000, 1250}, {1000, 1500}, {1000, 1750}, {1000, 2000}, {1250, 0}, {1250, 250}, {1250, 500}, {1250, 750}, {1250, 1000}, {1250, 1250}, {1250, 1500}, {1250, 1750}, {1250, 2000}, {1500, 0}, {1500, 250}, {1500, 500}, {1500, 750}, {1500, 1000}, {1500, 1250}, {1500, 1500}, {1500, 1750}, {1500, 2000}, {1750, 0}, {1750, 250}, {1750, 500}, {1750, 750}, {1750, 1000}, {1750, 1250}, {1750, 1500}, {1750, 1750}, {1750, 2000}, {2000, 0}, {2000, 250}, {2000, 500}, {2000, 750}, {2000, 1000}, {2000, 1250}, {2000, 1500}, {2000, 1750}, {2000, 2000}};
	int index = 0;
	int k = 1;
	int level = 1;
	if (nodes == 45)
	{
		for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End();
			 i++)
		{
			// sleep(5);
			// std::cout<<*i<<"\n";
			Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
			position->Add(boundry);
			devices.Add(asHelper.Create(*i, newDevice));
			newDevice->GetPhy()->SetTransRange(range);
			NS_LOG_DEBUG(
				"Node:" << newDevice->GetAddress() << " position(x):" << boundry.x << "  position(y):" << boundry.y << "  position(z):" << boundry.z);
			/*std::cout<<"newDevice Address:"<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<".position.x:"
			<<boundry.x<<".position.y:"<<boundry.y<<".position.z:"<<boundry.z<<"\n";*/
			outfile << AquaSimAddress::ConvertFrom(newDevice->GetAddress()) << " " << boundry.x << " " << boundry.y << " " << boundry.z << "\n";
			boundry.x = pos[index][0];
			boundry.y = pos[index][1];
			boundry.z = 700 * level;
			index++;
			if (index == 8)
			{
				index = 0;
			}
			k++;
			if (k == 9 || k == 18 || k == 47 || k == 36 || k == 45)
			{
				level++;
			}
		}
	}
	else if (nodes == 125)
	{
		for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End();
			 i++)
		{
			// sleep(5);
			// std::cout<<*i<<"\n";
			Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
			position->Add(boundry);
			devices.Add(asHelper.Create(*i, newDevice));
			newDevice->GetPhy()->SetTransRange(range);
			NS_LOG_DEBUG(
				"Node:" << newDevice->GetAddress() << " position(x):" << boundry.x << "  position(y):" << boundry.y << "  position(z):" << boundry.z);
			/*std::cout<<"newDevice Address:"<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<".position.x:"
			<<boundry.x<<".position.y:"<<boundry.y<<".position.z:"<<boundry.z<<"\n";*/
			outfile << AquaSimAddress::ConvertFrom(newDevice->GetAddress()) << " " << boundry.x << " " << boundry.y << " " << boundry.z << "\n";
			boundry.x = pos1[index][0];
			boundry.y = pos1[index][1];
			boundry.z = 700 * level;
			index++;
			if (index == 24)
			{
				index = 0;
			}
			k++;
			if (k == 25 || k == 50 || k == 75 || k == 100 || k == 125)
			{
				level++;
			}
		}
	}
	else if (nodes == 225)
	{
		for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End();
			 i++)
		{
			// sleep(5);
			// std::cout<<*i<<"\n";
			Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
			position->Add(boundry);
			devices.Add(asHelper.Create(*i, newDevice));
			newDevice->GetPhy()->SetTransRange(range);
			NS_LOG_DEBUG(
				"Node:" << newDevice->GetAddress() << " position(x):" << boundry.x << "  position(y):" << boundry.y << "  position(z):" << boundry.z);
			/*std::cout<<"newDevice Address:"<<AquaSimAddress::ConvertFrom(newDevice->GetAddress())<<".position.x:"
			<<boundry.x<<".position.y:"<<boundry.y<<".position.z:"<<boundry.z<<"\n";*/
			outfile << AquaSimAddress::ConvertFrom(newDevice->GetAddress()) << " " << boundry.x << " " << boundry.y << " " << boundry.z << "\n";
			boundry.x = pos2[index][0];
			boundry.y = pos2[index][1];
			boundry.z = 700 * level;
			index++;
			if (index == 44)
			{
				index = 0;
			}
			k++;
			if (k == 45 || k == 90 || k == 135 || k == 180 || k == 225)
			{
				level++;
			}
		}
	}
	// sink node create
	for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
	{
		Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
		// todo 修改目标节点坐标
		position->Add(Vector(1000, 1000, 4200));
		// position->Add(Vector(1000,1500,4200));
		devices.Add(asHelper.Create(*i, newDevice));
		newDevice->GetPhy()->SetTransRange(range);
		/*std::cout<<"sink node Add:"<<newDevice->GetAddress()/*<<",getposition:"<<newDevice->GetPosition().x
		<<","<<newDevice->GetPosition().y<<","<<newDevice->GetPosition().z<<"\n";*/
		outfile << AquaSimAddress::ConvertFrom(newDevice->GetAddress()) << " " << 1000 << " " << 1500 << " " << 4200 << "\n";
	}

	outfile.close();
	std::cout << "Creating Nodes End\n";

	mobility.SetPositionAllocator(position);
	// 移动模型,以后有机会再考虑移动的
	/*mobility.SetMobilityModel("ns3::RandomWalk3dMobilityModel", "Mode",
							  StringValue("Time"), "Time", StringValue("2s"), "Speed",
							  //StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"), "Bounds",
							  StringValue("0|2100|0|2100"));*/
	mobility.SetMobilityModel("ns3::UnderwaterGilderMobilityModel2",
							  "Speed", StringValue("ns3::UniformRandomVariable[Min=2.0|Max=3.0]"),
							  "Bounds", StringValue("0|5000|0|5000"));
	mobility.Install(senderCon);
	mobility.Install(nodesCon);
	mobility.Install(sinksCon);
	// PacketSocketAddress socket;
	AquaSimSocketAddress socket;
	socket.SetAllDevices();
	if (nodes == 45)
	{
		socket.SetDestinationAddress(devices.Get(46)->GetAddress()); // Set dest to first sink (nodes+1 device)
	}
	else if (nodes == 125)
	{
		socket.SetDestinationAddress(devices.Get(126)->GetAddress()); // Set dest to first sink (nodes+1 device)
	}
	else if (nodes == 225)
	{
		socket.SetDestinationAddress(devices.Get(226)->GetAddress()); // Set dest to first sink (nodes+1 device)
	}
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

	for (int i = 0; i < 17000; i++)
	{
		Simulator::Schedule(Seconds((double)i), &DectConnection, nodesCon);
		Simulator::Schedule(Seconds((double)i), &DectConnection, sinksCon);
		Simulator::Schedule(Seconds((double)i), &DectConnection, senderCon);
		// Simulator::Schedule(Seconds((double)i), &printallNodePosition2, senderCon);
		// Simulator::Schedule(Seconds((double)i), &printallNodePosition2, nodesCon);
		// Simulator::Schedule(Seconds((double)i), &printallNodePosition2, sinksCon);
		//   Simulator::Schedule(Seconds((double)i), &judgecloseenough);
	}

	Packet::EnablePrinting(); // for debugging purposes
	std::cout << "-----------Running Simulation-----------\n";
	Simulator::Stop(Seconds(simStop));
	Simulator::Run();
	Simulator::Destroy();

	std::cout << "fin.\n";
	return 0;
}