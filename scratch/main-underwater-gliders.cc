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

using namespace ns3;

static void 
CourseChange (std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition ();
  Vector vel = mobility->GetVelocity ();
  // std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
  //           << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
  //           << ", z=" << vel.z << std::endl;
}

int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::UnderwaterGilderMobilityModel::Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"));
  Config::SetDefault ("ns3::UnderwaterGilderMobilityModel::Depth", StringValue ("ns3::UniformRandomVariable[Min=100.0|Max=300.0]"));
  Config::SetDefault ("ns3::UnderwaterGilderMobilityModel::OpenAngle", StringValue ("ns3::UniformRandomVariable[Min=30.0|Max=50.0]"));
  Config::SetDefault ("ns3::UnderwaterGilderMobilityModel::Bounds", StringValue ("0|5000|0|5000"));
  
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  LogComponentEnable("UnderwaterGilderMobilityModel", LOG_LEVEL_INFO);

  NodeContainer c;
  c.Create (1);
  NodeContainer b;
  b.Create(1);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.SetMobilityModel ("ns3::UnderwaterGilderMobilityModel", 
                             "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),
							 "Depth", StringValue("ns3::UniformRandomVariable[Min=500.0|Max=1000.0]"),
							 "OpenAngle", StringValue("ns3::UniformRandomVariable[Min=30.0|Max=50.0]"),
							 "Bounds", StringValue("0|5000|0|5000"));
  mobility.InstallAll ();
  //mobility.Install(c);

  MobilityHelper mobility1;
  mobility1.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility1.SetMobilityModel ("ns3::UnderwaterGilderMobilityModel", 
                             "Speed", StringValue("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),
							 "Depth", StringValue("ns3::UniformRandomVariable[Min=500.0|Max=1000.0]"),
							 "OpenAngle", StringValue("ns3::UniformRandomVariable[Min=30.0|Max=50.0]"),
							 "Bounds", StringValue("0|5000|0|5000"));
  mobility1.Install(b);

  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeCallback (&CourseChange));

  Simulator::Stop (Seconds (1000.0));

  AnimationInterface anim("test.xml");
  // anim.SetMobilityPollInterval(Seconds(0.1));

  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
