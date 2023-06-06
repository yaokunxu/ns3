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

#include "aqua-sim-routing-QTVGR.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-propagation.h"
#include "aqua-sim-trailer.h"
#include "aqua-sim-datastructure.h"
#include "ns3/seq-ts-header.h"
#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include "ns3/data-rate.h"
#include "ns3/node-list.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <time.h>
#include <math.h>
using namespace ns3;

Vector allNodePositionQTVGR[18000][28];

NS_LOG_COMPONENT_DEFINE("AquaSimQTVGR");
// NS_OBJECT_ENSURE_REGISTERED(AquaSimPktHashTable);
// todo add timeer
// clock_t start,finish;
AquaSimPktlocalTableQTVGR::AquaSimPktlocalTableQTVGR()
{
    NS_LOG_FUNCTION(this);
    m_windowSize = WINDOW_SIZE;
}
AquaSimPktlocalTableQTVGR::~AquaSimPktlocalTableQTVGR()
{
    NS_LOG_FUNCTION(this);
    for (std::map<local_entry, qtvgr_neighbor *>::iterator it = m_htable.begin(); it != m_htable.end(); ++it)
    {
        delete it->second;
    }
    m_htable.clear();
}
void AquaSimPktlocalTableQTVGR::Reset()
{
    m_htable.clear();
}
qtvgr_neighbor *
AquaSimPktlocalTableQTVGR::GetHash(AquaSimAddress forwarderAddr, Vector p)
{
    // std::cout<<"++++++GetHash+++++++++++++\n";
    local_entry entry = std::make_pair(forwarderAddr, p);

    std::map<local_entry, qtvgr_neighbor *>::iterator it;

    it = m_htable.find(entry);
    if (it == m_htable.end())
        return NULL;
    return it->second;
}

void AquaSimPktlocalTableQTVGR::PutInHash(AquaSimAddress fAddr, double V, double Fvalue, Vector p, unsigned int Tt_forward, unsigned int Energy, unsigned int Avgenergy, unsigned int Density, unsigned int sendnum, unsigned int sendsucc)
{
    NS_LOG_DEBUG("PutinHash begin:" << fAddr << ","
                                    << "," << Fvalue << ",(" << p.x << "," << p.y << "," << p.z << ")");
    // std::cout<<"   Putinhash++++++++++++++    \n ";
    // std::cout<<"PutinHash begin:" << fAddr << "," << Vvalue << ",(" << p.x << "," << p.y << "," << p.z << ")"<<"\n";
    local_entry entry = std::make_pair(fAddr, p);
    qtvgr_neighbor *hashptr;
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    // todo //表中已有该节点信息，更新,(因为位置包含在map的key值中，所以只能删除原来的再加入新的)
    for (it = m_htable.begin(); it != m_htable.end(); it++)
    {
        local_entry cur = it->first;
        qtvgr_neighbor *nei = it->second;
        if (cur.first == fAddr)
        {
            // std::cout<<"hasptr exit,update\n";
            nei->T_forward = Tt_forward;
            nei->energy = Energy;
            nei->avgenergy = Avgenergy;
            nei->density = Density;
            // update Vvalue after recv a packet?
            nei->Value = V;
            nei->FValue = Fvalue;
            // update location
            cur.second = p;
            std::cout << "insert location:" << cur.second.x << " " << cur.second.y << " " << cur.second.z << std::endl;
            return;
        }
    }

    hashptr = new qtvgr_neighbor[1];
    hashptr[0].T_forward = Tt_forward;
    hashptr[0].Value = V;
    hashptr[0].FValue = Fvalue;
    hashptr[0].energy = Energy;
    hashptr[0].avgenergy = Avgenergy;
    hashptr[0].density = Density;
    hashptr[0].sendnum = 0;
    hashptr[0].sendsucc = 0;
    std::pair<local_entry, qtvgr_neighbor *> newPair;
    newPair.first = entry;
    newPair.second = hashptr;
    // std::cout<<"position:"<<entry.first<<"\n";
    //  std::cout<<m_htable.size()<<"......before insert.......\n";
    m_htable.insert(newPair);
    // std::cout<<m_htable.size()<<"......after insert.......\n";
    /*for(it=m_htable.begin();it!=m_htable.end();it++){
        QTVGR_neighbor *h=it->second;
        local_entry add=it->first;
        std::cout<<"insert Addr:"<<add.first<<"\n";
        std::cout<<"insert energy:"<<h->energy<<",insert value:"<<h->value<<", insert density: "<<h->density<<"......after insert.......\n";
    }*/
    //}
}

void AquaSimPktlocalTableQTVGR::UpdateHash(AquaSimAddress local, AquaSimAddress nexthop, double Vvalue, double Hvalue, double Fvalue)
{ // 更新V值和Sendnum
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    for (it = m_htable.begin(); it != m_htable.end(); it++)
    {
        local_entry update = it->first;
        qtvgr_neighbor *neibor = it->second;
        if (update.first == local)
        {
            neibor->Value = Vvalue;
            neibor->HValue = Hvalue;
            neibor->FValue = Fvalue;
            // std::cout<<"beibor->Vvalue:"<<neibor->value<<", Vvalue:"<<Vvalue<<"  ,Update vvalue successfully\n";
        }
    }
}

void AquaSimPktlocalTableQTVGR::updateOwn(unsigned int Tt_forward, double avgengy, double denty, AquaSimAddress cur)
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    for (it = m_htable.begin(); it != m_htable.end(); it++)
    {
        local_entry update = it->first;
        qtvgr_neighbor *neibor = it->second;
        if (update.first == cur)
        {
            neibor->T_forward = Tt_forward;
            neibor->avgenergy = avgengy;
            neibor->density = denty;
        }
    }
    // std::cout<<"average:"<<avgengy<<" ,density: "<<denty<<",Update density avgengy successfully\n";
}

AquaSimDataHashTableSQTVGR::AquaSimDataHashTableSQTVGR()
{
    NS_LOG_FUNCTION(this);
    Reset();
    // Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
}

AquaSimDataHashTableSQTVGR::~AquaSimDataHashTableSQTVGR()
{
    NS_LOG_FUNCTION(this);
    Reset();
}

void AquaSimDataHashTableSQTVGR::Reset()
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

int *AquaSimDataHashTableSQTVGR::GetHash(int *attr)
{
    std::map<int *, int *>::iterator it;
    it = m_htable.find(attr);
    if (it == m_htable.end())
        return NULL;

    return it->second;
}

void AquaSimDataHashTableSQTVGR::PutInHash(int *attr)
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

NS_OBJECT_ENSURE_REGISTERED(AquaSimQTVGR);

AquaSimQTVGR::AquaSimQTVGR()
{
    // Initialize variables.
    //  printf("VB initialized\n");
    m_pkCount = 0;
    // TODO
    m_width = 4000;
    m_dataRate = 80;
    m_packetSize = 32;
    m_nodenum = 27;
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
    m_rand = CreateObject<UniformRandomVariable>();
    // todo定期调用 创建包发包//初始化本地表
    /// 延迟一段时间构造，因为可能节点信息还没有创建
    /*Simulator::Schedule(Seconds(0.5),
                 &AquaSimQTVGR::iniNodetable, this);*/
    Simulator::Schedule(Seconds(0.5),
                        &AquaSimQTVGR::iniNodetable1, this);
    Simulator::Schedule(Seconds(rand() % 50), &AquaSimQTVGR::PrepareMessage, this);
}

TypeId
AquaSimQTVGR::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::AquaSimQTVGR")
                            .SetParent<AquaSimRouting>()
                            .AddConstructor<AquaSimQTVGR>()
                            .AddAttribute("HopByHop", "Hop by hop QTVGR setting. Default 0 is false.",
                                          IntegerValue(0),
                                          MakeIntegerAccessor(&AquaSimQTVGR::m_hopByHop),
                                          MakeIntegerChecker<int>())
                            .AddAttribute("EnableRouting", "Enable routing QTVGR setting. Default 1 is true.",
                                          IntegerValue(1),
                                          MakeIntegerAccessor(&AquaSimQTVGR::m_enableRouting),
                                          MakeIntegerChecker<int>())
                            .AddAttribute("Width", "Width of QTVGR. Default is 100.",
                                          DoubleValue(3000),
                                          // DoubleValue(100),
                                          MakeDoubleAccessor(&AquaSimQTVGR::m_width),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("Nodenum", "the num of the simulate num",
                                          IntegerValue(27),
                                          MakeIntegerAccessor(&AquaSimQTVGR::m_nodenum),
                                          MakeIntegerChecker<int>());
    return tid;
    // bind("m_useOverhear_", &m_useOverhear);
}

int64_t
AquaSimQTVGR::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_rand->SetStream(stream);
    return 1;
}
// 传输层调用Txprocess
bool AquaSimQTVGR::TxProcess(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
    QTVGRHeader vbh;
    // start=clock();
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
    vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
    packet->AddHeader(vbh);
    // LOG(INFO) << "start the packet print" << std::endl;
    // packet->Print(std::cout);
    //  todo add send DATA_READY packet
    //  MACprepare(packet);
    //  MACsend(packet,0);
    //  send time add to Nodedelay.txt
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
bool AquaSimQTVGR::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this);
    std::cout << "================QTVGR Recv=================\n";
    // LOG(INFO) << Simulator::Now().GetSeconds() << std::endl;
    std::cout << "Current node:" << AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() << "\n";
    // std::cout << "t:" << Simulator::Now().GetSeconds() << " pos:" << m_device->GetPosition() << std::endl;
    //  AquaSimEnergyModel en;
    //  en.GetEnergy();
    //  std::cout<<"QTVGR energy "<<en.GetEnergy();
    QTVGRHeader vbh;
    AquaSimTrailer tra;

    packet->RemoveHeader(vbh);
    packet->RemoveTrailer(tra);
    /*if (vbh.GetMessType() == DATA_READY) //no use
    {
        // 收到信息包，更新表
        std::cout << "recv message DATA_READY on QTVGR Recv\n";
        // 接到的包先将包信息插入(表中有信息则更新，没有就插入所有)
        //  QTVGR_neighbor *hash=PktlocalTable.GetHash(vbh.GetSenderAddr(),vbh.GetExtraInfo().f);
        //  todo
        PktlocalTable.PutInHash(vbh.GetSenderAddr(), vbh.GetValue(), vbh.GetHValue(), vbh.GetFValue(), vbh.GetExtraInfo().f, vbh.GetT_hold(), vbh.GetT_forward());

        LOG(INFO)<<"vbh.GetMessType() == DATA_READY"<<std::endl;
        return true;
    }*/
    if (vbh.GetMessType() != AS_DATA && vbh.GetMessType() != FLOODING) // data packet add the header 只有源节点第一次发包走这里
    {                                                                  // no headers //TODO create specalized Application instead of using this hack.// how many times this pkt was forwarded
        tra.SetNextHop(AquaSimAddress::GetBroadcast());
        tra.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        tra.SetDAddr(AquaSimAddress::ConvertFrom(dest));
        tra.SetModeId(1);
        vbh.SetMessType(AS_DATA);
        vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
        vbh.SetPkNum(packet->GetUid());
        vbh.SetT(0);
        vbh.SetMType(1);
        // todo
        vbh.SetResiEnergy(GetNetDevice()->EnergyModel()->GetEnergy());
        vbh.SetAvgEnergy(Avgenergy());
        vbh.SetDensity(PktlocalTable.m_htable.size());

        Ptr<Object> sObject = GetNetDevice()->GetNode();
        Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel>();

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

    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,QTVGR_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        if (curentry.first == vbh.GetSenderAddr())
        {
            neibor->T_forward = vbh.GetT_forward();
            neibor->energy = vbh.GetResiEnergy();
            neibor->avgenergy = vbh.GetAvgEnergy();
            neibor->density = vbh.GetDensity();
        }
    }

    // 以后有机会再考虑移动的
    //  updateDensity_delete_entry(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    //  std::cout<<"insert successfully---------------\n";

    // 加入位置信息然后considerNew然后considerNew
    Ptr<Object> sObject = GetNetDevice()->GetNode();
    Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel>();
    // Todo
    // std::cout<<"position.x:"<<sModel->GetPosition().x<<"position.y:"<<sModel->GetPosition().y<<"position.z:"<<sModel->GetPosition().z<<std::endl;
    // todo end
    updateAvgEnergyAfterrecv(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    uint8_t messagetype = vbh.GetMessType();
    // d:关于接收方与转发器的相对位置的信息
    packet->RemoveTrailer(tra);
    packet->RemoveHeader(vbh);

    packet->AddHeader(vbh);
    packet->AddTrailer(tra);
    if (messagetype != FLOODING)
    {
        ConsiderNew(packet); // 选择转发下一跳，判断节点是不是转发节点或目标节点}
    }

    return true;
}

void AquaSimQTVGR::ConsiderNew(Ptr<Packet> pkt)
{
    std::cout << "===========QTVGR: considerNew===========\n";
    NS_LOG_FUNCTION(this);
    QTVGRHeader vbh;
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
    std::cout << "t:" << Simulator::Now().GetSeconds() << " Curnode:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " receive pkr from node:"
              << vbh.GetForwardAddr().GetAsInt() << ",packet id:" << pkt->GetUid() << "\n";

    // todo add hop to hop delay
    printHopbyhopRecvdelay(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), vbh.GetForwardAddr(), pkt->GetUid());
    NS_LOG_INFO("AquaSimQTVGR::ConsiderNew: data packet");
    // todo 打印每个节点的剩余能量
    // from_nodeAddr = vbh.GetSenderAddr();
    printEnergy(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), pkt->GetUid());
    from_nodeAddr = vbh.GetForwardAddr();
    std::cout << "GetPrevioushop:" << vbh.GetPrevioushop() << " from_nodeAddr:" << from_nodeAddr << " TargetAddr:" << vbh.GetTargetAddr() << " NethopAddr:" << vbh.GetNexthopAddr() << std::endl;
    std::cout << "GetNetDevice()->GetAddress() == vbh.GetPrevioushop()" << (GetNetDevice()->GetAddress() == vbh.GetPrevioushop()) << std::endl;
    std::cout << "GetNetDevice()->GetAddress() == from_nodeAddr" << (GetNetDevice()->GetAddress() == from_nodeAddr) << std::endl;
    std::cout << "GetNetDevice()->GetAddress() == vbh.GetTargetAddr()" << (GetNetDevice()->GetAddress() == vbh.GetTargetAddr()) << std::endl;
    std::cout << "GetNetDevice()->GetAddress() == vbh.GetNexthopAddr()" << (GetNetDevice()->GetAddress() == vbh.GetNexthopAddr()) << std::endl;
    // TODO
    // 判断Previous_node是否是自己，是就更新本地表Sendsucc字段（隐式确认）
    if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == vbh.GetPrevioushop())
    {
        PktlocalTable.UpdateSuccnum(vbh.GetPrevioushop(), vbh.GetForwardAddr());
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
        Simulator::Schedule(Seconds(0),
                            &AquaSimQTVGR::MACprepareF, this, pkt);
        // MACsend(pkt,0.5);//macsend比Macprepare提前执行了
        int t = vbh.GetT();
        int mode = vbh.GetMType();
        if (mode == 0)
        {
            std::cout << "hold" << std::endl;
            Simulator::Schedule(Seconds(0.5 + t), &AquaSimQTVGR::MACsend, this, pkt, 0);
        }
        else
        {
            std::cout << "forward" << std::endl;
            Simulator::Schedule(Seconds(0.5), &AquaSimQTVGR::MACsend, this, pkt, 0);
        }
        Simulator::Schedule(Seconds(0.5), &AquaSimQTVGR::updateEnergyAfterSend, this,
                            AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        // Simulator::Schedule(Seconds(t), &AquaSimQTVGR::MACsend, this, pkt, 0.5);
        //   发送完后更新自己的表中能量和平均能量值
        //    todo
        //    updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        //   把source节点的V值单独打印log
        //   printSourceV();
        //  }
        return;
    }
    if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == vbh.GetTargetAddr()) // 接收节点
    {
        std::cout << "Node::" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << "recv data for sink,packet id:" << pkt->GetUid()
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
    if (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == vbh.GetNexthopAddr())
    {
        MACprepareF(pkt);
        int mode = vbh.GetMType();
        double t = vbh.GetT();
        if (mode == 0)
        {
            std::cout << "hold" << std::endl;
            MACsend(pkt, t + 0.5);
        }
        else
        {
            std::cout << "forward" << std::endl;
            MACsend(pkt, 0.5);
        }
        updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        // 发送完后更新自己的表中能量和平均能量值
        //  todo
        std::cout << "Node:: " << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " is next hop,broadcast it\n";
        return;
    }
    else
    {
        // 判断nexthop字段不是自己,节点移动了,其他节点获取不到移动信息,可不可以在接受以后,如果nexthop不是自己,自己就广播自己的信息包
        //  TODO
        pkt = 0;
        // MACprepare(pkt);
        std::cout << "Recv::  I am not nexthop! return!!!!!!!\n";
        return;
    }
}

void AquaSimQTVGR::Reset()
{
    PktlocalTable.Reset();
    /*
       for (int i=0; i<MAX_DATA_TYPE; i++) {
       routing_table[i].Reset();
       }
     */
}

void AquaSimQTVGR::Terminate()
{
    NS_LOG_DEBUG("AquaSimQTVGR::Terminate: Node=" << GetNetDevice()->GetAddress() << ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() << ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
}

void AquaSimQTVGR::StopSource()
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
AquaSimQTVGR::CreatePacket()
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> pkt = Create<Packet>();

    if (pkt == NULL)
        return NULL;

    QTVGRHeader vbh;
    AquaSimTrailer tra;
    // ash.SetSize(36);
    // vbh.SetTs(Simulator::Now().ToDouble(Time::S));

    //!! I add new part

    pkt->AddHeader(vbh);
    pkt->AddTrailer(tra);
    return pkt;
}

void AquaSimQTVGR::PrepareMessage()
{
    // std::cout << "-----------qlr PrepareMessage-----------\n";
    Ptr<Packet> pkt = Create<Packet>();
    // std::cout << "-----------QTVGR PrepareMessage Create pkt-----------\n";
    QTVGRHeader vbh;
    AquaSimTrailer ast;
    ast.SetModeId(1);
    ast.SetSAddr(AquaSimAddress::GetBroadcast());
    ast.SetDAddr(AquaSimAddress::GetBroadcast());
    // std::cout << "-----------ast.SetNextHop-----------\n";
    ast.SetNextHop(AquaSimAddress::GetBroadcast());
    // std::cout << "-----------ast.SetNexthopAddr-----------\n";
    vbh.SetNexthopAddr(AquaSimAddress::GetBroadcast());
    // todo set 模式号
    // 信息包
    vbh.SetMessType(FLOODING);
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetNexthopAddr(AquaSimAddress::GetBroadcast());
    vbh.SetResiEnergy(GetNetDevice()->EnergyModel()->GetEnergy());
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,QTVGR_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *myself = it->second;
        if (curentry.first == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()))
        {

            vbh.SetT_forward(myself->T_forward + theta * myself->Value);
            vbh.SetAvgEnergy(myself->avgenergy);
            vbh.SetDensity(myself->density);
        }
    }
    // std::cout << "vbh.SetSenderAddr:" << vbh.GetSenderAddr() << "\n";
    //  printf("vectorbasedforward: last line MACprepare\n");

    pkt->AddHeader(vbh);
    // std::cout << "-----------pkt->AddHeader(vbh);-----------\n";
    pkt->AddTrailer(ast);
    // std::cout << "-----------pkt->AddTrailer(ast);;-----------\n";
    MACsend(pkt, 0.5);
    // 发送完后更新自己的表中能量和平均能量值
    //  todo
    updateEnergyAfterSend(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    Simulator::Schedule(Seconds(rand() %50),
                        &AquaSimQTVGR::PrepareMessage, this); // 定期调用自己
                                                              //  MACprepare(pkt);
                                                              // return pkt;
}

void AquaSimQTVGR::MACprepareF(Ptr<Packet> pkt)
{
    std::cout << "-----------QTVGR MACprepareF-----------\n";
    QTVGRHeader vbh;
    AquaSimTrailer tra;
    pkt->RemoveTrailer(tra);
    pkt->RemoveHeader(vbh);
    // 记录前一跳节点
    AquaSimAddress previous_hop = vbh.GetForwardAddr();
    AquaSimAddress from_addr = vbh.GetPrevioushop();
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    // 计算V值
    AquaSimAddress curadd = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
    int myid = curadd.GetAsInt();
    double HV = INT16_MIN; // INT8_MIN===== -127
    double FV = INT16_MIN;
    double V = INT16_MIN;
    bool flag = 0;
    double waittime;
    int t = Simulator::Now().GetSeconds();
    AquaSimAddress nexthop;
    AquaSimAddress nextNeighbor;

    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,qtvgr_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        uint16_t nodeid = curentry.first.GetAsInt();
        // std::cout<<"local_entry add: "<<curentry.first<<"position:"<<curentry.second.x<<" "<<curentry.second.y<<" "<<curentry.second.z<<"\n";
        if (curentry.first == curadd || curentry.first == previous_hop || curentry.first == from_addr || !f(pkt, curadd, curentry.first))
        { // 不能计算自己,不能计算它的前一跳节点的V值
            continue;
        }
        if (!iscloseenough(GetNetDevice()->GetPosition(), getPosition(t, nodeid)))
        {
            double th = -1 * Th(myid, nodeid);
            /*if (th != -32767)
            {
                std::cout << "nodeid:" << nodeid << " t:" << t << " pos1:" << getPosition(t - th, nodeid) << " pos2:" << getPosition(t - th, myid) << std::endl;
            }*/
            double VAC = neibor->HValue >= neibor->FValue ? neibor->HValue : neibor->FValue; // the max value of (ACH and ACF)
            std::cout << "HValue " << neibor->HValue << std::endl;
            std::cout << "FValue " << neibor->FValue << std::endl;
            std::cout << "VAC " << VAC << std::endl;
            double succEn = SuccEnergy(neibor->energy);
            double defEn = DefeatEnergy();

            double SuccAvgEn = SuccAvgEnergy(neibor->energy, neibor->avgenergy);
            double DefeatAvgEn = DefeatAvgEnergy();
            double Density = getDensity(neibor->density);
            double Psucc = Sendsucc(neibor->sendnum, neibor->sendsucc);
            double Pdefeat = 1 - Psucc;
            std::cout << "Node:" << curentry.first << " Vvalue  succEn:" << succEn << ","
                      << "defEn:" << defEn << ","
                      << "defAvgEn:" << DefeatAvgEn << ","
                      << "succAvgen:" << SuccAvgEn << ","
                      << "Density:" << Density
                      << ","
                      << "Psucc:" << Psucc << ","
                      << "Pdefeat:" << Pdefeat << "\n";

            double rh = th + theta * VAC;

            double curHV = Psucc * (-g + a1 * succEn + a2 * SuccAvgEn + a3 * rh + a4 * Density) + Pdefeat * (-g + b1 * defEn + b2 * DefeatAvgEn + b3 * rh + b4 * Density);
            std::cout << "Cur_Node:" << curadd << "  calculate Table_node:" << curentry.first << " 's "
                      << "th:" << th << ", curHV:" << curHV << "\n";
            if (curHV > HV)
            {
                waittime = -1 * th;
                HV = curHV;
                nextNeighbor = curentry.first;
                std::cout << "Cur_Node:" << curadd << "  nextNeighbor:" << curentry.first << " 's "
                          << "HV " << HV << ", VAC:" << VAC << "\n";
            }
        }
        else
        {
            std::cout << "nodeid:" << nodeid << " F:" << neibor->FValue << " FV:" << FV << std::endl;
            std::cout << "p:" << GetNetDevice()->GetPosition() << " forecast:" << getPosition(t, myid) << " node:" << getPosition(t, nodeid) << std::endl;
            std::cout << "distance:" << DoGetDistance(GetNetDevice()->GetPosition(), getPosition(t, nodeid)) << std::endl;
            double succEn = SuccEnergy(neibor->energy);
            double defEn = DefeatEnergy();
            double SuccAvgEn = SuccAvgEnergy(neibor->energy, neibor->avgenergy);
            double DefeatAvgEn = DefeatAvgEnergy();
            double Density = getDensity(neibor->density);
            double Psucc = Sendsucc(neibor->sendnum, neibor->sendsucc);
            double Pdefeat = 1 - Psucc;
            std::cout << "Node:" << curentry.first << " Vvalue  succEn:" << succEn << ","
                      << "defEn:" << defEn << ","
                      << "defAvgEn:" << DefeatAvgEn << ","
                      << "succAvgen:" << SuccAvgEn << ","
                      << "Density:" << Density
                      << ","
                      << "Psucc:" << Psucc << ","
                      << "Pdefeat:" << Pdefeat << "\n";
            double curFV = Psucc * (-g + a1 * succEn + a2 * SuccAvgEn + a3 * neibor->FValue + a4 * Density) + Pdefeat * (-g + b1 * defEn + b2 * DefeatAvgEn + b3 * neibor->FValue + b4 * Density);
            if (curFV > FV)
            {
                FV = curFV;
                nexthop = curentry.first;
            }
        }
    }
    // double curV=Rt+theta*((-1)*Pdefeat*curnode->value+(-1)*Psucc*neibor->value);
    // std::cout<<"neibor->value:"<<neibor->value<<"\n";
    std::cout << "Cur_Node:" << curadd << " V " << V << ", Hvalue:" << HV << ", Fvalue:" << FV << "\n";
    if (FV > V)
    {
        std::cout << "FV>V" << std::endl;
        V = FV;
        flag = 1;
        vbh.SetMType(1);
        vbh.SetT(0);
    }
    if (HV > V)
    {
        V = HV;
        std::cout << "waittime:" << waittime << std::endl;
        vbh.SetMType(0);
        vbh.SetT(waittime);
        flag = 0;
        nexthop = nextNeighbor;
        // Simulator::Schedule(Seconds(0.5), &AquaSimqtvgr::MACsend, this, pkt, 0.5);
        //  Simulator::Schedule(Seconds(t), &AquaSimqtvgr::MACsend, this, pkt);
    }
    if (flag == 1)
    {
        for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
        {
            // std::pair<local_entry,qtvgr_neighbor*> curpair;
            local_entry curentry = it->first;
            qtvgr_neighbor *neibor = it->second;
            // std::cout<<"local_entry add: "<<curentry.first<<"position:"<<curentry.second.x<<" "<<curentry.second.y<<" "<<curentry.second.z<<"\n";
            if (curentry.first == curadd || curentry.first == previous_hop)
            { // 不能计算自己,不能计算它的前一跳节点的V值
                continue;
            }
            if (curentry.first == nexthop)
            {
                uint16_t nodeid = curentry.first.GetAsInt();

                double tt = Td(curentry.first, curadd, pkt);
                double tf = neibor->T_forward;
                // todo 计算奖励惩罚函数// add  alpha,beita,gama,yita

                std::cout << "tt:" << tt << std::endl;
                std::cout << "tf:" << tf << std::endl;
                double curFV = -tt - tf;

                neibor->FValue = curFV + theta * neibor->Value;
                neibor->Value = neibor->HValue >= neibor->FValue ? neibor->HValue : neibor->FValue;
            }
        }
    }
    else
    {
        for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
        {
            // std::pair<local_entry,qtvgr_neighbor*> curpair;
            local_entry curentry = it->first;
            qtvgr_neighbor *neibor = it->second;
            // std::cout<<"local_entry add: "<<curentry.first<<"position:"<<curentry.second.x<<" "<<curentry.second.y<<" "<<curentry.second.z<<"\n";
            if (curentry.first == curadd || curentry.first == previous_hop)
            { // 不能计算自己,不能计算它的前一跳节点的V值
                continue;
            }
            if (curentry.first == nexthop)
            {
                neibor->HValue = HV;
                neibor->Value = neibor->HValue >= neibor->FValue ? neibor->HValue : neibor->FValue;
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

    // todo    set other info
    // vbh.SetNexthopAddr();
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        local_entry cur = it->first;
        qtvgr_neighbor *own = it->second;
        if (cur.first == curadd)
        {
            vbh.SetT_forward(own->FValue + theta * own->Value);
            vbh.SetAvgEnergy(own->avgenergy);
            vbh.SetDensity(own->density);
            vbh.SetResiEnergy(own->energy);
            // std::cout<<"LocalAdd:"<<curadd<<", Avgenergy:"<<own->avgenergy<<", Density: "<<own->density<<",ResiEnergy:"<<own->energy
            //<<",Value:"<<own->value<<"\n";
        }
    }
    std::cout << "t:" << vbh.GetT() << " mode:" << (int)vbh.GetMType() << std::endl;
    tra.SetNextHop(AquaSimAddress::GetBroadcast());
    tra.SetModeId(1);

    pkt->AddHeader(vbh);
    pkt->AddTrailer(tra);
    // printf("vectorbasedforward: last line MACprepare\n");
    //     if (flag == 0)
    //     {
    //         Simulator::Schedule(Seconds(0), &AquaSimQTVGR::MACsend, this, pkt, 0.5);
    //     }
}

void AquaSimQTVGR::MACprepare(Ptr<Packet> pkt)
{
    std::cout << "-----------QTVGR MACprepare send DATA_READY-----------\n";
    QTVGRHeader vbh;
    AquaSimTrailer tra;

    pkt->RemoveTrailer(tra);
    pkt->RemoveHeader(vbh);

    // 信息包
    vbh.SetMessType(DATA_READY);
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetNexthopAddr(AquaSimAddress::GetBroadcast());

    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,QTVGR_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        if (curentry.first == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()))
        {
            vbh.SetT_forward(neibor->T_forward);
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
    int mode = vbh.GetMType();
    double t = vbh.GetT();

    pkt->AddHeader(vbh);
    pkt->AddTrailer(tra);
    // printf("vectorbasedforward: last line MACprepare\n");

    if (mode == 0)
    {
        LOG(INFO) << "hold" << std::endl;
        MACsend(pkt, t + 0.5);
    }
    else
    {
        LOG(INFO) << "forward" << std::endl;
        MACsend(pkt, 0.5);
    }

    // 更新能量,更新自己的avgenergy
}

void AquaSimQTVGR::MACsend(Ptr<Packet> pkt, double delay)
{
    std::cout << "-----------QTVGR MACsend-----------\n";
    NS_LOG_INFO("MACsend: delay " << delay << " at time " << Simulator::Now().GetSeconds());

    QTVGRHeader vbh;
    AquaSimTrailer tra;

    pkt->RemoveTrailer(tra);
    pkt->RemoveHeader(vbh);
    AquaSimAddress next_hop = vbh.GetNexthopAddr();
    uint8_t type = vbh.GetMessType();
    pkt->AddHeader(vbh);
    pkt->AddTrailer(tra);
    /*Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
                           pkt,AquaSimAddress::GetBroadcast(),Seconds(0));*/
    // first seconnds  how long time to send
    if (type != FLOODING)
    {
        printHopbyhopSenddelay(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()), next_hop, pkt->GetUid());
    }
    Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this,
                        pkt, next_hop, Seconds(0)); // 发给MAC层
}

double AquaSimQTVGR::getlocalV(AquaSimAddress source)
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    double sourceV = 0.0;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,QTVGR_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        if (curentry.first == source)
        {
            sourceV = neibor->Value;
        }
    }
    return sourceV;
}

void AquaSimQTVGR::iniNodetable()
{
    std::cout << "QTVGR iniNodetable"
              << "\n";
    AquaSimAddress localadd = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
    std::cout << "localadd:" << localadd << "\n";
    Vector p = GetNetDevice()->GetPosition();
    double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
    std::cout << "QTVGR iniNodetable:"
              << "localadd:" << localadd
              << "\n";
    PktlocalTable.PutInHash(localadd, 0, 0, p, 0, LocalResiEnergy, LocalResiEnergy, 0, 0, 0);
    // PrepareMessage();
}

void AquaSimQTVGR::iniNodetable1()
{
    // 初始化自己信息
    //  std::cout<<"QTVGR iniNodetable1"<<"\n";
    AquaSimAddress localadd = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
    // std::cout << localadd << std::endl;
    // std::cout << localadd.GetAsInt() << std::endl;
    int t = Simulator::Now().GetSeconds();

    // std::cout<<"localadd:"<<localadd<<"\n";
    Vector p = GetNetDevice()->GetPosition();
    double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
    // std::cout << "p:" << p << " allNodePositionQTVGR" << allNodePositionQTVGR[t][localadd.GetAsInt()] << " " << allNodePositionQTVGR[t][localadd.GetAsInt() - 1] << " " << allNodePositionQTVGR[t][localadd.GetAsInt() + 1] << std::endl;
    //  std::cout<<"QTVGR iniNodetable:"<<"localadd:"<<localadd<<","<<"LocalResiEnergy:"<<LocalResiEnergy<<"\n";
    PktlocalTable.PutInHash(localadd, 0, 0, p, 0, LocalResiEnergy, LocalResiEnergy, 0, 0, 0);
    // 初始化邻居信息
    int current_node;
    for (int i = 1; i <= m_nodenum; i++)
    {
        if (i != localadd.GetAsInt())
        {
            // std::cout<<"current_node:"<<current_node<<"\n";
            // std::cout << i << " " << localadd.GetAsInt() << std::endl;
            PktlocalTable.PutInHash(i, 0, 0, allNodePositionQTVGR[t][i - 1], 0, LocalResiEnergy, LocalResiEnergy, 0, 0, 0);
            // std::cout<<"QTVGR iniNodetable1 insert successful"<<"\n";
        }
    }

    // 更新自己节点的密度，平均能量
    PktlocalTable.updateOwn(INT8_MAX, Avgenergy(), PktlocalTable.m_htable.size(), localadd);
    // todo 打印每个节点的表
    std::cout << "localadd: " << localadd << " 's table is : "
              << "\n";
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        local_entry cur = it->first;
        qtvgr_neighbor *own = it->second;
        std::cout << cur.first << " , ";
    }
    std::cout << "\n";
}

int AquaSimQTVGR::iscloseenough(Vector p, Vector q)
{
    /*std::cout<<"p.x:"<<p.x<<" p.y:"<<p.y<<" p.z:"<<p.z<<"\n";
    std::cout<<"q.x:"<<q.x<<" q.y:"<<q.y<<" q.z:"<<q.z<<"\n";*/
    double d = sqrt((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y) + (p.z - q.z) * (p.z - q.z));
    // std::cout<<"d:"<<d<<"\n";
    return d > 1500 ? 0 : 1;
}
void AquaSimQTVGR::DataForSink(Ptr<Packet> pkt)
{
    QTVGRHeader vbh;
    pkt->RemoveHeader(vbh);
    if (!SendUp(pkt))
        NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}

void AquaSimQTVGR::SetDelayTimer(Ptr<Packet> pkt, double c)
{
    NS_LOG_FUNCTION(this << c);
    if (c < 0)
        c = 0;
    Simulator::Schedule(Seconds(c), &AquaSimQTVGR::Timeout, this, pkt);
}

void AquaSimQTVGR::Timeout(Ptr<Packet> pkt)
{
    QTVGRHeader vbh;

    pkt->PeekHeader(vbh);

    unsigned char msg_type = vbh.GetMessType();
    std::cout << "Timeout:" << msg_type << "\n";
    // vbf_neighborhood  *hashPtr;
    // Ptr<Packet> p1;
}

/*
double AquaSimQTVGR::SendDefeat(){

}
*/
bool AquaSimQTVGR::IsTarget(Ptr<Packet> pkt)
{
    QTVGRHeader vbh;
    pkt->PeekHeader(vbh);

    // TODO
    // if (vbh.GetTargetAddr().GetAsInt()==1) {
    if (vbh.GetTargetAddr().GetAsInt() == 0)
    {

        //  printf("vectorbased: advanced is %lf and my range is %f\n",Advance(pkt),vbh.GetRange());
        return (true);
    }
    else
        return (AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) == vbh.GetTargetAddr());
}

void AquaSimQTVGR::DoDispose()
{
    m_rand = 0;
    AquaSimRouting::DoDispose();
}

double AquaSimQTVGR::Th(int myid, int nodeid)
{
    double th;
    int t = Simulator::Now().GetSeconds();
    for (int i = t; i <= t + 500; i++)
    {
        Vector p = getPosition(i, nodeid);
        Vector q = getPosition(i, myid);
        if (iscloseenough(p, q))
        {
            th = (double)i - t;
            if (th == 0)
            {
                std::cout << "p:" << p << " MyP:" << q << std::endl;
            }
            return th;
        }
    }
    return (double)INT16_MAX;
}

double AquaSimQTVGR::Td(AquaSimAddress TAddr, AquaSimAddress Curadd, Ptr<Packet> pkt)
{
    int t = Simulator::Now().GetSeconds();
    double d = DoGetDistance(getPosition(t, Curadd.GetAsInt()), getPosition(t, TAddr.GetAsInt()));
    double transmission_delay = d / 1500;
    double propagation_delay = pkt->GetSize() / m_dataRate;
    return transmission_delay + propagation_delay;
}
Vector AquaSimQTVGR::getPosition(int i, int nodeid)
{
    return allNodePositionQTVGR[i][nodeid - 1];
}
double AquaSimQTVGR::DoGetDistance(Vector p, Vector q)
{
    double d = sqrt((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y) + (p.z - q.z) * (p.z - q.z));
    // std::cout<<"d:"<<d<<"\n";
    return d;
}
void AquaSimQTVGR::printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid)
{
    std::ofstream hopbyhop("Nodehopbyhop.txt", std::ios::app);
    hopbyhop << "PktID:" << pktid << " Node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " RecvTime:"
             << Simulator::Now().GetSeconds() << " from node:" << previous << "\n";
    hopbyhop.close();
}
void AquaSimPktlocalTableQTVGR::UpdateSuccnum(AquaSimAddress previous, AquaSimAddress forward)
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    for (it = m_htable.begin(); it != m_htable.end(); it++)
    {
        local_entry update = it->first;
        qtvgr_neighbor *neibor = it->second;
        if (update.first == forward)
        {
            neibor->sendsucc++;
        }
    }
    std::cout << "Previousnode:" << previous << " recv the ACK from node:" << forward << " Update Sendsucc successfully\n";
}
int AquaSimQTVGR::f(Ptr<Packet> pkt, AquaSimAddress source, AquaSimAddress target)
{
    int texp = LinkDuralTime(source, target);
    double td = Td(source, target, pkt);
    int flag = texp - td;
    return flag > 0 ? 1 : 0;
}
int AquaSimQTVGR::LinkDuralTime(AquaSimAddress source, AquaSimAddress target)
{
    int flag1 = 0;
    int flag2 = 0;
    int t = Simulator::Now().GetSeconds();
    int th1 = t;
    int th2 = t;
    for (int i = t; i <= t + 500; i++)
    {
        Vector p = getPosition(i, source.GetAsInt());
        Vector q = getPosition(i, target.GetAsInt());
        if (iscloseenough(p, q))
        {
            th1 = i;
            flag1 = 1;
            break;
        }
    }
    for (int i = th1; i <= t + 500; i++)
    {
        Vector p = getPosition(i, source.GetAsInt());
        Vector q = getPosition(i, target.GetAsInt());
        if (!iscloseenough(p, q))
        {
            th2 = i;
            flag2 = 1;
            break;
        }
    }
    if (flag1 == 0)
    {
        return 0;
    }
    if (flag2 == 0)
    {
        return t + 500 - th1;
    }
    return th2 - th1;
}

double AquaSimQTVGR::SuccEnergy(unsigned int energy)
{
    double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
    double IniEnergy = GetNetDevice()->EnergyModel()->GetInitialEnergy();
    double LocalE = 1 - LocalResiEnergy / IniEnergy;
    double NeighborE = 1 - energy / IniEnergy;
    double res = (-1) * (LocalE + NeighborE);
    return res;
}

double AquaSimQTVGR::DefeatEnergy()
{
    double ResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
    double IniEnergy = GetNetDevice()->EnergyModel()->GetInitialEnergy();
    double res = (-1) * (1 - ResiEnergy / IniEnergy);
    return res;
}

double AquaSimQTVGR::SuccAvgEnergy(unsigned int energy, unsigned int avgenergy)
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    double AvgESi = 0.0;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,qtvgr_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        AvgESi += neibor->energy;
    }
    AvgESi = AvgESi / PktlocalTable.m_htable.size(); // Si节点平均能量
    double ResiEnergySi = GetNetDevice()->EnergyModel()->GetEnergy();
    double ESi = atan(ResiEnergySi - AvgESi) / M_PI; // Si节点能量分布
    double ResiEnergySj = energy;
    double AvgESj = avgenergy;                       // Sj节点平均能量
    double ESj = atan(ResiEnergySj - AvgESj) / M_PI; // Sj节点能量分布
    double AvgE = ESi + ESj;
    return AvgE;
}

double AquaSimQTVGR::DefeatAvgEnergy()
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    double AvgESi = 0.0;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,qtvgr_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        AvgESi += neibor->energy;
    }
    AvgESi = AvgESi / PktlocalTable.m_htable.size(); // Si节点平均能量
    double ResiEnergySi = GetNetDevice()->EnergyModel()->GetEnergy();
    double ESi = atan(ResiEnergySi - AvgESi) / M_PI;
    return ESi;
}

double AquaSimQTVGR::getDensity(unsigned int den)
{
    // std::cout<<"Density:"<<(-1)*(1-(float)den/47)<<"\n";
    // std::cout<<"neibor num/47:"<<(float)den/47<<"\n";
    return (-1) * (1 - (float)den / 27);
}
double AquaSimQTVGR::Avgenergy()
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    double AvgESi = 0.0;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,qtvgr_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        AvgESi += neibor->energy;
    }
    AvgESi = AvgESi / PktlocalTable.m_htable.size(); // Si节点平均能量
    return AvgESi;
}
double AquaSimQTVGR::Sendsucc(unsigned int sendnum, unsigned int sendsucc)
{
    // std::cout<<"sendnum:"<<sendnum<<" ,sendsucc:"<<sendsucc<<"\n";
    if (sendnum == 0)
    {
        return 1;
    }
    return (float)sendsucc / sendnum;
}
void AquaSimQTVGR::printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid)
{
    std::ofstream hopbyhop("Nodehopbyhop.txt", std::ios::app);
    hopbyhop << "PktID:" << pktid << " Node:" << AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) << " SendTime:"
             << Simulator::Now().GetSeconds() << " to node:" << nexthop << "\n";
    hopbyhop.close();
}
void AquaSimQTVGR::printEndtoendRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid)
{
    std::cout << "PktID:" << pktid << " Node:" << local << "\n";
}
void AquaSimQTVGR::printEndtoendSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid)
{
    std::cout << "PktID:" << pktid << " Node:" << local << "\n";
}
void AquaSimQTVGR::updateEnergyAfterSend(AquaSimAddress local)
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
    Vector p = GetNetDevice()->GetNode()->GetObject<MobilityModel>()->GetPosition();
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,qlrp_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        if (curentry.first == local)
        {
            neibor->energy = LocalResiEnergy;
            neibor->avgenergy = calculateAvgEn();
        }
    }
}
double
AquaSimQTVGR::calculateAvgEn()
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    double avgEn = 0;
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,qlrp_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        avgEn += neibor->energy;
    }
    return avgEn / PktlocalTable.m_htable.size();
}
void AquaSimQTVGR::updateAvgEnergyAfterrecv(AquaSimAddress local)
{
    std::map<local_entry, qtvgr_neighbor *>::iterator it;
    double LocalResiEnergy = GetNetDevice()->EnergyModel()->GetEnergy();
    for (it = PktlocalTable.m_htable.begin(); it != PktlocalTable.m_htable.end(); it++)
    {
        // std::pair<local_entry,qlrp_neighbor*> curpair;
        local_entry curentry = it->first;
        qtvgr_neighbor *neibor = it->second;
        if (curentry.first == local)
        {
            neibor->avgenergy = calculateAvgEn();
        }
    }
}
void AquaSimQTVGR::printEnergy(AquaSimAddress local, int pktID)
{
    std::ofstream outfile("NodeResiEnergy.txt", std::ios::app);
    // std::cout<<"LocalNode:"<<local<<",resiEnergy:"<<own->energy<<"\n";
    outfile << /*"PktID:"<<pktID<<*/ "Node:" << local /*<<"  ResiEnergy:"*/ << " " << GetNetDevice()->EnergyModel()->GetEnergy() << "\n";
    // outfile<<local<<" "<<own->energy<<"\n";
    //}
    outfile.close();
}