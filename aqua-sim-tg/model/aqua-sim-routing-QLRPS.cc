/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 JinLin University
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
 *
 *
 * Author: HanCheng <827569146@qq.com>
 */

#include "aqua-sim-routing-QLRPS.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-propagation.h"
#include "aqua-sim-trailer.h"
#include "aqua-sim-datastructure.h"
#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <time.h>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimQLRPS");
// NS_OBJECT_ENSURE_REGISTERED(AquaSimPktHashTable);
// todo add timeer
// clock_t start,finish;
AquaSimPktlocalTable::AquaSimPktlocalTable()
{
	NS_LOG_FUNCTION(this);
	m_windowSize = WINDOW_SIZE;
}
AquaSimPktlocalTable::~AquaSimPktlocalTable()
{
	NS_LOG_FUNCTION(this);
	for (std::map<local_entry, qlrp_neighbor *>::iterator it = m_htable.begin(); it != m_htable.end(); ++it)
	{
		delete it->second;
	}
	m_htable.clear();
}
void AquaSimPktlocalTable::Reset()
{
	m_htable.clear();
}
qlrp_neighbor *
AquaSimPktlocalTable::GetHash(AquaSimAddress forwarderAddr, Vector p)
{
	// std::cout<<"++++++GetHash+++++++++++++\n";
	local_entry entry = std::make_pair(forwarderAddr, p);

	std::map<local_entry, qlrp_neighbor *>::iterator it;

	it = m_htable.find(entry);
	if (it == m_htable.end())
		return NULL;
	return it->second;
}
void AquaSimPktlocalTable::PutInHash(AquaSimAddress fAddr, double Vvalue, Vector p, unsigned int Energy, unsigned int Avgenergy, unsigned int Density, unsigned int sendnum, unsigned int sendsucc)
{
	NS_LOG_DEBUG("PutinHash begin:" << fAddr << "," << Vvalue << ",(" << p.x << "," << p.y << "," << p.z << ")");
	// std::cout<<"   Putinhash++++++++++++++    \n ";
	// std::cout<<"PutinHash begin:" << fAddr << "," << Vvalue << ",(" << p.x << "," << p.y << "," << p.z << ")"<<"\n";
	local_entry entry = std::make_pair(fAddr, p);
	qlrp_neighbor *hashptr;
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	// todo //表中已有该节点信息，更新,(因为位置包含在map的key值中，所以只能删除原来的再加入新的)
	for (it = m_htable.begin(); it != m_htable.end(); it++)
	{
		local_entry cur = it->first;
		qlrp_neighbor *nei = it->second;
		if (cur.first == fAddr)
		{
			// std::cout<<"hasptr exit,update\n";
			nei->energy = Energy;
			nei->avgenergy = Avgenergy;
			nei->density = Density;
			// update Vvalue after recv a packet?
			nei->value = Vvalue;
			// update location
			cur.second = p;
			std::cout << "insert location:" << cur.second.x << " " << cur.second.y << " " << cur.second.z << std::endl;
			return;
		}
	}
	// 以后有机会再考虑移动的
	/*for(it=m_htable.begin();it!=m_htable.end();it++){
		local_entry cur=it->first;
		qlrp_neighbor *nei=it->second;
		if(cur.first==fAddr){
		   //std::cout<<"hasptr exit,update\n";
		  unsigned int sndnum=nei->sendnum;
		  unsigned int sndsucc=nei->sendsucc;
		   std::map<local_entry,qlrp_neighbor*>::iterator iter;
		   iter=m_htable.erase(it);
		   it=iter;
		   PutInHash(fAddr,Vvalue,p,Energy,Avgenergy,Density,sndnum,sndsucc);
		   return;
		}
	}*/

	hashptr = new qlrp_neighbor[1];
	hashptr[0].energy = Energy;
	hashptr[0].value = Vvalue;
	hashptr[0].avgenergy = Avgenergy;
	hashptr[0].density = Density;
	hashptr[0].sendnum = 0;
	hashptr[0].sendsucc = 0;
	std::pair<local_entry, qlrp_neighbor *> newPair;
	newPair.first = entry;
	newPair.second = hashptr;
	// std::cout<<"position:"<<entry.first<<"\n";
	//  std::cout<<m_htable.size()<<"......before insert.......\n";
	m_htable.insert(newPair);
	// std::cout<<m_htable.size()<<"......after insert.......\n";
	/*for(it=m_htable.begin();it!=m_htable.end();it++){
		qlrp_neighbor *h=it->second;
		local_entry add=it->first;
		std::cout<<"insert Addr:"<<add.first<<"\n";
		std::cout<<"insert energy:"<<h->energy<<",insert value:"<<h->value<<", insert density: "<<h->density<<"......after insert.......\n";
	}*/
	//}
}

void AquaSimPktlocalTable::UpdateHash(AquaSimAddress local, AquaSimAddress nexthop, double Vvalue)
{ // 更新V值和Sendnum
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	for (it = m_htable.begin(); it != m_htable.end(); it++)
	{
		local_entry update = it->first;
		qlrp_neighbor *neibor = it->second;
		if (update.first == local)
		{
			neibor->value = Vvalue;
			// std::cout<<"beibor->Vvalue:"<<neibor->value<<", Vvalue:"<<Vvalue<<"  ,Update vvalue successfully\n";
		}
		if (update.first == nexthop)
		{
			// neibor->value=Vvalue;
			neibor->sendnum++;
			// std::cout<<"sendnum:"<<neibor->sendnum<<"  ,Update Sendnum successfully\n";
		}
	}
}

void AquaSimPktlocalTable::UpdateSuccnum(AquaSimAddress previous, AquaSimAddress forward)
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	for (it = m_htable.begin(); it != m_htable.end(); it++)
	{
		local_entry update = it->first;
		qlrp_neighbor *neibor = it->second;
		if (update.first == forward)
		{
			neibor->sendsucc++;
		}
	}
	std::cout << "Previousnode:" << previous << " recv the ACK from node:" << forward << " Update Sendsucc successfully\n";
}

void AquaSimPktlocalTable::updateOwn(double avgengy, double denty, AquaSimAddress cur)
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	for (it = m_htable.begin(); it != m_htable.end(); it++)
	{
		local_entry update = it->first;
		qlrp_neighbor *neibor = it->second;
		if (update.first == cur)
		{
			neibor->avgenergy = avgengy;
			neibor->density = denty;
		}
	}
	// std::cout<<"average:"<<avgengy<<" ,density: "<<denty<<",Update density avgengy successfully\n";
}

AquaSimDataHashTableS::AquaSimDataHashTableS()
{
	NS_LOG_FUNCTION(this);
	Reset();
	// Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
}

AquaSimDataHashTableS::~AquaSimDataHashTableS()
{
	NS_LOG_FUNCTION(this);
	Reset();
}

void AquaSimDataHashTableS::Reset()
{
	for (std::map<int *, int *>::iterator it = m_htable.begin(); it != m_htable.end(); ++it)
	{
		delete it->first;
		delete it->second;
	}
	m_htable.clear();
	/*
	  Tcl_HashEntry *entryPtr;
	  Tcl_HashSearch searchPtr;

	  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
	  while (entryPtr != NULL) {
		  Tcl_DeleteHashEntry(entryPtr);
		  entryPtr = Tcl_NextHashEntry(&searchPtr);
	  }
	*/
}

int *AquaSimDataHashTableS::GetHash(int *attr)
{
	std::map<int *, int *>::iterator it;
	it = m_htable.find(attr);
	if (it == m_htable.end())
		return NULL;

	return it->second;
}

void AquaSimDataHashTableS::PutInHash(int *attr)
{
	// bool newPtr = true;

	// Tcl_HashEntry* entryPtr=Tcl_CreateHashEntry(&htable, (char *)attr, &newPtr);
	if (m_htable.count(attr) > 0)
		return;

	int *hashPtr = new int[1];
	hashPtr[0] = 1;
	m_htable.insert(std::pair<int *, int *>(attr, hashPtr));
	// Tcl_SetHashValue(entryPtr, hashPtr);
}

NS_OBJECT_ENSURE_REGISTERED(AquaSimQLRPS);

AquaSimQLRPS::AquaSimQLRPS()
{
	// Initialize variables.
	//  printf("VB initialized\n");
	m_pkCount = 0;
	// TODO
	m_width = 4000;
	// m_width=0;
	m_counter = 0;
	m_priority = 1.5;
	a1 = 0.4103;
	a2 = 0.0884;
	a3 = 0.1021;
	a4 = 0.3987;
	b1 = 0.4103;
	b2 = 0.0884;
	b3 = 0.1021;
	b4 = 0.3987;
	g = 1;
	theta = 0.5;
	// m_priority=2;
	// m_useOverhear = 0;
	m_enableRouting = 1;
	m_targetPos = Vector();
	m_rand = CreateObject<UniformRandomVariable>();
	// todo定期调用 创建包发包//初始化本地表
	/// 延迟一段时间构造，因为可能节点信息还没有创建
	/*Simulator::Schedule(Seconds(0.5),
				 &AquaSimQLRPS::iniNodetable, this);*/
	Simulator::Schedule(Seconds(0.5),
						&AquaSimQLRPS::iniNodetable1, this);
	/*Simulator::Schedule(Seconds(10),
				  &AquaSimQLRPS::PrepareMessage, this);*/
}

TypeId
AquaSimQLRPS::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::AquaSimQLRPS")
							.SetParent<AquaSimRouting>()
							.AddConstructor<AquaSimQLRPS>()
							.AddAttribute("HopByHop", "Hop by hop QLRP setting. Default 0 is false.",
										  IntegerValue(0),
										  MakeIntegerAccessor(&AquaSimQLRPS::m_hopByHop),
										  MakeIntegerChecker<int>())
							.AddAttribute("EnableRouting", "Enable routing QLRP setting. Default 1 is true.",
										  IntegerValue(1),
										  MakeIntegerAccessor(&AquaSimQLRPS::m_enableRouting),
										  MakeIntegerChecker<int>())
							.AddAttribute("Width", "Width of QLRPS. Default is 100.",
										  DoubleValue(3000),
										  // DoubleValue(100),
										  MakeDoubleAccessor(&AquaSimQLRPS::m_width),
										  MakeDoubleChecker<double>())
							.AddAttribute("TargetPos", "Position of target sink (x,y,z).",
										  Vector3DValue(),
										  MakeVector3DAccessor(&AquaSimQLRPS::m_targetPos),
										  MakeVector3DChecker());
	return tid;
	// bind("m_useOverhear_", &m_useOverhear);
}

int64_t
AquaSimQLRPS::AssignStreams(int64_t stream)
{
	NS_LOG_FUNCTION(this << stream);
	m_rand->SetStream(stream);
	return 1;
}
// 传输层调用Txprocess
bool AquaSimQLRPS::TxProcess(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
	QLRPSHeader vbh;
	// start=clock();
	vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
	packet->AddHeader(vbh);
	// todo add send DATA_READY packet
	// MACprepare(packet);
	// MACsend(packet,0);
	// send time add to Nodedelay.txt
	if (packet->GetUid() > 100)
	{
		std::ofstream outfile("Nodedelay.txt", std::ios::app);
		double curtime = Simulator::Now().GetSeconds();
		outfile << "pktID:" << packet->GetUid() << " sendTime " << curtime << "\n";
		outfile.close();
	}
	return Recv(packet, dest, protocolNumber);
}
// MAC层的包调用Recv
bool AquaSimQLRPS::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
	NS_LOG_FUNCTION(this);
	std::cout << "================qlrp Recv=================\n";
	std::cout << "Current node:" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) << "\n";
	// std::cout<<"Current node:"<<GetNetDevice()->GetAddress()<<"\n";
	// AquaSimEnergyModel en;
	// en.GetEnergy();
	// std::cout<<"qlrp energy "<<en.GetEnergy();
	QLRPSHeader vbh;
	AquaSimTrailer tra;

	packet->RemoveHeader(vbh);
	packet->RemoveTrailer(tra);
	if (vbh.GetMessType() == DATA_READY)
	{
		// 收到信息包，更新表
		std::cout << "recv message DATA_READY on QLRPS Recv\n";
		// 接到的包先将包信息插入(表中有信息则更新，没有就插入所有)
		// qlrp_neighbor *hash=PktlocalTable.GetHash(vbh.GetSenderAddr(),vbh.GetExtraInfo().f);
		// todo
		PktlocalTable.PutInHash(vbh.GetSenderAddr(), vbh.GetValue(), vbh.GetExtraInfo().f, vbh.GetResiEnergy(),
								vbh.GetAvgEnergy(), vbh.GetDensity(), 0, 0);
		std::cout << "DATA_READY  insert successfully---------------\n";

		return true;
	}

	if (vbh.GetMessType() != AS_DATA)
	{ // no headers //TODO create specalized Application instead of using this hack.// how many times this pkt was forwarded
		tra.SetNextHop(AquaSimAddress::GetBroadcast());
		tra.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		tra.SetDAddr(AquaSimAddress::ConvertFrom(dest));
		tra.SetModeId(1);
		vbh.SetMessType(AS_DATA);
		vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
		vbh.SetPkNum(packet->GetUid());
		// todo
		vbh.SetValue(getlocalV(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())));
		// vbh.SetNexthopAddr();
		vbh.SetResiEnergy(GetNetDevice()->EnergyModel()->GetEnergy());
		vbh.SetAvgEnergy(Avgenergy());
		vbh.SetDensity(PktlocalTable.m_htable.size());

		Ptr<Object> sObject = GetNetDevice()->GetNode();
		Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel>();

		/*AquaSimEnergyModel en;
		std::cout<<"-----------extra energy-----"<<" "<<GetNetDevice()->EnergyModel()->GetEnergy();*/
		// vbh.SetOriginalSource(sModel->GetPosition());
		vbh.SetExtraInfo_f(sModel->GetPosition());
		vbh.SetExtraInfo_t(m_targetPos);
		vbh.SetExtraInfo_o(sModel->GetPosition());
		// std::cout<<"source position:x:"<<sModel->GetPosition().x << " y:"<<sModel->GetPosition().y<<" z:"<<sModel->GetPosition().z<< std::endl;
		std::cout << "Recv-addheader\n";
		packet->AddHeader(vbh);
	}
	else
	{
		packet->AddHeader(vbh);
		packet->PeekHeader(vbh);
		// ignoring forward iterator, but this can be simply changed if necessary
	}

	packet->AddTrailer(tra);
	// unsigned char msg_type =vbh.GetMessType();  //unused
	// unsigned int dtype = vbh.GetDataType();  //unused
	// double t1=vbh.GetTs();  //unused

	if (!m_enableRouting)
	{
		if (vbh.GetMessType() != AS_DATA)
		{
			packet = 0;
			std::cout << "qlrp recv vbh.GetMessType() != AS_DATA vbh.GetMessType():" << vbh.GetMessType() << std::endl;
			return false;
		}
		std::cout << "vbh.GetTargetAddr():" << vbh.GetTargetAddr() << " GetNetDevice()->GetAddress()" << GetNetDevice()->GetAddress() << std::endl;

		if (vbh.GetSenderAddr() == GetNetDevice()->GetAddress())
		{

			MACprepare(packet);
			MACsend(packet, (m_rand->GetValue() * JITTER));
			std::cout << "Recv macsend\n";
		}
		else if (vbh.GetTargetAddr() == GetNetDevice()->GetAddress())
		{
			DataForSink(packet);
			std::cout << "Recv data for sink\n";
		}
		return true;
	}
	else
	{
		// 接到的数据包先将包信息插入(表中有信息则更新能量，位置信息,更新自己的avgenergy,没有就插入所有)
		qlrp_neighbor *hash = PktlocalTable.GetHash(vbh.GetForwardAddr(), vbh.GetExtraInfo().f);
		// todo
		PktlocalTable.PutInHash(vbh.GetForwardAddr(), vbh.GetValue(), vbh.GetExtraInfo().f, vbh.GetResiEnergy(),
								vbh.GetAvgEnergy(), vbh.GetDensity(), 0, 0);
		updateAvgEnergyAfterrecv(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		// 以后有机会再考虑移动的
		// updateDensity_delete_entry(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		// std::cout<<"insert successfully---------------\n";

		// 加入位置信息然后considerNew然后considerNew
		Ptr<Object> sObject = GetNetDevice()->GetNode();
		Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel>();
		// Todo
		// std::cout<<"position.x:"<<sModel->GetPosition().x<<"position.y:"<<sModel->GetPosition().y<<"position.z:"<<sModel->GetPosition().z<<std::endl;
		// todo end
		Vector forwarder = vbh.GetExtraInfo().f;
		// d:关于接收方与转发器的相对位置的信息
		packet->RemoveTrailer(tra);
		packet->RemoveHeader(vbh);
		Vector d = Vector(sModel->GetPosition().x - forwarder.x,
						  sModel->GetPosition().y - forwarder.y,
						  sModel->GetPosition().z - forwarder.z);
		vbh.SetExtraInfo_d(d);
		packet->AddHeader(vbh);
		packet->AddTrailer(tra);
		ConsiderNew(packet);
	}
	return true;
}

void AquaSimQLRPS::ConsiderNew(Ptr<Packet> pkt)
{
	std::cout << "===========qlrp: considerNew===========\n";
	NS_LOG_FUNCTION(this);

	QLRPSHeader vbh;
	AquaSimTrailer tra;
	pkt->RemoveTrailer(tra);
	pkt->PeekHeader(vbh);
	pkt->AddTrailer(tra);
	// Todo
	double tx = vbh.GetExtraInfo().t.x;
	double ty = vbh.GetExtraInfo().t.y;
	double tz = vbh.GetExtraInfo().t.z;
	// std::cout<<"consider new begin : Advance source position is:"<<vbh.GetExtraInfo().o.x<<","<<vbh.GetExtraInfo().o.y<<","<<vbh.GetExtraInfo().o.z<<'\n';
	// std::cout<<"consider new begin : Advance target position is:"<<tx<<","<<ty<<","<<tz<<'\n';
	Vector pos = GetNetDevice()->GetPosition();
	// std::cout<<"consider new begin : Advance current position is:"<<pos.x<<","<<pos.y<<","<<pos.z<<'\n';
	// todo end
	unsigned char msg_type = vbh.GetMessType();
	AquaSimAddress from_nodeAddr; //, forward_nodeAddr;zhuan fa qi
	std::cout << "Curnode:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " receive pkr from node:"
			  << vbh.GetForwardAddr() << ",packet id:" << pkt->GetUid() << "\n";
	// todo add hop to hop delay
	printHopbyhopRecvdelay(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), vbh.GetForwardAddr(), pkt->GetUid());
	NS_LOG_INFO("AquaSimQLRPS::ConsiderNew: data packet");
	// todo 打印每个节点的剩余能量
	printEnergy(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), pkt->GetUid());
	// from_nodeAddr = vbh.GetSenderAddr();
	from_nodeAddr = vbh.GetForwardAddr();
	// TODO
	// 判断Previous_node是否是自己，是就更新本地表Sendsucc字段（隐式确认）
	if (GetNetDevice()->GetAddress() == vbh.GetPrevioushop())
	{
		PktlocalTable.UpdateSuccnum(vbh.GetPrevioushop(), vbh.GetForwardAddr());
	}
	// 第一个节点发送
	if (GetNetDevice()->GetAddress() == from_nodeAddr)
	{
		// come from the same node, broadcast it
		// todo
		// while(m_pkCount<1){//源节点不断发包.m_pkCount<100//设置多久发一次schedule()
		m_pkCount++;
		std::cout << "Node::" << GetNetDevice()->GetAddress() << "  m_pkCount++::" << m_pkCount << ",,"
				  << "considerNew_MACsssend\n";
		// MACprepareF(pkt);
		Simulator::Schedule(Seconds(0.5),
							&AquaSimQLRPS::MACprepareF, this, pkt);
		// MACsend(pkt,0.5);//macsend比Macprepare提前执行了
		Simulator::Schedule(Seconds(0.5), &AquaSimQLRPS::MACsend, this, pkt, 0.5);
		// 发送完后更新自己的表中能量和平均能量值
		// todo
		// updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		Simulator::Schedule(Seconds(0.5), &AquaSimQLRPS::updateEnergyAfterSend, this,
							AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		// 把source节点的V值单独打印log
		Simulator::Schedule(Seconds(0.5),
							&AquaSimQLRPS::printSourceV, this, m_pkCount);
		// printSourceV();
		//}
		return;
	}
	if (GetNetDevice()->GetAddress() == vbh.GetTargetAddr()) // 接收节点
	{
		std::cout << "Node::" << GetNetDevice()->GetAddress() << "recv data for sink,packet id:" << pkt->GetUid()
				  << ",At time :" << Simulator::Now().GetSeconds() << "\n";
		double curtime = Simulator::Now().GetSeconds();
		updateAvgEnergyAfterrecv(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		std::ofstream outfile("packet.txt", std::ios::app);
		outfile << pkt->GetUid() << "\n";
		outfile.close();
		if (pkt->GetUid() > 100)
		{
			std::ofstream delayT("Nodedelay.txt", std::ios::app);
			// delayT<<"pktID:"<<pkt->GetUid()<<" RecvTime "<<Simulator::Now().GetSeconds()<<"\n";
			delayT << "pktID:" << pkt->GetUid() << " RecvTime " << curtime << "\n";
			delayT.close();
		}
		DataForSink(pkt); // process it
		// finish=clock();
		// double totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
		// std::cout<<"totaltime:"<<totaltime<<"\n";
		return;
	}

	// todo
	// 判断nexthop字段是不是自己
	if (GetNetDevice()->GetAddress() == vbh.GetNexthopAddr())
	{
		MACprepareF(pkt);
		MACsend(pkt, 0);
		// 发送完后更新自己的表中能量和平均能量值
		// todo
		updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		std::cout << "Node:: " << GetNetDevice()->GetAddress() << " is next hop,broadcast it\n";
		return;
	}
	else
	{
		// 判断nexthop字段不是自己,节点移动了,其他节点获取不到移动信息,可不可以在接受以后,如果nexthop不是自己,自己就广播自己的信息包
		// TODO
		pkt = 0;
		// MACprepare(pkt);
		std::cout << "Recv::  I am not nexthop! return!!!!!!!\n";
		return;
	}
}

void AquaSimQLRPS::Reset()
{
	PktlocalTable.Reset();
	/*
	   for (int i=0; i<MAX_DATA_TYPE; i++) {
	   routing_table[i].Reset();
	   }
	 */
}

void AquaSimQLRPS::Terminate()
{
	NS_LOG_DEBUG("AquaSimQLRPS::Terminate: Node=" << GetNetDevice()->GetAddress() << ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() << ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
}

void AquaSimQLRPS::StopSource()
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
AquaSimQLRPS::CreatePacket()
{
	NS_LOG_FUNCTION(this);

	Ptr<Packet> pkt = Create<Packet>();

	if (pkt == NULL)
		return NULL;

	QLRPSHeader vbh;
	AquaSimTrailer tra;
	// ash.SetSize(36);
	// vbh.SetTs(Simulator::Now().ToDouble(Time::S));

	//!! I add new part

	vbh.SetExtraInfo_o(GetNetDevice()->GetPosition());
	vbh.SetExtraInfo_f(GetNetDevice()->GetPosition());

	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	return pkt;
}

void AquaSimQLRPS::PrepareMessage()
{
	std::cout << "-----------qlr PrepareMessage-----------\n";
	Ptr<Packet> pkt = Create<Packet>();
	std::cout << "-----------qlrp PrepareMessage Create pkt-----------\n";
	QLRPSHeader vbh;
	AquaSimTrailer ast;
	ast.SetModeId(1);
	ast.SetSAddr(AquaSimAddress::GetBroadcast());
	ast.SetDAddr(AquaSimAddress::GetBroadcast());
	std::cout << "-----------ast.SetNextHop-----------\n";
	ast.SetNextHop(AquaSimAddress::GetBroadcast());
	std::cout << "-----------ast.SetNexthopAddr-----------\n";
	vbh.SetNexthopAddr(AquaSimAddress::GetBroadcast());
	// todo set 模式号
	// 信息包
	vbh.SetMessType(DATA_READY);
	vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	vbh.SetNexthopAddr(AquaSimAddress::GetBroadcast());
	vbh.SetResiEnergy(GetNetDevice()->EnergyModel()->GetEnergy());
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		if (curentry.first == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()))
		{
			vbh.SetAvgEnergy(neibor->avgenergy);
			vbh.SetDensity(neibor->density);
		}
	}
	std::cout << "vbh.SetSenderAddr:" << vbh.GetSenderAddr() << "\n";
	// printf("vectorbasedforward: last line MACprepare\n");
	Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
	vbh.SetExtraInfo_f(model->GetPosition());

	pkt->AddHeader(vbh);
	std::cout << "-----------pkt->AddHeader(vbh);-----------\n";
	pkt->AddTrailer(ast);
	std::cout << "-----------pkt->AddTrailer(ast);;-----------\n";
	MACsend(pkt, 0);
	// 发送完后更新自己的表中能量和平均能量值
	// todo
	updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	Simulator::Schedule(Seconds(65.0),
						&AquaSimQLRPS::PrepareMessage, this); // 定期调用自己
	// MACprepare(pkt);
	// return pkt;
}

void AquaSimQLRPS::MACprepareF(Ptr<Packet> pkt)
{
	std::cout << "-----------qlrp MACprepareF-----------\n";
	QLRPSHeader vbh;
	AquaSimTrailer tra;
	pkt->RemoveTrailer(tra);
	pkt->RemoveHeader(vbh);
	// 记录前一跳节点
	AquaSimAddress previous_hop = vbh.GetForwardAddr();
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	// 计算V值
	AquaSimAddress curadd = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
	double V = INT8_MIN; // INT8_MIN===== -127
	AquaSimAddress nexthop;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		// std::cout<<"local_entry add: "<<curentry.first<<"position:"<<curentry.second.x<<" "<<curentry.second.y<<" "<<curentry.second.z<<"\n";
		if (curentry.first == curadd || curentry.first == previous_hop)
		{ // 不能计算自己,不能计算它的前一跳节点的V值
			continue;
		}
		else
		{
			double succEn = SuccEnergy(neibor->energy);
			double defEn = DefeatEnergy();
			double d = Projection(pkt, curentry.second);
			double SuccAvgEn = SuccAvgEnergy(neibor->energy, neibor->avgenergy);
			double DefeatAvgEn = DefeatAvgEnergy();
			double Density = getDensity(neibor->density);
			double Psucc = Sendsucc(neibor->sendnum, neibor->sendsucc);
			double Pdefeat = 1 - Psucc;
			std::cout << "Node:" << curentry.first << " Vvalue  succEn:" << succEn << ","
					  << "defEn:" << defEn << ","
					  << "projection: " << d << ","
					  << "defAvgEn:" << DefeatAvgEn << ","
					  << "succAvgen:" << SuccAvgEn << ","
					  << "Density:" << Density
					  << ","
					  << "Psucc:" << Psucc << ","
					  << "Pdefeat:" << Pdefeat << "\n";
			// todo 计算奖励惩罚函数// add  alpha,beita,gama,yita
			double Rt = Psucc * (-g + a1 * succEn + a2 * SuccAvgEn + a3 * d + a4 * Density) + Pdefeat * (-g + b1 * defEn + b2 * DefeatAvgEn + b3 * d + b4 * Density);
			// 计算V值,判断V值大小，找下一跳,
			double curV = Rt + theta * (-1) * neibor->value;
			// double curV=Rt+theta*((-1)*Pdefeat*curnode->value+(-1)*Psucc*neibor->value);
			// std::cout<<"neibor->value:"<<neibor->value<<"\n";
			std::cout << "Cur_Node:" << curadd << "  calculate Table_node:" << curentry.first << " 's Rt:" << Rt << ", Vvalue:" << curV << "\n";
			if (curV > V)
			{
				V = curV;
				nexthop = curentry.first;
			}
		}
	}
	std::cout << "Current_Node: " << curadd << ","
			  << " Nexthop:" << nexthop << ","
			  << " Vvalue:" << V << "\n";
	// 下一跳
	vbh.SetNexthopAddr(nexthop);
	vbh.SetPrevioushop(previous_hop);
	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	// 发送以后更新本地表对应邻居的sendnum  and  和自己的V值 Vvalue
	PktlocalTable.UpdateHash(curadd, nexthop, (-1) * V);
	// todo    set other info
	// vbh.SetNexthopAddr();
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		local_entry cur = it->first;
		qlrp_neighbor *own = it->second;
		if (cur.first == curadd)
		{
			vbh.SetAvgEnergy(own->avgenergy);
			vbh.SetDensity(own->density);
			vbh.SetValue(own->value);
			vbh.SetResiEnergy(own->energy);
			// std::cout<<"LocalAdd:"<<curadd<<", Avgenergy:"<<own->avgenergy<<", Density: "<<own->density<<",ResiEnergy:"<<own->energy
			//<<",Value:"<<own->value<<"\n";
		}
	}
	tra.SetNextHop(AquaSimAddress::GetBroadcast());
	tra.SetModeId(1);
	Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
	vbh.SetExtraInfo_f(model->GetPosition());

	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	// printf("vectorbasedforward: last line MACprepare\n");
}

double
AquaSimQLRPS::calculateAvgEn()
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	double avgEn = 0;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		avgEn += neibor->energy;
	}
	return avgEn / PktlocalTable.m_htable.size();
}

void AquaSimQLRPS::MACprepare(Ptr<Packet> pkt)
{
	std::cout << "-----------qlrp MACprepare send DATA_READY-----------\n";
	QLRPSHeader vbh;
	AquaSimTrailer tra;

	pkt->RemoveTrailer(tra);
	pkt->RemoveHeader(vbh);

	// 信息包
	vbh.SetMessType(DATA_READY);
	vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	vbh.SetNexthopAddr(AquaSimAddress::GetBroadcast());
	vbh.SetResiEnergy(GetNetDevice()->EnergyModel()->GetEnergy());
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		if (curentry.first == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()))
		{
			vbh.SetAvgEnergy(neibor->avgenergy);
			vbh.SetDensity(neibor->density);
		}
	}

	// vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	// todo
	// vbh.SetNexthopAddr();

	tra.SetNextHop(AquaSimAddress::GetBroadcast());
	tra.SetModeId(1);
	Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
	vbh.SetExtraInfo_f(model->GetPosition());

	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	// printf("vectorbasedforward: last line MACprepare\n");
	MACsend(pkt, 10);
	// 更新能量,更新自己的avgenergy
	updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
}

void AquaSimQLRPS::MACsend(Ptr<Packet> pkt, double delay)
{
	std::cout << "-----------qlrp MACsend-----------\n";
	NS_LOG_INFO("MACsend: delay " << delay << " at time " << Simulator::Now().GetSeconds());

	QLRPSHeader vbh;
	AquaSimTrailer tra;

	pkt->RemoveTrailer(tra);
	pkt->RemoveHeader(vbh);
	AquaSimAddress next_hop = vbh.GetNexthopAddr();
	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	printHopbyhopSenddelay(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), next_hop, pkt->GetUid());
	/*Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
						  pkt,AquaSimAddress::GetBroadcast(),Seconds(0));*/
	// first seconnds  how long time to send
	Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this,
						pkt, next_hop, Seconds(0));
}

double AquaSimQLRPS::getlocalV(AquaSimAddress source)
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	double sourceV = 0.0;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		if (curentry.first == source)
		{
			sourceV = neibor->value;
		}
	}
	return sourceV;
}
void AquaSimQLRPS::updateEnergyAfterSend(AquaSimAddress local)
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
	Vector p = GetNetDevice()->GetNode()->GetObject<MobilityModel>()->GetPosition();
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		if (curentry.first == local)
		{
			PktlocalTable.PutInHash(local, neibor->value, p, LocalResiEnergy,
									calculateAvgEn(), neibor->density, 0, 0);
		}
	}
}

void AquaSimQLRPS::updateAvgEnergyAfterrecv(AquaSimAddress local)
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		if (curentry.first == local)
		{
			PktlocalTable.PutInHash(local, neibor->value, curentry.second, neibor->energy,
									calculateAvgEn(), neibor->density, 0, 0);
		}
	}
}

void AquaSimQLRPS::printSourceV(int pktsendnum)
{
	std::ofstream outfile("SourceValue.txt", std::ios::app);
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		local_entry curentry = it->first;
		qlrp_neighbor *own = it->second;
		if (curentry.first == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()))
		{
			outfile << /*AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())<<" "<<*/ (-1) * own->value << " " << pktsendnum << "\n";
		}
	}
	outfile.close();
}

void AquaSimQLRPS::updateDensity_delete_entry(AquaSimAddress local)
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	Vector p = GetNetDevice()->GetNode()->GetObject<MobilityModel>()->GetPosition();
	// 先更新自己的位置.因为位置包含在map的key值中，所以只能删除原来的再加入新的
	/*for(it=PktlocalTable.m_htable.begin();it!=PktlocalTable.m_htable.end();it++){
		 local_entry curentry=it->first;
		 qlrp_neighbor *own= it->second;
		 if(curentry.first==local){
			 curentry.second.x=p.x;
			 curentry.second.y=p.y;
			 curentry.second.z=p.z;
			 break;
		 }
	 }*/
	// 删除不在传输范围内的条目
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		local_entry curentry = it->first;
		qlrp_neighbor *own = it->second;
		// std::cout<<"localadd:"<<local<<" ,location:"<<p.x<<" "<<p.y<<" "<<p.z<<"\n";
		// std::cout<<"neibor:"<<curentry.first<<" ,location:"<<curentry.second.x<<" "<<curentry.second.y<<" "<<curentry.second.z<<"\n";
		if (curentry.first != local && iscloseenough(p, curentry.second) == 1)
		{
			// std::cout<<"iscloseenough!"<<"\n";
			continue;
		}
		else if (curentry.first != local && iscloseenough(p, curentry.second) == 0)
		{
			std::map<local_entry, qlrp_neighbor *>::iterator iter;
			iter = PktlocalTable.m_htable.erase(it);
			it = iter;
			std::cout << "Delete entry successfully!!!!!" << std::endl;
		}
		else if (curentry.first == local)
		{
			curentry.second = p;
		}
	}
	// update own density
	std::map<local_entry, qlrp_neighbor *>::iterator it1;
	for (it1 = PktlocalTable.m_htable.begin(); it1 != PktlocalTable.m_htable.end(); it1++)
	{
		local_entry curentry = it1->first;
		qlrp_neighbor *own = it1->second;
		if (curentry.first == local)
		{
			own->density = PktlocalTable.m_htable.size();
			std::cout << "Update Density after delete entry successfully!!!!!"
					  << " ,curlocation:" << curentry.second.x << " " << curentry.second.y << " " << curentry.second.z << std::endl;
		}
	}
	// print node table:
	/*std::cout<<"localadd: "<<local<<" 's table is : "<<"\n";
	for(it=PktlocalTable.m_htable.begin();it!=PktlocalTable.m_htable.end();it++){
		local_entry cur=it->first;
	   qlrp_neighbor *own=it->second;
	   std::cout<<cur.first<<" , ";
	}
	std::cout<<"\n";*/
}
void AquaSimQLRPS::printEnergy(AquaSimAddress local, int pktID)
{
	std::ofstream outfile("NodeResiEnergy.txt", std::ios::app);
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	// 打印700次以后发的包
	// if(pktID>700){
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		local_entry curentry = it->first;
		qlrp_neighbor *own = it->second;
		if (curentry.first == local)
		{
			// std::cout<<"LocalNode:"<<local<<",resiEnergy:"<<own->energy<<"\n";
			outfile << /*"PktID:"<<pktID<<*/ "Node:" << local /*<<"  ResiEnergy:"*/ << " " << own->energy << "\n";
			// outfile<<local<<" "<<own->energy<<"\n";
		}
	}
	//}
	outfile.close();
}

void AquaSimQLRPS::iniNodetable()
{
	std::cout << "QLRPS iniNodetable"
			  << "\n";
	AquaSimAddress localadd = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
	std::cout << "localadd:" << localadd << "\n";
	Vector p = GetNetDevice()->GetPosition();
	double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
	std::cout << "QLRPS iniNodetable:"
			  << "localadd:" << localadd << ","
			  << "LocalResiEnergy:" << LocalResiEnergy << "\n";
	PktlocalTable.PutInHash(localadd, 0, p, LocalResiEnergy,
							LocalResiEnergy, 0, 0, 0);
	// PrepareMessage();
}

void AquaSimQLRPS::iniNodetable1()
{
	// 初始化自己信息
	// std::cout<<"QLRPS iniNodetable1"<<"\n";
	AquaSimAddress localadd = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
	// std::cout<<"localadd:"<<localadd<<"\n";
	Vector p = GetNetDevice()->GetPosition();
	double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
	// std::cout<<"QLRPS iniNodetable:"<<"localadd:"<<localadd<<","<<"LocalResiEnergy:"<<LocalResiEnergy<<"\n";
	PktlocalTable.PutInHash(localadd, 0, p, LocalResiEnergy,
							LocalResiEnergy, 0, 0, 0);
	// 初始化邻居信息
	std::ifstream neiborinfo("info.txt");
	if (!neiborinfo.is_open())
	{
		std::cout << "can not open this file"
				  << "\n";
	}
	std::string str;
	int current_node;
	Vector q;
	while (getline(neiborinfo, str))
	{
		std::istringstream istr(str);
		istr >> current_node;
		istr >> q.x;
		istr >> q.y;
		istr >> q.z;
		if (iscloseenough(p, q) == 1 && current_node != localadd)
		{
			// std::cout<<"current_node:"<<current_node<<"\n";
			PktlocalTable.PutInHash(current_node, 0, q, LocalResiEnergy,
									LocalResiEnergy, 0, 0, 0);
			// std::cout<<"QLRPS iniNodetable1 insert successful"<<"\n";
		}
	}
	neiborinfo.close();
	// 更新自己节点的密度，平均能量
	PktlocalTable.updateOwn(Avgenergy(), PktlocalTable.m_htable.size(), localadd);
	// todo 打印每个节点的表
	std::cout << "localadd: " << localadd << " 's table is : "
			  << "\n";
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		local_entry cur = it->first;
		qlrp_neighbor *own = it->second;
		std::cout << cur.first << " , ";
	}
	std::cout << "\n";
}

int AquaSimQLRPS::iscloseenough(Vector p, Vector q)
{
	/*std::cout<<"p.x:"<<p.x<<" p.y:"<<p.y<<" p.z:"<<p.z<<"\n";
	std::cout<<"q.x:"<<q.x<<" q.y:"<<q.y<<" q.z:"<<q.z<<"\n";*/
	double d = sqrt((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y) + (p.z - q.z) * (p.z - q.z));
	// std::cout<<"d:"<<d<<"\n";
	return d > 1000 ? 0 : 1;
}
void AquaSimQLRPS::DataForSink(Ptr<Packet> pkt)
{
	QLRPSHeader vbh;
	pkt->RemoveHeader(vbh);
	if (!SendUp(pkt))
		NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}

void AquaSimQLRPS::SetDelayTimer(Ptr<Packet> pkt, double c)
{
	NS_LOG_FUNCTION(this << c);
	if (c < 0)
		c = 0;
	Simulator::Schedule(Seconds(c), &AquaSimQLRPS::Timeout, this, pkt);
}

void AquaSimQLRPS::Timeout(Ptr<Packet> pkt)
{
	QLRPSHeader vbh;

	pkt->PeekHeader(vbh);

	unsigned char msg_type = vbh.GetMessType();
	std::cout << "Timeout:" << msg_type << "\n";
	// vbf_neighborhood  *hashPtr;
	// Ptr<Packet> p1;
}

double
AquaSimQLRPS::Distance(Ptr<Packet> pkt) // d
{
	QLRPSHeader vbh;
	pkt->PeekHeader(vbh);

	double tx = vbh.GetExtraInfo().f.x;
	double ty = vbh.GetExtraInfo().f.y;
	double tz = vbh.GetExtraInfo().f.z;
	// printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
	Vector pos = GetNetDevice()->GetPosition();
	// double x=GetNetDevice()->CX(); //change later
	// double y=GetNetDevice()->CY();// printf(" Vectorbasedforward: I am in advanced\n");
	// double z=GetNetDevice()->CZ();
	//  printf("the target is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
	return sqrt((tx - pos.x) * (tx - pos.x) + (ty - pos.y) * (ty - pos.y) + (tz - pos.z) * (tz - pos.z));
}

double
AquaSimQLRPS::Advance(Ptr<Packet> pkt)
{
	QLRPSHeader vbh;
	pkt->PeekHeader(vbh);

	double tx = vbh.GetExtraInfo().t.x;
	double ty = vbh.GetExtraInfo().t.y;
	double tz = vbh.GetExtraInfo().t.z;
	// TODO
	std::cout << "Advance target position is:" << tx << "," << ty << "," << tz << '\n';
	// printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
	Vector pos = GetNetDevice()->GetPosition();
	std::cout << "Advance current position is:" << pos.x << "," << pos.y << "," << pos.z << '\n';
	// double x=GetNetDevice()->CX(); //change later
	// double y=GetNetDevice()->CY();// printf(" Vectorbasedforward: I am in advanced\n");
	// double z=GetNetDevice()->CZ();
	//  printf("the target is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
	return sqrt((tx - pos.x) * (tx - pos.x) + (ty - pos.y) * (ty - pos.y) + (tz - pos.z) * (tz - pos.z));
}

double
AquaSimQLRPS::Projection(Ptr<Packet> pkt, Vector p)
{
	QLRPSHeader vbh;
	AquaSimTrailer tra;
	pkt->RemoveTrailer(tra);
	pkt->PeekHeader(vbh);
	pkt->AddTrailer(tra);
	double tx = vbh.GetExtraInfo().t.x;
	double ty = vbh.GetExtraInfo().t.y;
	double tz = vbh.GetExtraInfo().t.z;

	Vector o;
	// double ox, oy, oz;
	// std::cout <<"m_hopByHop:"<<m_hopByHop << std::endl;
	if (!m_hopByHop)
	{
		// std::cout<<"enter !m_hopByHop" <<std::endl;
		// printf("vbf is used\n");
		o.x = vbh.GetExtraInfo().o.x;
		o.y = vbh.GetExtraInfo().o.y;
		o.z = vbh.GetExtraInfo().o.z;
	}
	else
	{
		std::cout << "not enter !m_hopByHop" << std::endl;
		// printf("m_hopByHop vbf is used\n");
		o.x = vbh.GetExtraInfo().f.x;
		o.y = vbh.GetExtraInfo().f.y;
		o.z = vbh.GetExtraInfo().f.z;
	}

	// NOTE below may not work if the nodes are mobile.
	/*double x=GetNetDevice()->CX();
	double y=GetNetDevice()->CY();
	double z=GetNetDevice()->CZ();
  */
	// Vector myPos = GetNetDevice()->GetPosition();
	Vector NeiborPos = p;
	// std::cout<<"neiborposition_x:"<<NeiborPos.x<<" "<<NeiborPos.y<<" "<<NeiborPos.z<<std::endl;
	double wx = tx - o.x;
	double wy = ty - o.y;
	double wz = tz - o.z;

	double vx = NeiborPos.x - o.x;
	double vy = NeiborPos.y - o.y;
	double vz = NeiborPos.z - o.z;
	// xiang liang cha cheng qiu mian ji,ci chu qiu wan hai shi xiang liang
	double cross_product_x = vy * wz - vz * wy;
	double cross_product_y = vz * wx - vx * wz;
	double cross_product_z = vx * wy - vy * wx;
	/*std::cout<<"cross_product_x:"<<cross_product_x<<std::endl;
	std::cout<<"cross_product_y:"<<cross_product_y<<std::endl;
	std::cout<<"cross_product_z:"<<cross_product_z<<std::endl;

	std::cout<<"vx-vy-vz:"<<vx<<"-"<<vy<<"-"<<vz<<std::endl;
	std::cout<<"wx-wy-wz:"<<wx<<"-"<<wy<<"-"<<wz<<std::endl;
	std::cout<<"vy*wz:"<<vy*wz<<std::endl;
	std::cout<<"vz*wy:"<<vz*wy<<std::endl;*/
	// mian ji wei cha cheng hou de mo
	double area = sqrt(cross_product_x * cross_product_x +
					   cross_product_y * cross_product_y + cross_product_z * cross_product_z);
	double length = sqrt((tx - o.x) * (tx - o.x) + (ty - o.y) * (ty - o.y) + (tz - o.z) * (tz - o.z));
	// printf("vectorbasedforward: the area is %f and length is %f\n",area,length);
	NS_LOG_DEBUG("Projection: area is " << area << " length is " << length);
	// std::cout<<"Projection: area is " << area << " length is " << length<<std::endl;
	if (length == 0)
		return 0;
	// std::cout<<"Projection="<<area/length<<std::endl;
	double res = area / length;
	return (-1) * (res / 10000);
}

double AquaSimQLRPS::SuccEnergy(unsigned int energy)
{
	double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
	double IniEnergy = GetNetDevice()->EnergyModel()->GetInitialEnergy();
	double LocalE = 1 - LocalResiEnergy / IniEnergy;
	double NeighborE = 1 - energy / IniEnergy;
	double res = (-1) * (LocalE + NeighborE);
	return res;
}

double AquaSimQLRPS::DefeatEnergy()
{
	double ResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
	double IniEnergy = GetNetDevice()->EnergyModel()->GetInitialEnergy();
	double res = (-1) * (1 - ResiEnergy / IniEnergy);
	return res;
}

double AquaSimQLRPS::SuccAvgEnergy(unsigned int energy, unsigned int avgenergy)
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	double AvgESi = 0.0;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		AvgESi += neibor->energy;
	}
	AvgESi = AvgESi / PktlocalTable.m_htable.size(); // Si节点平均能量
	double ResiEnergySi = GetNetDevice()->EnergyModel()->GetEnergy();
	double ESi = atan(ResiEnergySi - AvgESi) / M_PI; // Si节点能量分布
	double ResiEnergySj = energy;
	double AvgESj = avgenergy;						 // Sj节点平均能量
	double ESj = atan(ResiEnergySj - AvgESj) / M_PI; // Sj节点能量分布
	double AvgE = ESi + ESj;
	return AvgE;
}

double AquaSimQLRPS::DefeatAvgEnergy()
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	double AvgESi = 0.0;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		AvgESi += neibor->energy;
	}
	AvgESi = AvgESi / PktlocalTable.m_htable.size(); // Si节点平均能量
	double ResiEnergySi = GetNetDevice()->EnergyModel()->GetEnergy();
	double ESi = atan(ResiEnergySi - AvgESi) / M_PI;
	return ESi;
}

double AquaSimQLRPS::getDensity(unsigned int den)
{
	// std::cout<<"Density:"<<(-1)*(1-(float)den/47)<<"\n";
	// std::cout<<"neibor num/47:"<<(float)den/47<<"\n";
	return (-1) * (1 - (float)den / 47);
}
double AquaSimQLRPS::Avgenergy()
{
	std::map<local_entry, qlrp_neighbor *>::iterator it;
	double AvgESi = 0.0;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<local_entry,qlrp_neighbor*> curpair;
		local_entry curentry = it->first;
		qlrp_neighbor *neibor = it->second;
		AvgESi += neibor->energy;
	}
	AvgESi = AvgESi / PktlocalTable.m_htable.size(); // Si节点平均能量
	return AvgESi;
}

double AquaSimQLRPS::Sendsucc(unsigned int sendnum, unsigned int sendsucc)
{
	// std::cout<<"sendnum:"<<sendnum<<" ,sendsucc:"<<sendsucc<<"\n";
	if (sendnum == 0)
	{
		return 1;
	}
	return (float)sendsucc / sendnum;
}

void AquaSimQLRPS::printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid)
{
	std::ofstream hopbyhop("Nodehopbyhop.txt", std::ios::app);
	hopbyhop << "PktID:" << pktid << " Node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " RecvTime:"
			 << Simulator::Now().GetSeconds() << " from node:" << previous << "\n";
	hopbyhop.close();
}

void AquaSimQLRPS::printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid)
{
	std::ofstream hopbyhop("Nodehopbyhop.txt", std::ios::app);
	hopbyhop << "PktID:" << pktid << " Node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " SendTime:"
			 << Simulator::Now().GetSeconds() << " to node:" << nexthop << "\n";
	hopbyhop.close();
}

void AquaSimQLRPS::printEndtoendRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid)
{
	std::cout << "PktID:" << pktid << " Node:" << local << "\n";
}

void AquaSimQLRPS::printEndtoendSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid)
{
	std::cout << "PktID:" << pktid << " Node:" << local << "\n";
}

/*
double AquaSimQLRPS::SendDefeat(){

}
*/
bool AquaSimQLRPS::IsTarget(Ptr<Packet> pkt)
{
	QLRPSHeader vbh;
	pkt->PeekHeader(vbh);

	// TODO
	// if (vbh.GetTargetAddr().GetAsInt()==1) {
	if (vbh.GetTargetAddr().GetAsInt() == 0)
	{

		//  printf("vectorbased: advanced is %lf and my range is %f\n",Advance(pkt),vbh.GetRange());
		return (true);
	}
	else
		return (GetNetDevice()->GetAddress() == vbh.GetTargetAddr());
}

void AquaSimQLRPS::DoDispose()
{
	m_rand = 0;
	AquaSimRouting::DoDispose();
}

void AquaSimQLRPS::SetTargetPos(Vector pos)
{
	m_targetPos = pos;
}
