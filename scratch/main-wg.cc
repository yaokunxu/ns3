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
//   std::cout << Simulator::Now().GetSeconds() << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
//             << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
//             << ", z=" << vel.z << std::endl;
}

int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::WaveGilderMobilityModel::Bounds", StringValue ("0|5000|0|5000"));
  
  CommandLine cmd;
  cmd.Parse (argc, argv);

 // LogComponentEnable("ConstantPalstanceHelper", LOG_LEVEL_DEBUG);
 //  LogComponentEnable("WaveGilderMobilityModel", LOG_LEVEL_INFO);
//   LogComponentEnable("UVCmdContainer", LOG_LEVEL_DEBUG);

  NodeContainer c;
  c.Create (1);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("0.0"),
                                 "Y", StringValue ("0.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.SetMobilityModel ("ns3::WaveGilderMobilityModel", 
							 "Bounds", StringValue("0|5000|0|5000"));
  mobility.InstallAll ();
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeCallback (&CourseChange));

  Simulator::Stop (Seconds (500));

  // AnimationInterface anim("test.xml");

  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
