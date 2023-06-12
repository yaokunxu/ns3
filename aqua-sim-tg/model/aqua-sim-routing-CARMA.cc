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

#define Drop -1
#define Rec -2
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
	AquaSimAddress entry = fAddr;
	carma_neighbor *hashptr;
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	if (fAddr.GetAsInt() > 31 || fAddr.GetAsInt() < 1)
	{
		std::cout << "fAddr error:" << fAddr.GetAsInt() << std::endl;
	}
	// todo //表中已有该节点信息，更新,(因为位置包含在map的key值中，所以只能删除原来的再加入新的)
	for (it = m_htable.begin(); it != m_htable.end(); it++)
	{
		AquaSimAddress cur = it->first;
		carma_neighbor *nei = it->second;
		if (cur == fAddr)
		{
			nei->value = Vvalue;
			if (nei->pj < ppj)
			{
				nei->pj = ppj;
			}
			nei->pji++;
			nei->Pij = PPij;
			nei->Pji = (double)nei->pji / (double)nei->pj;
			return;
		}
	}

	hashptr = new carma_neighbor[1];
	hashptr[0].value = Vvalue;
	hashptr[0].pj = ppj;
	hashptr[0].pji = 1;
	hashptr[0].Pij = PPij;
	hashptr[0].Pji = 1;
	std::pair<AquaSimAddress, carma_neighbor *> newPair;
	newPair.first = entry;
	newPair.second = hashptr;

	m_htable.insert(newPair);
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
	std::cout << "Previousnode:" << previous.GetAsInt() << " recv the ACK from node:" << forward.GetAsInt() << " Update Sendsucc successfully\n";
}

NS_OBJECT_ENSURE_REGISTERED(AquaSimCARMA);

AquaSimCARMA::AquaSimCARMA()
{
	m_k = 5;
	m_pkCount = 1;
	m_senderCount = 1;
	V = 0;
	theta = 0.5;
	m_rand = CreateObject<UniformRandomVariable>();

	Simulator::Schedule(Seconds(rand() % 1400), &AquaSimCARMA::broadinit, this);
}

TypeId
AquaSimCARMA::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::AquaSimCARMA")
							.SetParent<AquaSimRouting>()
							.AddConstructor<AquaSimCARMA>()
							.AddAttribute("TargetID", "ID of target sink.",
										  IntegerValue(31),
										  MakeIntegerAccessor(&AquaSimCARMA::m_targetAddress),
										  MakeIntegerChecker<int>());
	return tid;
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
	std::cout << "================carma TxProcess=================\n";
	CARMAHeader vbh;
	vbh.SetMessType(17);
	vbh.SetM(0);
	vbh.SetN(0);
	vbh.SetForNum(0);
	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	packet->AddHeader(vbh);

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
	std::cout << "Current node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() << "\n";
	std::cout << "table size:" << PktlocalTable.m_htable.size() << std::endl;
	std::cout << "time:" << Now().GetSeconds() << std::endl;

	CARMAHeader vbh;
	AquaSimTrailer tra;

	packet->RemoveHeader(vbh);
	packet->RemoveTrailer(tra);

	if (vbh.GetMessType() == 17)
	{ // no headers //TODO create specalized Application instead of using this hack.// how many times this pkt was forwarded
		tra.SetNextHop(AquaSimAddress::GetBroadcast());
		tra.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		tra.SetDAddr(AquaSimAddress::ConvertFrom(dest));
		tra.SetModeId(1);

		vbh.SetMessType(AS_DATA);
		vbh.SetM(0);
		vbh.SetN(0);
		vbh.clear();
		vbh.SetPkNum((uint16_t)m_senderCount++);
		vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
		vbh.SetValue(V);

		packet->AddHeader(vbh);
		packet->AddTrailer(tra);
		std::cout << "Recv-addheader\n";
	}
	else
	{
		packet->AddHeader(vbh);
		packet->AddTrailer(tra);
	}

	packet->PeekHeader(vbh);
	vbh.Print(std::cout);
	if (vbh.GetForwardAddr() != AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()))
	{
		PktlocalTable.PutInHash(vbh.GetForwardAddr(), vbh.GetValue(), vbh.GetForNum(), vbh.GetP(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt()));
	}
	print();
	if (vbh.GetMessType() != DATA_READY)
	{
		ConsiderNew(packet);
	}

	return true;
}

void AquaSimCARMA::ConsiderNew(Ptr<Packet> pkt)
{
	std::cout << "===========carma: considerNew===========\n";
	std::cout << "time:" << Now().GetSeconds() << std::endl;
	NS_LOG_FUNCTION(this);

	CARMAHeader vbh;
	AquaSimTrailer tra;
	pkt->PeekHeader(vbh);
	// Todo
	// std::cout<<"consider new begin : Advance source position is:"<<vbh.GetExtraInfo().o.x<<","<<vbh.GetExtraInfo().o.y<<","<<vbh.GetExtraInfo().o.z<<'\n';
	// std::cout<<"consider new begin : Advance target position is:"<<tx<<","<<ty<<","<<tz<<'\n';
	// std::cout<<"consider new begin : Advance current position is:"<<pos.x<<","<<pos.y<<","<<pos.z<<'\n';
	// todo end
	AquaSimAddress from_nodeAddr; //, forward_nodeAddr;zhuan fa qi
	std::cout << "Curnode:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() << " receive pkr from node:" << vbh.GetForwardAddr().GetAsInt() << ",packet id:" << pkt->GetUid() << "\n";
	printHopbyhopRecvdelay(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), vbh.GetForwardAddr(), pkt->GetUid());
	NS_LOG_INFO("AquaSimCARMA::ConsiderNew: data packet");
	// todo 打印每个节点的剩余能量
	// from_nodeAddr = vbh.GetSenderAddr();
	from_nodeAddr = vbh.GetForwardAddr();
	// TODO
	// 判断Previous_node是否是自己，是就更新本地表Sendsucc字段（隐式确认）
	if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == vbh.GetPrevioushop())
	{
		PktlocalTable.UpdateSuccnum(vbh.GetPrevioushop(), vbh.GetForwardAddr());
		PktRecv(vbh.GetSenderAddr(), vbh.GetPkNum());
	}
	// 第一个节点发送
	if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == from_nodeAddr)
	{
		std::cout << "Node::" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() << "  m_pkCount++::" << m_pkCount << ",,"
				  << "considerNew_MACsssend\n";
		int status = getstatus(vbh.GetSenderAddr(), vbh.GetPkNum());

		MACprepareF(pkt);
		MACsend(pkt, 0.5);
		printSourceV(m_pkCount);

		return;
	}
	if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() == m_targetAddress) // 接收节点
	{
		std::cout << "Node::" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() << "recv data for sink,packet id:" << pkt->GetUid()
				  << ",At time :" << Simulator::Now().GetSeconds() << "\n";
		double curtime = Simulator::Now().GetSeconds();
		if (getstatusreadonly(vbh.GetSenderAddr(), vbh.GetPkNum()) == Pkt_Not_find)
		{
			std::cout << "SenderAddr" << vbh.GetSenderAddr().GetAsInt() << " PkNum:" << vbh.GetPkNum() << std::endl;
			setPacketRec(vbh.GetSenderAddr(), vbh.GetPkNum());
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
			DataForSink(pkt);
		}

		return;
	}

	// todo
	// 判断nexthop字段是不是自己
	if (IsOneofNexthop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt(), vbh.GetM(), vbh.GetRelay()))
	{
		vbh.Print(std::cout);
		std::cout << "Recv::  I am one of nexthop!\n";
		std::cout << "MyAddr:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() << std::endl;
		int status = getstatus(vbh.GetSenderAddr(), vbh.GetPkNum());
		if (status != Rec && status != Drop)
		{
			MACprepareF(pkt);
			MACsend(pkt, 0.5);
		}

		std::cout << "Node:: " << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() << " is next hop,broadcast it\n";
		return;
	}
	else
	{
		pkt = 0;
		std::cout << "Recv::  I am not nexthop! return!\n";
		return;
	}
}

void AquaSimCARMA::Reset()
{
	PktlocalTable.Reset();
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

	pkt->PeekHeader(vbh);
	int status;
	status = getstatusreadonly(vbh.GetSenderAddr(), vbh.GetPkNum());
	if (status == Rec || status == Drop)
	{
		return;
	}
	pkt->RemoveTrailer(tra);
	pkt->RemoveHeader(vbh);
	// 记录前一跳节点

	AquaSimAddress previous_hop = vbh.GetForwardAddr();
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	// 计算V值
	AquaSimAddress curadd = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());

	uint16_t pktnum;

	std::cout << "status:" << status << std::endl;
	print();
	std::vector<uint8_t> node;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		node.push_back(AquaSimAddress::ConvertFrom(it->first).GetAsInt());
	}
	std::vector<std::vector<uint8_t>> comb;
	for (int i = 1; i <= node.size(); i++) // node是放所有邻节点vector，comb是node组合的结果
	{
		std::vector<uint8_t> temp(i);
		Combination(node, temp, comb, 0, i, i);
	}
	double vis[m_k] = {0};
	std::vector<uint8_t> a;
	// computerelays

	for (int i = 0; i < m_k; i++) // K-s状态
	{
		double curq = INT_FAST64_MAX;
		for (auto j : comb)
		{
			for (auto k : j)
			{
				std::cout << "comb:"
						  << "j:" << (int)k << " ";
			}
			std::cout << std::endl;
			double q;
			double c;
			double e = GetNetDevice()->EnergyModel()->GetTxPower();
			double n = GetNisa(j);
			double p = GetPisa(j); // 失败转移概率
			std::cout << "e:" << e << " n:" << n << " p:" << p << std::endl;
			double sigma = 0;
			if (i == 0) // 说明s=K-1
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
			std::cout << "q:" << q << " c:" << c << " sigma:" << sigma << " curq:" << curq << std::endl;
			std::cout << "status:" << status << " i:" << i << std::endl;
			if (q < curq)
			{
				if (i == status)
				{
					a = j;
				}
				curq = q;
				std::cout << "a:";
				for (auto it : a)
				{
					std::cout << (int)it << " ";
				}
				std::cout << std::endl;
			}
			else if (q == curq)
			{
				if (i == status && j.size() > a.size())
				{
					a = j;
				}
				std::cout << "a:";
				for (auto it : a)
				{
					std::cout << (int)it << " ";
				}
				std::cout << std::endl;
			}
		}
		vis[i] = curq;
	}
	V = vis[4];

	std::cout << "Current_Node: " << curadd << ","
			  << " Vvalue:" << V << "\n";
	for (auto i : a)
	{
		std::cout << "Nexthop" << (int)i << std::endl;
	}
	vbh.clear();
	vbh.SetM((uint8_t)a.size());
	vbh.SetRelay(a);
	vbh.SetValue(V);
	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	vbh.SetPrevioushop(previous_hop);
	vbh.SetN((uint8_t)PktlocalTable.m_htable.size());
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		vbh.SetP(AquaSimAddress::ConvertFrom(it->first).GetAsInt(), it->second->Pji);
	}

	tra.SetNextHop(AquaSimAddress::GetBroadcast());
	tra.SetModeId(1);
	pkt->AddHeader(vbh);
	pkt->AddTrailer(tra);
	return;
}
void AquaSimCARMA::MACsend(Ptr<Packet> pkt, double delay)
{
	std::cout << "-----------carma MACsend-----------\n";
	NS_LOG_INFO("MACsend: delay " << delay << " at time " << Simulator::Now().GetSeconds());

	CARMAHeader vbh;
	AquaSimAddress next_hop = AquaSimAddress::GetBroadcast();
	pkt->RemoveHeader(vbh);
	vbh.SetForNum(m_pkCount++);
	if (vbh.GetMessType() == AS_DATA)
	{
		pkt->AddHeader(vbh);
		Processonpkt(pkt->Copy());
		printHopbyhopSenddelay(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), next_hop, pkt->GetUid());
	}
	else
	{
		pkt->AddHeader(vbh);
	}
	/*Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
						  pkt,AquaSimAddress::GetBroadcast(),Seconds(0));*/
	// first seconnds  how long time to send
	Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this, pkt->Copy(), next_hop, Seconds(0));
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

void AquaSimCARMA::DataForSink(Ptr<Packet> pkt)
{
	CARMAHeader vbh;
	pkt->RemoveHeader(vbh);
	if (!SendUp(pkt))
		NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}
void AquaSimCARMA::printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid)
{
	std::ofstream hopbyhop("Nodehopbyhop.txt", std::ios::app);
	hopbyhop << "PktID:" << pktid << " Node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() << " RecvTime:"
			 << Simulator::Now().GetSeconds() << " from node:" << previous << "\n";
	hopbyhop.close();
}
void AquaSimCARMA::printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid)
{
	std::ofstream hopbyhop("Nodehopbyhop.txt", std::ios::app);
	hopbyhop << "PktID:" << pktid << " Node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()).GetAsInt() << " SendTime:"
			 << Simulator::Now().GetSeconds() << " to node:" << nexthop << "\n";
	hopbyhop.close();
}
void AquaSimCARMA::printEndtoendRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid)
{
	std::cout << "PktID:" << pktid << " Node:" << local.GetAsInt() << "\n";
}
void AquaSimCARMA::printEndtoendSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid)
{
	std::cout << "PktID:" << pktid << " Node:" << local.GetAsInt() << "\n";
}

void AquaSimCARMA::DoDispose()
{
	m_rand = 0;
	AquaSimRouting::DoDispose();
}
bool AquaSimCARMA::IsOneofNexthop(uint16_t addr, uint8_t num, std::vector<uint8_t> relay)
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
	pkt->PeekHeader(vbh);
	int times = getstatusreadonly(vbh.GetSenderAddr(), vbh.GetPkNum());
	vbh.Print(std::cout);
	/*Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
						  pkt,AquaSimAddress::GetBroadcast(),Seconds(0));*/
	// first seconnds  how long time to send
	if (times != Rec && times != Drop)
	{
		Simulator::Schedule(Seconds(20), &AquaSimCARMA::Retransmission, this, pkt->Copy());
	}
	return;
}
int AquaSimCARMA::getstatus(AquaSimAddress addr, uint16_t num)
{
	for (std::map<std::pair<AquaSimAddress, uint16_t>, int>::iterator it = packet.begin(); it != packet.end(); it++)
	{
		if (it->first.first == addr && it->first.second == num)
		{
			if (it->second == Rec || it->second == Drop)
			{
				return it->second;
			}
			it->second--;
			return it->second;
		}
	}
	std::pair<std::pair<AquaSimAddress, uint16_t>, int> newPair;
	newPair.first.first = addr;
	newPair.first.second = num;
	newPair.second = m_k - 1;
	packet.insert(newPair);
	return m_k - 1;
}
int AquaSimCARMA::getstatusreadonly(AquaSimAddress addr, uint16_t num)
{
	for (std::map<std::pair<AquaSimAddress, uint16_t>, int>::iterator it = packet.begin(); it != packet.end(); it++)
	{
		if (it->first.first == addr && it->first.second == num)
		{
			return it->second;
		}
	}
	return Pkt_Not_find;
}
void AquaSimCARMA::setPacketRec(AquaSimAddress addr, uint16_t num)
{
	std::pair<std::pair<AquaSimAddress, uint16_t>, int> newPair;
	newPair.first.first = addr;
	newPair.first.second = num;
	newPair.second = Rec;
	packet.insert(newPair);
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
	pkt->PeekHeader(vbh);
	// todo 打印每个节点的剩余能量
	// from_nodeAddr = vbh.GetSenderAddr();
	// 第一个节点发送
	// come from the same node, broadcast it
	// todo
	// while(m_pkCount<1){//源节点不断发包.m_pkCount<100//设置多久发一次schedule()
	// MACprepareF(pkt);
	int status = getstatus(vbh.GetSenderAddr(), vbh.GetPkNum());
	if (status != Rec && status != Drop)
	{
		MACprepareF(pkt);
		MACsend(pkt, 0.5);
	}

	return;
}
void AquaSimCARMA::PktRecv(AquaSimAddress addr, uint16_t num)
{
	for (std::map<std::pair<AquaSimAddress, uint16_t>, int>::iterator it = packet.begin(); it != packet.end(); it++)
	{
		if (it->first.first == addr && it->first.second == num)
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

	CARMAHeader vbh;
	AquaSimTrailer ast;

	ast.SetModeId(1);
	ast.SetNextHop(AquaSimAddress::GetBroadcast());

	vbh.SetMessType(DATA_READY);
	vbh.SetM(0);
	vbh.SetN(0);
	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	vbh.SetPkNum(1);
	vbh.SetValue(0);
	// vbh.SetM(0);
	// vbh.SetN(0);
	// printf("vectorbasedforward: last line MACprepare\n")
	pkt->AddHeader(vbh);
	pkt->AddTrailer(ast);
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
			if ((uint8_t)AquaSimAddress::ConvertFrom(it->first).GetAsInt() == i)
			{
				P *= (1 - it->second->Pij * it->second->Pji);
			}
		}
	}
	return P;
}
void AquaSimCARMA::print()
{
	std::map<AquaSimAddress, carma_neighbor *>::iterator it;
	for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
	{
		std::cout << "Address:" << it->first.GetAsInt() << " pj:" << it->second->pj << " pji" << it->second->pji << " value:" << it->second->value << " Pij:" << it->second->Pij << " Pji:" << it->second->Pji << std::endl;
	}
}