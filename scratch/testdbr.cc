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
#include "ns3/aqua-sim-mobility-ug.h"
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

static void
CourseChange(std::string foo, Ptr<const MobilityModel> mobility)
{
    Vector pos = mobility->GetPosition();
    Vector vel = mobility->GetVelocity();
    // std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
    //           << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
    //           << ", z=" << vel.z << std::endl;
}

NS_LOG_COMPONENT_DEFINE("QDTR");

int main(int argc, char *argv[])
{
    Config::SetDefault("ns3::UnderwaterGilderMobilityModel::Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"));
    Config::SetDefault("ns3::UnderwaterGilderMobilityModel::Depth", StringValue("ns3::UniformRandomVariable[Min=100.0|Max=300.0]"));
    Config::SetDefault("ns3::UnderwaterGilderMobilityModel::OpenAngle", StringValue("ns3::UniformRandomVariable[Min=30.0|Max=50.0]"));
    Config::SetDefault("ns3::UnderwaterGilderMobilityModel::Bounds", StringValue("0|5000|0|5000"));

    double simStop = 17000; // seconds
    int nodes = 25;         // pos
    // int nodes=125;//pos1
    // int nodes=225;//pos2
    // int nodes=315;//pos3
    // int nodes=360;//pos4
    int sinks = 1;  // sink node num
    int source = 1; // source node num
    uint32_t m_dataRate = 80;
    uint32_t m_packetSize = 32;
    double range = 1500;

    std::ofstream outfile("info.txt", std::ios::app);

    LogComponentEnable("AquaSimAloha", LOG_LEVEL_DEBUG);
    LogComponentEnable("AquaSimPhyCmn", LOG_LEVEL_DEBUG);
    LogComponentEnable("UnderwaterGilderMobilityModel", LOG_LEVEL_ERROR);
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
    asHelper.SetRouting("ns3::AquaSimDBR");
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
    if (nodes == 25)
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
            if (k == 9 || k == 18 || k == 27 || k == 36 || k == 45)
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
            if (index == 24)
            {
                index = 0;
            }
            k++;
            if (k == 25 || k == 90 || k == 135 || k == 180 || k == 225)
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

    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                  "X", StringValue("1000.0"),
                                  "Y", StringValue("1000.0"),
                                  "Rho", StringValue("ns3::UniformRandomVariable[Min=500|Max=1000]"));
    mobility.SetMobilityModel("ns3::UnderwaterGilderMobilityModel",
                              "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),
                              "Depth", StringValue("ns3::UniformRandomVariable[Min=4500.0|Max=5000.0]"),
                              "OpenAngle", StringValue("ns3::UniformRandomVariable[Min=30.0|Max=50.0]"),
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
    if (nodes == 25)
    {
        socket.SetDestinationAddress(devices.Get(26)->GetAddress()); // Set dest to first sink (nodes+1 device)
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

    // Simulator::Stop(Seconds(1000.0));

    Packet::EnablePrinting(); // for debugging purposes
    std::cout << "-----------Running Simulation-----------\n";
    Simulator::Stop(Seconds(simStop));
    AnimationInterface anim("dbr.xml");
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "fin.\n";
    return 0;
}