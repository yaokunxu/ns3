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

#include "aqua-sim-routing-CARMA.h"
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

#define Rec -1
#define Drop -2
#define Pkt_Not_find -3

NS_LOG_COMPONENT_DEFINE("AquaSimCARMA");
// NS_OBJECT_ENSURE_REGISTERED(AquaSimPktHashTable);
// todo add timeer
// clock_t start,finish;
AquaSimPktlocalTableCARMA::AquaSimPktlocalTableCARMA()
{
	NS_LOG_FUNCTION(this);
	m_windowSize = WINDOW_SIZE;
}
AquaSimPktlocalTableCARMA::~AquaSimPktlocalTableCARMA()
{
	NS_LOG_FUNCTION(this);
	for (std::map<AquaSimAddress, carma_neighbor *>::iterator it = m_htable.begin(); it != m_htable.end(); ++it)
	{
		delete it->second;
	}
	m_htable.clear();
}
void AquaSimPktlocalTableCARMA::Reset()
{
	m_htable.clear();
}
carma_neighbor *
AquaSimPktlocalTableCARMA::GetHash(AquaSimAddress forwarderAddr)
{
	// std::cout<<"++++++GetHash+++++++++++++\n";
	AquaSimAddress entry = forwarderAddr;
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	it = m_htable.find(entry);
	if (it == m_htable.end())
		return NULL;
	return it->second;
}
void AquaSimPktlocalTableCARMA::PutInHash(AquaSimAddress fAddr, double Vvalue, uint16_t ppj, double PPij)
{
	NS_LOG_DEBUG("PutinHash begin:" << fAddr << "," << Vvalue << "," << ppj << "," << PPij);
	// std::cout<<"   Putinhash++++++++++++++    \n ";
	// std::cout<<"PutinHash begin:" << fAddr << "," << Vvalue << ",(" << p.x << "," << p.y << "," << p.z << ")"<<"\n";
	AquaSimAddress entry = fAddr;
	carma_neighbor *hashptr;
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	// todo //表中已有该节点信息，更新,(因为位置包含在map的key值中，所以只能删除原来的再加入新的)
	for (it = m_htable.begin(); it != m_htable.end(); it++)
	{
		AquaSimAddress cur = it->first;
		carma_neighbor *nei = it->second;
		if (cur == fAddr)
		{
			// std::cout<<"hasptr exit,update\n";
			// update Vvalue after recv a packet?
			nei->value = Vvalue;
			if (nei->pj < ppj)
			{
				nei->pj = ppj;
			}
			nei->Pij = PPij;
			// update location
			return;
		}
	}
	// 以后有机会再考虑移动的
	/*for(it=m_htable.begin();it!=m_htable.end();it++){
		AquaSimAddress cur=it->first;
		carma_neighbor *nei=it->second;
		if(cur.first==fAddr){
		   //std::cout<<"hasptr exit,update\n";
		  unsigned int sndnum=nei->sendnum;
		  unsigned int sndsucc=nei->sendsucc;
		   std::map<AquaSimAddress,carma_neighbor*>::iterator iter;
		   iter=m_htable.erase(it);
		   it=iter;
		   PutInHash(fAddr,Vvalue,p,Energy,Avgenergy,Density,sndnum,sndsucc);
		   return;
		}
	}*/
	hashptr = new carma_neighbor[1];
	hashptr[0].value = Vvalue;
	hashptr[0].pj = ppj;
	hashptr[0].pji = 0;
	hashptr[0].Pij = PPij;
	hashptr[0].Pji = 0;
	std::pair<AquaSimAddress, carma_neighbor *> newPair;
	newPair.first = entry;
	newPair.second = hashptr;
	// std::cout<<"position:"<<entry.first<<"\n";
	//  std::cout<<m_htable.size()<<"......before insert.......\n";
	m_htable.insert(newPair);
	// std::cout<<m_htable.size()<<"......after insert.......\n";
	/*for(it=m_htable.begin();it!=m_htable.end();it++){
		carma_neighbor *h=it->second;
		AquaSimAddress add=it->first;
		std::cout<<"insert Addr:"<<add.first<<"\n";
		std::cout<<"insert energy:"<<h->energy<<",insert value:"<<h->value<<", insert density: "<<h->density<<"......after insert.......\n";
	}*/
	//}
}
void AquaSimPktlocalTableCARMA::UpdateHash(AquaSimAddress local, double Vvalue)
{ // 更新V值和Sendnum
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	for (it = m_htable.begin(); it != m_htable.end(); it++)
	{
		AquaSimAddress update = it->first;
		carma_neighbor *neibor = it->second;
		if (update == local)
		{
			neibor->value = Vvalue;
			// std::cout<<"beibor->Vvalue:"<<neibor->value<<", Vvalue:"<<Vvalue<<"  ,Update vvalue successfully\n";
		}
	}
}
void AquaSimPktlocalTableCARMA::UpdateSuccnum(AquaSimAddress previous, AquaSimAddress forward)
{
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	for (it = m_htable.begin(); it != m_htable.end(); it++)
	{
		AquaSimAddress update = it->first;
		carma_neighbor *neibor = it->second;
		if (update == forward)
		{
			neibor->pji++;
		}
	}
	std::cout << "Previousnode:" << previous << " recv the ACK from node:" << forward << " Update Sendsucc successfully\n";
}

NS_OBJECT_ENSURE_REGISTERED(AquaSimCARMA);

AquaSimCARMA::AquaSimCARMA()
{
	// Initialize variables.
	//  printf("VB initialized\n");
	m_pkCount = 1;
	// TODO
	// m_priority=2;
	// m_useOverhear = 0;
	m_targetPos = Vector();
	m_rand = CreateObject<UniformRandomVariable>();
	// todo定期调用 创建包发包//初始化本地表
	/// 延迟一段时间构造，因为可能节点信息还没有创建
	/*Simulator::Schedule(Seconds(0.5),
				 &AquaSimCARMA::iniNodetable, this);*/
	/*Simulator::Schedule(Seconds(10),
				  &AquaSimCARMA::PrepareMessage, this);*/
	Simulator::Schedule(Seconds(rand() % 30), &AquaSimCARMA::broadinit, this);
}

TypeId
AquaSimCARMA::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::AquaSimCARMA")
							.SetParent<AquaSimRouting>()
							.AddConstructor<AquaSimCARMA>()
							.AddAttribute("TargetID", "ID of target sink.",
										  IntegerValue(51),
										  MakeIntegerAccessor(&AquaSimCARMA::m_targetAddress),
										  MakeIntegerChecker<int>());
	return tid;
	// bind("m_useOverhear_", &m_useOverhear);
}

int64_t
AquaSimCARMA::AssignStreams(int64_t stream)
{
	NS_LOG_FUNCTION(this << stream);
	m_rand->SetStream(stream);
	return 1;
}
// 传输层调用Txprocess
bool AquaSimCARMA::TxProcess(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
	CARMAHeader vbh;
	vbh.SetM(0);
	vbh.SetN(0);
	// start=clock();
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
bool AquaSimCARMA::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
	NS_LOG_FUNCTION(this);
	std::cout << "================carma Recv=================\n";
	std::cout << "Current node:" << AquaSimAddress::ConvertFrom(m_device->GetAddress()) << "\n";
	std::cout << "table size:" << PktlocalTable.m_htable.size() << std::endl;
	// std::cout<<"Current node:"<<GetNetDevice()->GetAddress()<<"\n";
	// AquaSimEnergyModel en;
	// en.GetEnergy();
	// std::cout<<"carma energy "<<en.GetEnergy();
	CARMAHeader vbh;
	AquaSimTrailer tra;

	packet->RemoveHeader(vbh);
	packet->RemoveTrailer(tra);

	int type = vbh.GetMessType();

	if (vbh.GetMessType() != AS_DATA && vbh.GetMessType() != DATA_READY)
	{ // no headers //TODO create specalized Application instead of using this hack.// how many times this pkt was forwarded
		tra.SetNextHop(AquaSimAddress::GetBroadcast());
		tra.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		tra.SetDAddr(AquaSimAddress::ConvertFrom(dest));
		tra.SetModeId(1);
		vbh.SetMessType(AS_DATA);
		vbh.SetM(0);
		vbh.SetN(0);
		vbh.SetPrevioushop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		vbh.SetPkNum(m_pkCount++);
		// todo
		vbh.SetValue(getlocalV(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())));
		// vbh.SetNexthopAddr();

		/*AquaSimEnergyModel en;
		std::cout<<"-----------extra energy-----"<<" "<<GetNetDevice()->EnergyModel()->GetEnergy();*/
		// vbh.SetOriginalSource(sModel->GetPosition());
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
	// 接到的数据包先将包信息插入(表中有信息则更新能量，位置信息,更新自己的avgenergy,没有就插入所有)

	PktlocalTable.PutInHash(vbh.GetForwardAddr(), vbh.GetValue(), vbh.GetPkNum(),
							vbh.GetP(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt()));
	carma_neighbor *hash = PktlocalTable.GetHash(vbh.GetForwardAddr());
	// todo

	hash->pji++;

	// 以后有机会再考虑移动的
	// updateDensity_delete_entry(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	// std::cout<<"insert successfully---------------\n";

	// 加入位置信息然后considerNew然后considerNew
	// Todo
	// std::cout<<"position.x:"<<sModel->GetPosition().x<<"position.y:"<<sModel->GetPosition().y<<"position.z:"<<sModel->GetPosition().z<<std::endl;
	// todo end
	// d:关于接收方与转发器的相对位置的信息

	packet->RemoveTrailer(tra);
	packet->RemoveHeader(vbh);

	packet->AddHeader(vbh);
	packet->AddTrailer(tra);
	if (type != DATA_READY)
	{
		ConsiderNew(packet);
	}
	return true;
}

void AquaSimCARMA::ConsiderNew(Ptr<Packet> pkt)
{
	std::cout << "===========carma: considerNew===========\n";
	NS_LOG_FUNCTION(this);

	CARMAHeader vbh;
	AquaSimTrailer tra;
	pkt->RemoveTrailer(tra);
	pkt->PeekHeader(vbh);
	pkt->AddTrailer(tra);
	// Todo
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
	NS_LOG_INFO("AquaSimCARMA::ConsiderNew: data packet");
	// todo 打印每个节点的剩余能量
	// from_nodeAddr = vbh.GetSenderAddr();
	from_nodeAddr = vbh.GetForwardAddr();
	// TODO
	// 判断Previous_node是否是自己，是就更新本地表Sendsucc字段（隐式确认）
	if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() == vbh.GetPrevioushop())
	{
		PktlocalTable.UpdateSuccnum(vbh.GetPrevioushop(), vbh.GetForwardAddr());
		PktRecv(vbh.GetPkNum());
	}
	// 第一个节点发送
	if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == from_nodeAddr)
	{
		// come from the same node, broadcast it
		// todo
		// while(m_pkCount<1){//源节点不断发包.m_pkCount<100//设置多久发一次schedule()
		m_pkCount++;
		std::cout << "Node::" << GetNetDevice()->GetAddress() << "  m_pkCount++::" << m_pkCount << ",,"
				  << "considerNew_MACsssend\n";
		// MACprepareF(pkt);
		Simulator::Schedule(Seconds(0.5),
							&AquaSimCARMA::MACprepareF, this, pkt);
		// MACsend(pkt,0.5);//macsend比Macprepare提前执行了
		Simulator::Schedule(Seconds(0.5), &AquaSimCARMA::MACsend, this, pkt, 0.5);
		// 发送完后更新自己的表中能量和平均能量值
		// todo
		// updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		// 把source节点的V值单独打印log
		Simulator::Schedule(Seconds(0.5),
							&AquaSimCARMA::printSourceV, this, m_pkCount);
		// printSourceV();
		//}
		return;
	}
	if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() == m_targetAddress) // 接收节点
	{
		std::cout << "Node::" << GetNetDevice()->GetAddress() << "recv data for sink,packet id:" << pkt->GetUid()
				  << ",At time :" << Simulator::Now().GetSeconds() << "\n";
		double curtime = Simulator::Now().GetSeconds();
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
	if (IsOneofNexthop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt(), vbh.GetM(), vbh.GetRelay()))
	{
		MACprepareF(pkt);
		MACsend(pkt, 0);
		// 发送完后更新自己的表中能量和平均能量值
		// todo
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

void AquaSimCARMA::Reset()
{
	PktlocalTable.Reset();
	/*
	   for (int i=0; i<MAX_DATA_TYPE; i++) {
	   routing_table[i].Reset();
	   }
	 */
}

void AquaSimCARMA::Terminate()
{
	NS_LOG_DEBUG("AquaSimCARMA::Terminate: Node=" << GetNetDevice()->GetAddress() << ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() << ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
}

Ptr<Packet>
AquaSimCARMA::CreatePacket()
{
	NS_LOG_FUNCTION(this);

	Ptr<Packet> pkt = Create<Packet>();

	if (pkt == NULL)
		return NULL;

	CARMAHeader vbh;
	AquaSimTrailer tra;
	// ash.SetSize(36);
	// vbh.SetTs(Simulator::Now().ToDouble(Time::S));

	//!! I add new part

	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	return pkt;
}

void AquaSimCARMA::MACprepareF(Ptr<Packet> pkt)
{
	std::cout << "-----------carma MACprepareF-----------\n";
	CARMAHeader vbh;
	AquaSimTrailer tra;

	pkt->RemoveTrailer(tra);
	pkt->RemoveHeader(vbh);
	// 记录前一跳节点

	AquaSimAddress previous_hop = vbh.GetForwardAddr();
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	// 计算V值
	AquaSimAddress curadd = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
	int status;
	uint16_t pktnum;

	if (previous_hop == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()))
	{
		pktnum = vbh.GetPkNum();
		status = getstatus(pktnum);
	}
	else
	{
		vbh.SetPkNum(m_pkCount++);
		pktnum = vbh.GetPkNum();
		status = getstatus(pktnum);
	}
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		AquaSimAddress curentry = it->first;
		carma_neighbor *neibor = it->second;
		neibor->Pji = (double)neibor->pji / (double)neibor->pj;
	}
	std::vector<uint8_t> node;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		node.push_back(AquaSimAddress::ConvertFrom(it->first).GetAsInt());
	}
	std::vector<std::vector<uint8_t>> comb;
	for (int i = 1; i <= node.size(); i++)
	{
		std::vector<uint8_t> temp(i);
		Combination(node, temp, comb, 0, i, i);
	}
	double vis[m_k + 2] = {0};
	std::vector<uint8_t> a;
	for (int i = 0; i < m_k; i++)
	{
		double curq = INT_FAST64_MAX;
		for (auto j : comb)
		{
			double q;
			double c;
			double e = GetNetDevice()->EnergyModel()->GetTxPower();
			double n = GetNisa(j);
			double p = GetPisa(j);
			double sigma = 0;
			if (i == m_k - 1)
			{
				double l = 1000 * GetLisa(j);
				c = e + n + l;
			}
			else
			{
				c = e + n;
			}
			for (int k = 0; k < m_k; k++)
			{
				sigma += p * vis[k];
			}
			q = c + theta * sigma;
			if (q < curq)
			{
				curq = q;
				vis[i] = q;
				if (i == status)
				{
					a = j;
				}
			}
		}
	}
	V = vis[0];

	std::cout << "Current_Node: " << curadd << ","
			  << " Vvalue:" << V << "\n";
	for (auto i : a)
	{
		std::cout << "Nexthop" << (int)i << std::endl;
	}
	// 下一跳
	vbh.SetM(a.size());
	vbh.SetRelay(a);
	vbh.SetValue(V);
	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	vbh.SetPrevioushop(previous_hop);
	uint8_t size = PktlocalTable.m_htable.size();
	vbh.SetN(size);
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		vbh.SetP(AquaSimAddress::ConvertFrom(it->first).GetAsInt(), it->second->Pji);
	}
	// 发送以后更新本地表对应邻居的sendnum  and  和自己的V值 Vvalue

	// todo    set other info
	// vbh.SetNexthopAddr();

	tra.SetNextHop(AquaSimAddress::GetBroadcast());
	tra.SetModeId(1);

	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	// printf("vectorbasedforward: last line MACprepare\n");
	return;
}
void AquaSimCARMA::MACsend(Ptr<Packet> pkt, double delay)
{
	std::cout << "-----------carma MACsend-----------\n";
	NS_LOG_INFO("MACsend: delay " << delay << " at time " << Simulator::Now().GetSeconds());

	CARMAHeader vbh;
	AquaSimTrailer tra;
	AquaSimAddress next_hop = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
	pkt->RemoveTrailer(tra);
	pkt->RemoveHeader(vbh);

	uint8_t type=vbh.GetMessType();

	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	if (type== AS_DATA)
	{
		Processonpkt(pkt->Copy());
		printHopbyhopSenddelay(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), next_hop, pkt->GetUid());
	}
	/*Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
						  pkt,AquaSimAddress::GetBroadcast(),Seconds(0));*/
	// first seconnds  how long time to send
	Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this, pkt, next_hop, Seconds(0));
}
double AquaSimCARMA::getlocalV(AquaSimAddress source)
{
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	double sourceV = 0.0;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		// std::pair<AquaSimAddress,carma_neighbor*> curpair;
		AquaSimAddress curentry = it->first;
		carma_neighbor *neibor = it->second;
		if (curentry == source)
		{
			sourceV = neibor->value;
		}
	}
	return sourceV;
}
void AquaSimCARMA::printSourceV(int pktsendnum)
{
	std::ofstream outfile("SourceValue.txt", std::ios::app);
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		AquaSimAddress curentry = it->first;
		carma_neighbor *own = it->second;
		if (curentry == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()))
		{
			outfile << /*AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())<<" "<<*/ (-1) * own->value << " " << pktsendnum << "\n";
		}
	}
	outfile.close();
}
int AquaSimCARMA::iscloseenough(Vector p, Vector q)
{
	/*std::cout<<"p.x:"<<p.x<<" p.y:"<<p.y<<" p.z:"<<p.z<<"\n";
	std::cout<<"q.x:"<<q.x<<" q.y:"<<q.y<<" q.z:"<<q.z<<"\n";*/
	double d = sqrt((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y) + (p.z - q.z) * (p.z - q.z));
	// std::cout<<"d:"<<d<<"\n";
	return d > 1000 ? 0 : 1;
}
void AquaSimCARMA::DataForSink(Ptr<Packet> pkt)
{
	CARMAHeader vbh;
	pkt->RemoveHeader(vbh);
	if (!SendUp(pkt))
		NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}
double AquaSimCARMA::SuccEnergy(unsigned int energy)
{
	double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
	double IniEnergy = GetNetDevice()->EnergyModel()->GetInitialEnergy();
	double LocalE = 1 - LocalResiEnergy / IniEnergy;
	double NeighborE = 1 - energy / IniEnergy;
	double res = (-1) * (LocalE + NeighborE);
	return res;
}
double AquaSimCARMA::DefeatEnergy()
{
	double ResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
	double IniEnergy = GetNetDevice()->EnergyModel()->GetInitialEnergy();
	double res = (-1) * (1 - ResiEnergy / IniEnergy);
	return res;
}
double AquaSimCARMA::Sendsucc(unsigned int sendnum, unsigned int sendsucc)
{
	// std::cout<<"sendnum:"<<sendnum<<" ,sendsucc:"<<sendsucc<<"\n";
	if (sendnum == 0)
	{
		return 1;
	}
	return (float)sendsucc / sendnum;
}
void AquaSimCARMA::printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid)
{
	std::ofstream hopbyhop("Nodehopbyhop.txt", std::ios::app);
	hopbyhop << "PktID:" << pktid << " Node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " RecvTime:"
			 << Simulator::Now().GetSeconds() << " from node:" << previous << "\n";
	hopbyhop.close();
}
void AquaSimCARMA::printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid)
{
	std::ofstream hopbyhop("Nodehopbyhop.txt", std::ios::app);
	hopbyhop << "PktID:" << pktid << " Node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " SendTime:"
			 << Simulator::Now().GetSeconds() << " to node:" << nexthop << "\n";
	hopbyhop.close();
}
void AquaSimCARMA::printEndtoendRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid)
{
	std::cout << "PktID:" << pktid << " Node:" << local << "\n";
}
void AquaSimCARMA::printEndtoendSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid)
{
	std::cout << "PktID:" << pktid << " Node:" << local << "\n";
}
/*
double AquaSimCARMA::SendDefeat(){

}
*/
bool AquaSimCARMA::IsTarget(Ptr<Packet> pkt)
{
	CARMAHeader vbh;
	pkt->PeekHeader(vbh);

	// TODO
	// if (vbh.GetTargetAddr().GetAsInt()==1) {
	return (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == m_targetAddress);
}
void AquaSimCARMA::DoDispose()
{
	m_rand = 0;
	AquaSimRouting::DoDispose();
}
void AquaSimCARMA::SetTargetPos(Vector pos)
{
	m_targetPos = pos;
}
bool AquaSimCARMA::IsOneofNexthop(int addr, uint8_t num, std::vector<uint8_t> relay)
{
	for (auto i : relay)
	{
		if ((uint8_t)addr == i)
		{
			return true;
		}
	}
	return false;
}
void AquaSimCARMA::Processonpkt(Ptr<Packet> pkt)
{
	std::cout << "-----------carma Processonpkt-----------\n";
	CARMAHeader vbh;
	AquaSimTrailer tra;

	pkt->RemoveTrailer(tra);
	pkt->RemoveHeader(vbh);

	int times = getstatus(vbh.GetPkNum());

	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	/*Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
						  pkt,AquaSimAddress::GetBroadcast(),Seconds(0));*/
	// first seconnds  how long time to send
	if (times != Rec && times != Drop)
	{
		Simulator::Schedule(Seconds(20), &AquaSimCARMA::Retransmission, this, pkt);
	}
	return;
}
int AquaSimCARMA::getstatus(int num)
{
	for (std::map<int, int>::iterator it = packet.begin(); it != packet.end(); it++)
	{
		if (it->first == num)
		{
			it->second--;
			if (it->second == 0)
			{
				it->second = Drop;
			}
			return it->second;
		}
	}
	std::pair<int, int> newPair;
	newPair.first = num;
	newPair.second = m_k;
	packet.insert(newPair);
	return m_k;
}
void AquaSimCARMA::Retransmission(Ptr<Packet> pkt)
{
	std::cout << "===========carma: Retransmission===========\n";
	NS_LOG_FUNCTION(this);
	// Todo
	// std::cout<<"consider new begin : Advance source position is:"<<vbh.GetExtraInfo().o.x<<","<<vbh.GetExtraInfo().o.y<<","<<vbh.GetExtraInfo().o.z<<'\n';
	// std::cout<<"consider new begin : Advance target position is:"<<tx<<","<<ty<<","<<tz<<'\n';
	// std::cout<<"consider new begin : Advance current position is:"<<pos.x<<","<<pos.y<<","<<pos.z<<'\n';
	// todo end
	NS_LOG_INFO("AquaSimCARMA::Retransmission: data packet");
	CARMAHeader vbh;
	AquaSimTrailer tra;

	pkt->RemoveTrailer(tra);
	pkt->RemoveHeader(vbh);

	std::cout<<"n:"<<vbh.GetN();

	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);

	// todo 打印每个节点的剩余能量
	// from_nodeAddr = vbh.GetSenderAddr();
	// 第一个节点发送
	// come from the same node, broadcast it
	// todo
	// while(m_pkCount<1){//源节点不断发包.m_pkCount<100//设置多久发一次schedule()
	// MACprepareF(pkt);
	Simulator::Schedule(Seconds(0.5), &AquaSimCARMA::MACprepareF, this, pkt);
	// MACsend(pkt,0.5);//macsend比Macprepare提前执行了
	Simulator::Schedule(Seconds(0.5), &AquaSimCARMA::MACsend, this, pkt, 0.5);
	// 发送完后更新自己的表中能量和平均能量值
	// todo
	// updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	// 把source节点的V值单独打印log
	Simulator::Schedule(Seconds(0.5), &AquaSimCARMA::printSourceV, this, m_pkCount);
	// printSourceV();
	//}
	return;
}
void AquaSimCARMA::PktRecv(int num)
{
	for (std::map<int, int>::iterator it = packet.begin(); it != packet.end(); it++)
	{
		if (it->first == num)
		{
			it->second = Rec;
			return;
		}
	}
}
void AquaSimCARMA::broadinit()
{
	std::cout << "-----------carma broadinit-----------\n";
	Ptr<Packet> pkt = Create<Packet>();
	std::cout << "-----------qlrp PrepareMessage Create pkt-----------\n";
	CARMAHeader vbh;
	AquaSimTrailer ast;
	ast.SetModeId(1);
	std::cout << "-----------ast.SetNextHop-----------\n";
	ast.SetNextHop(AquaSimAddress::GetBroadcast());
	std::cout << "-----------ast.SetNexthopAddr-----------\n";
	// todo set 模式号
	// 信息包
	vbh.SetMessType(DATA_READY);
	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt());
	vbh.SetPkNum(0);
	vbh.SetPrevioushop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	//vbh.SetM(0);
	//vbh.SetN(0);
	vbh.SetValue(0);
	// printf("vectorbasedforward: last line MACprepare\n");

	pkt->AddHeader(vbh);
	std::cout << "-----------pkt->AddHeader(vbh);-----------\n";
	pkt->AddTrailer(ast);
	std::cout << "-----------pkt->AddTrailer(ast);;-----------\n";
	MACsend(pkt, 0);
	// 发送完后更新自己的表中能量和平均能量值
	// todo
}

void AquaSimCARMA::Combination(std::vector<uint8_t> &a, std::vector<uint8_t> &b, std::vector<std::vector<uint8_t>> &c, int l, int m, int M)
{
	int N = a.size();
	if (m == 0)
	{
		c.push_back(b);
		return;
	}
	for (int i = l; i < N; i++)
	{
		b[M - m] = a[i];
		Combination(a, b, c, i + 1, m - 1, M);
	}
}

double AquaSimCARMA::GetNisa(std::vector<uint8_t> a)
{
	double n = 0;
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	for (auto i : a)
	{
		for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
		{
			if (AquaSimAddress::ConvertFrom(it->first).GetAsInt() == i)
			{
				n += it->second->value * it->second->Pij;
			}
		}
	}
	return n;
}
double AquaSimCARMA::GetLisa(std::vector<uint8_t> a)
{
	double l = 1;
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	for (auto i : a)
	{
		for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
		{
			if (AquaSimAddress::ConvertFrom(it->first).GetAsInt() == i)
			{
				l *= (1 - it->second->Pij);
			}
		}
	}
	return l;
}
double AquaSimCARMA::GetPisa(std::vector<uint8_t> a)
{
	double P = 1;
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	for (auto i : a)
	{
		for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
		{
			if (AquaSimAddress::ConvertFrom(it->first).GetAsInt() == i)
			{
				P *= (1 - it->second->Pij * it->second->Pji);
			}
		}
	}
	return P;
}