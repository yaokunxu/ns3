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

#ifndef AQUA_SIM_ROUTING_QDTR_H
#define AQUA_SIM_ROUTING_QDTR_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-channel.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"

#include <map>
extern ns3::Vector allNodePosition[18000][28];
namespace ns3
{

    class QDTRHeader;

    // qdtr neighbor
    struct qdtr_neighbor
    {
        // unsigned int value;
        double Value;           // 所有邻居节点的V值的最大值
        double HValue = 0;      // Q（A，B，H）查谁跟当前节点Hold的时间最短，作为V（A,C）
        double FValue;          // Q（A，B，F）
        unsigned int T_forward; // 通过通信
    };
    typedef std::pair<AquaSimAddress, Vector> local_entry;

    class AquaSimPktlocalTableQDTR
    {
    public:
        std::map<local_entry, qdtr_neighbor *> m_htable;
        AquaSimPktlocalTableQDTR();
        ~AquaSimPktlocalTableQDTR();

        int m_windowSize;
        void Reset();
        // void PutInHash(AquaSimAddress fAddr, unsigned int pkNum);
        /*void PutInHash(AquaSimAddress fAddr, unsigned int Vvalue, Vector p,unsigned int energy,unsigned int avgenergy,unsigned int density,
        unsigned int sendnum,unsigned int sendsucc);*/

        void PutInHash(AquaSimAddress fAddr, double V, double Fvalue, Vector p, unsigned int Tt_forward);
        // void UpdateHash(AquaSimAddress nexthop,unsigned int Vvalue);
        void UpdateHash(AquaSimAddress local, AquaSimAddress nexthop, double Vvalue, double Hvalue, double Fvalue);
        void UpdateSuccnum(AquaSimAddress previoushop, AquaSimAddress cur);
        void updateOwn(unsigned int Tt_forward, AquaSimAddress cur);
        qdtr_neighbor *GetHash(AquaSimAddress forwarderAddr, Vector p);
    };
    /**
     * \ingroup aqua-sim-ng
     *
     * \brief Packet Hash table for qdtr to assist in specialized tables.
     */
    class AquaSimDataHashTableSQDTR
    {
    public:
        std::map<int *, int *> m_htable;
        // Tcl_HashTable htable;
        AquaSimDataHashTableSQDTR();
        ~AquaSimDataHashTableSQDTR();

        void Reset();
        void PutInHash(int *attr);
        int *GetHash(int *attr);
    }; // class AquaSimDataHashTable

    /**
     * \brief qdtr
     * JinLin University   author:HanCheng
     */
    class AquaSimQDTR : public AquaSimRouting
    {
    public:
        AquaSimQDTR(); // 初始化本地表//定期调用 创建包发包
        static TypeId GetTypeId(void);
        int64_t AssignStreams(int64_t stream);
        virtual bool TxProcess(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
        virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

        Vector getPosition(int t, int nodeid);
        double DoGetDistance(Vector p, Vector q);
        void printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktid);
        void printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktid);
        void SetTargetPos(Vector pos);
        // AquaSimVBF_Entry routing_table[MAX_DATA_TYPE];

    protected:
        uint32_t m_dataRate;
        uint32_t m_packetSize;
        double range = 1500;
        int m_pkCount;
        int m_counter;
        int m_hopByHop;
        int m_nodenum;
        int m_enableRouting; // if true, qdtr can perform routing functionality. Otherwise, not perform
        // int m_useOverhear;
        double m_priority;
        bool m_measureStatus;
        double theta;
        // V值计算时公式中的参数
        // int m_portNumber;
        AquaSimPktlocalTableQDTR PktlocalTable;

        double m_width;
        // the width is used to test if the node is close enough to the path specified by the packet
        Ptr<UniformRandomVariable> m_rand;

        void Terminate();
        void Reset();
        void ConsiderNew(Ptr<Packet> pkt);
        void SetDelayTimer(Ptr<Packet>, double);
        void Timeout(Ptr<Packet>);
        double Distance(Ptr<Packet>);
        void SetMeasureTimer(Ptr<Packet>, double);
        bool IsTarget(Ptr<Packet>);

        double Th(int myid, int nodeid);
        double Td(Vector TAddr, Vector Curadd, Ptr<Packet> pkt);

        void MACprepareF(Ptr<Packet> pkt); // MACPrepareF函数计算V值

        Ptr<Packet> CreatePacket();
        // Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type);
        void PrepareMessage();
        void iniNodetable();
        void iniNodetable1();
        int iscloseenough(Vector p, Vector q);
        void DataForSink(Ptr<Packet> pkt);
        void StopSource();
        void MACprepare(Ptr<Packet> pkt);
        void MACsend(Ptr<Packet> pkt, double delay = 0);
        double getlocalV(AquaSimAddress source);
        void printEnergy(AquaSimAddress local, int pktID);

        virtual void DoDispose();
    }; // class AquaSimqdtr

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_qdtrS_H */
