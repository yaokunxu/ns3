/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 ****
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
 */

/* WARNING: This file has not been tested. */
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/aqua-sim-mobility-ug2.h"

using namespace ns3;

Vector allNodePosition[18000][28];

void printallNodePosition(NodeContainer node)
{
    int t = Simulator::Now().GetSeconds();
    for (int i = 0; i < 27; i++)
    {
        std::cout << "first loop"
                  << "t:" << t << " id:" << i << " allNodePos:" << allNodePosition[t][i] << std::endl;
    }
    for (NodeContainer::Iterator i = node.Begin(); i != node.End(); ++i)
    {
        Ptr<Object> object = *i;
        int id = (*i)->GetId();
        std::cout << "second loop"
                  << "t:" << t << " id:" << id << " allNodePos:" << allNodePosition[t][id] << std::endl;
    }
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
            allNodePosition[t + i + 1][id] = nextPos;
            // std::cout << "position: " << position << " velocity: " << velocity << " nextPos:" << nextPos << " allNodePosition:" << allNodePosition[t + i + 1][id] << std::endl;

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
    std::cout << "start" << std::endl;
    std::cout << Simulator::Now().GetSeconds() << std::endl;
    std::cout << "GetN " << node.GetN() << std::endl;

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
            allNodePosition[0][id] = pos;
        }
        if ((int)pos.x == (int)allNodePosition[t][id].x&&(int)pos.y == (int)allNodePosition[t][id].y&&(int)pos.z == (int)allNodePosition[t][id].z)
        {
            std::cout << "YE,t:" << t << " id:" << id << " pos:" << pos << " allNodePosition:" << allNodePosition[t][id] << std::endl;
        }
        else
        {
            std::cout << "NE,t:" << t << " id:" << id << " pos:" << pos << " allNodePosition:" << allNodePosition[t][id] << std::endl;
            std::cout << "distance:" << (pos - allNodePosition[t][id]).GetLength() << std::endl;
        }
        // std::cout << "pos:" << pos << "allNodePosition:" << allNodePosition[t][id] << std::endl;
        allNodePosition[t][id] = pos;
        // if (t != 0)
        //{
        //	std::cout << allNodePosition[t - 1][id] << std::endl;
        // }
        // std::cout << allNodePosition[t][id] << std::endl;
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
    Config::SetDefault("ns3::UnderwaterGilderMobilityModel2::Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"));
    Config::SetDefault("ns3::UnderwaterGilderMobilityModel2::Bounds", StringValue("0|5000|0|5000"));

    CommandLine cmd;
    cmd.Parse(argc, argv);
    int simStop = 17000;

    LogComponentEnable("UnderwaterGilderMobilityModel2", LOG_LEVEL_INFO);

    NodeContainer c;
    c.Create(27);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                  "X", StringValue("100.0"),
                                  "Y", StringValue("100.0"),
                                  "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));
    mobility.SetMobilityModel("ns3::UnderwaterGilderMobilityModel2",
                              "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),
                              "Bounds", StringValue("0|5000|0|5000"));
    mobility.Install(c);

    Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
                    MakeCallback(&CourseChange));

    AnimationInterface anim("test2.xml");
    // anim.SetMobilityPollInterval(Seconds(0.1));
    for (int i = 0; i < 17000; i++)
    {
        Simulator::Schedule(Seconds((double)i), &DectConnection, c);
        Simulator::Schedule(Seconds((double)i), &printallNodePosition, c);
    }
    Simulator::Stop(Seconds(simStop));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
