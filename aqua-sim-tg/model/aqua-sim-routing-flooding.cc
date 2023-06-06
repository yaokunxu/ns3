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

#include "aqua-sim-routing-flooding.h"
#include "aqua-sim-address.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-trailer.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "Logger.h"
#include "aqua-sim-header-transport.h"
//#include "underwatersensor/uw_common/uw_hash_table.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimFloodingRouting");
NS_OBJECT_ENSURE_REGISTERED(AquaSimFloodingRouting);


AquaSimFloodingRouting::AquaSimFloodingRouting()
{
  // Initialize variables.
  //  printf("VB initialized\n");
  m_pkCount = 0;
}

TypeId
AquaSimFloodingRouting::GetTypeId()
{
  static TypeId tid = TypeId ("ns3::AquaSimFloodingRouting")
    .SetParent<AquaSimRouting> ()
    .AddConstructor<AquaSimFloodingRouting> ()
  ;
  return tid;
}

int64_t
AquaSimFloodingRouting::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  return 0;
}
//传输层下来的调用txprocess
bool AquaSimFloodingRouting::TxProcess(Ptr< Packet > packet, const Address &dest, uint16_t protocolNumber)
{
  VBHeader vbh;
  
  vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
	packet->AddHeader(vbh);
//send time add to Nodedelay.txt
  if(packet->GetUid()>100){
            std::ofstream outfile("Nodedelay.txt", std::ios::app);
			double curtime=Simulator::Now().GetSeconds();
			outfile<<"pktID:"<<packet->GetUid()<<" sendTime "<<curtime<<"\n";
			outfile.close();
	}
	return Recv(packet, dest, protocolNumber);
}
//MAV层上来的包调用Recv
bool
AquaSimFloodingRouting::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  std::cout<<"=========AquaSimFloodingRouting::Recv==========\n";
  NS_LOG_FUNCTION(this << packet << GetNetDevice()->GetAddress());
  std::cout<<"Recv packet NetDevice->getaddress:"<<GetNetDevice()->GetAddress()<<"\n";
  LOG(WARNING)<<"success";
  std::ofstream outfile("NodeResiEnergy.txt", std::ios::app);
	outfile<</*"PktID:"<<pktID<<*/"Node:"<<AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())/*<<"  ResiEnergy:"*/<<" "<<GetNetDevice()->EnergyModel()->GetEnergy()<<"\n";
	
//todo
  AquaSimTrailer tra;
  VBHeader vbh;
  packet->RemoveTrailer(tra);
  packet->RemoveHeader(vbh);  //Han
  std::cout<<"Recv MessType:"<<vbh.GetMessType()<<"\n";
 // if (!vbh.GetMessType())  //no headers
  if(vbh.GetMessType()!=AS_DATA)  //Han
  {
	std::cout<<"AquaSimFloodingRouting::Recv no headers.Sender's app->routing\n";
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetPkNum(packet->GetUid());
    vbh.SetMessType(AS_DATA);
    vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));

    tra.SetNextHop(AquaSimAddress::GetBroadcast());
    tra.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    tra.SetDAddr(AquaSimAddress::ConvertFrom(dest));

     /*std::cout<<"vbh info :"<<vbh.GetSenderAddr()<<vbh.GetTargetAddr()<<"\n";
     std::cout<<"tra info :"<<tra.GetSAddr()<<tra.GetDAddr()<<"\n";*/
    packet->AddHeader(vbh);
    packet->AddTrailer(tra);
  }
  else
  {  //Han
	//TODO
 
    //ash.SetNumForwards(ash.GetNumForwards() + 1);
    //TODO
    packet->AddHeader(vbh);
    packet->AddTrailer(tra);

  }
 

	// Packet Hash Table is used to keep info about experienced pkts.
  vbf_neighborhood *hashPtr= PktTable.GetHash(vbh.GetSenderAddr(), packet->GetUid());
	// Received this packet before ?

	if (hashPtr != NULL) {
	std::cout<<"Recv hashPtr!=null\n";
    packet=0;
    return false;
  }
	else {
		PktTable.PutInHash(vbh.GetSenderAddr(), packet->GetUid());
		// Take action for a new pkt.
		ConsiderNew(packet);
    return true;
	}
}

void
AquaSimFloodingRouting::ConsiderNew(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << pkt);
 // LOG(WARNING)<<"SUCCCESS";
  VBHeader vbh;
  AquaSimTrailer tra;

  pkt->PeekHeader(vbh);
  //std::cout<<"Considernew MessType:"<<vbh.GetMessType()<<"\n";

	AquaSimAddress nodeAddr; //, forward_nodeID, target_nodeID; //not used...


	switch (vbh.GetMessType()) {
	case AS_DATA:
		std::cout<<"Recv MessType AS_Data:SenderAddr::"<<vbh.GetSenderAddr()<<"\n";
		//    printf("uwflooding(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);
		nodeAddr = vbh.GetSenderAddr();
    
		if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == nodeAddr)
    {
			// come from the same node, briadcast it
      LOG(WARNING)<<"from the same node broadcast it";
			MACprepare(pkt);
			MACsend(pkt,Seconds(0));
			return;
		}


		if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())==vbh.GetTargetAddr())
		{
			//	printf("uwflooding: %d is the target\n", here_.addr_);
     // LOG(WARNING)<<"dataforsink";
     if(pkt->GetUid()>100){
                std::ofstream delayT("Nodedelay.txt", std::ios::app);
			    //delayT<<"pktID:"<<pkt->GetUid()<<" RecvTime "<<Simulator::Now().GetSeconds()<<"\n";
				delayT<<"pktID:"<<pkt->GetUid()<<" RecvTime "<<Simulator::Now().GetSeconds()<<"\n";
			    delayT.close();
	}
			DataForSink(pkt); // process it
		}

		else{
			// printf("uwflooding: %d is the not  target\n", here_.addr_);
			MACprepare(pkt);
			MACsend(pkt, Seconds(0));
		}
		return;

	default:
    pkt=0;
    break;
	}
}


void
AquaSimFloodingRouting::Reset()
{

	PktTable.Reset();
	/*
	   for (int i=0; i<MAX_DATA_TYPE; i++) {
	   routing_table[i].reset();
	   }
	 */
}


void
AquaSimFloodingRouting::Terminate()
{
  NS_LOG_DEBUG("Terminate: Node=" << m_device->GetAddress() <<
        ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<
        ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
}

void
AquaSimFloodingRouting::StopSource()
{
	/*
	   Agent_List *cur;

	   for (int i=0; i<MAX_DATA_TYPE; i++) {
	   for (cur=routing_table[i].source; cur!=NULL; cur=AGENT_NEXT(cur) ) {
	    SEND_MESSAGE(i, AGT_ADDR(cur), DATA_STOP);
	   }
	   }
	 */
}


Ptr<Packet>
AquaSimFloodingRouting::CreatePacket()
{
	Ptr<Packet> pkt = Create<Packet>();

	if (pkt==NULL) return NULL;


  AquaSimTrailer tra;
  //ash.size(36);

  VBHeader vbh;
  vbh.SetTs(Simulator::Now().ToDouble(Time::S));

  pkt->AddHeader(vbh);
  pkt->AddTrailer(tra);
	return pkt;
}

Ptr<Packet>
AquaSimFloodingRouting::PrepareMessage(unsigned int dtype, AquaSimAddress addr,  int msg_type)
{
	Ptr<Packet> pkt = Create<Packet>();
  VBHeader vbh;
  AquaSimTrailer tra;
	//hdr_ip *iph;

	vbh.SetMessType(msg_type);
	vbh.SetPkNum(m_pkCount);
	m_pkCount++;
	vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	// vbh.SetDataType(dtype);
	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));

	vbh.SetTs(Simulator::Now().ToDouble(Time::S));

  pkt->AddHeader(vbh);
  pkt->AddTrailer(tra);
	return pkt;
}

void
AquaSimFloodingRouting::MACprepare(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);
  LOG(WARNING)<<"MACprepare";
  VBHeader vbh;
  AquaSimTrailer tra;

  pkt->RemoveHeader(vbh);
  pkt->RemoveTrailer(tra);
	// hdr_ip*  iph = HDR_IP(pkt); // I am not sure if we need it

	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));

  tra.SetNextHop(AquaSimAddress::GetBroadcast());
	

  pkt->AddHeader(vbh);
  pkt->AddTrailer(tra);
  
}

void
AquaSimFloodingRouting::MACsend(Ptr<Packet> pkt, Time delay)
{
  NS_LOG_FUNCTION(this);

  VBHeader vbh;
  AquaSimTrailer tra;

  pkt->PeekHeader(vbh);
  pkt->RemoveTrailer(tra);
	if (vbh.GetMessType() == AS_DATA)

  std::cout<<" asdata"<<std::endl;
	else
	//	ash.SetSize(36);
   std::cout<<" no asdata"<<std::endl;
  //pkt->AddHeader(ash);
  pkt->AddTrailer(tra);
 
  Simulator::Schedule(delay, &AquaSimRouting::SendDown,this,
                        pkt,AquaSimAddress::GetBroadcast(),Seconds(0));
}

void
AquaSimFloodingRouting::DataForSink(Ptr<Packet> pkt)
{
	//  printf("DataforSink: the packet is send to demux\n");
	NS_LOG_FUNCTION(this << pkt << "Sending up to dmux.");
  //LOG(INFO)<<"dataforsink";
  VBHeader vbh;
  pkt->RemoveHeader(vbh);
  SendUp(pkt);
	/*if (!SendUp(pkt))
		NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
    LOG(WARNING)<<"DataForSink: Something went wrong when passing packet up to dmux";*/
}

void
AquaSimFloodingRouting::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimRouting::DoDispose();
}

// Some methods for Flooding Entry
