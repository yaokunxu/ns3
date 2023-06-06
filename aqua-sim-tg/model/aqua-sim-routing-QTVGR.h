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

#ifndef AQUA_SIM_ROUTING_QTVGR_H
#define AQUA_SIM_ROUTING_QTVGR_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-channel.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"

#include <map>
extern ns3::Vector allNodePositionQTVGR[18000][28];
namespace ns3
{

    class QTVGRHeader;

    // QTVGR neighbor
    struct qtvgr_neighbor
    {
        // unsigned int value;
        double Value;           // 所有邻居节点的V值的最大值
        double HValue = 0;      // Q（A，B，H）查谁跟当前节点Hold的时间最短，作为V（A,C）
        double FValue;          // Q（A，B，F）
        unsigned int T_forward; // 通过通信
        unsigned int energy;
        unsigned int density;
        unsigned int avgenergy;
        unsigned int sendnum;
        unsigned int sendsucc;
    };
    typedef std::pair<AquaSimAddress, Vector> local_entry;

    class AquaSimPktlocalTableQTVGR
    {
    public:
        std::map<local_entry, qtvgr_neighbor *> m_htable;
        AquaSimPktlocalTableQTVGR();
        ~AquaSimPktlocalTableQTVGR();

        int m_windowSize;
        void Reset();
        // void PutInHash(AquaSimAddress fAddr, unsigned int pkNum);
        /*void PutInHash(AquaSimAddress fAddr, unsigned int Vvalue, Vector p,unsigned int energy,unsigned int avgenergy,unsigned int density,
        unsigned int sendnum,unsigned int sendsucc);*/

        void PutInHash(AquaSimAddress fAddr, double V, double Fvalue, Vector p, unsigned int Tt_forward, unsigned int energy, unsigned int avgenergy, unsigned int density,
                       unsigned int sendnum, unsigned int sendsucc);
        // void UpdateHash(AquaSimAddress nexthop,unsigned int Vvalue);
        void UpdateHash(AquaSimAddress local, AquaSimAddress nexthop, double Vvalue, double Hvalue, double Fvalue);
        void UpdateSuccnum(AquaSimAddress previoushop, AquaSimAddress cur);
        void updateOwn(unsigned int Tt_forward, double avgenergy, double density, AquaSimAddress cur);
        qtvgr_neighbor *GetHash(AquaSimAddress forwarderAddr, Vector p);
    };
    /**
     * \ingroup aqua-sim-ng
     *
     * \brief Packet Hash table for QTVGR to assist in specialized tables.
     */
    class AquaSimDataHashTableSQTVGR
    {
    public:
        std::map<int *, int *> m_htable;
        // Tcl_HashTable htable;
        AquaSimDataHashTableSQTVGR();
        ~AquaSimDataHashTableSQTVGR();

        void Reset();
        void PutInHash(int *attr);
        int *GetHash(int *attr);
    }; // class AquaSimDataHashTable

    /**
     * \brief QTVGR
     * JinLin University   author:HanCheng
     */
    class AquaSimQTVGR : public AquaSimRouting
    {
    public:
        AquaSimQTVGR(); // 初始化本地表//定期调用 创建包发包
        static TypeId GetTypeId(void);
        int64_t AssignStreams(int64_t stream);
        virtual bool TxProcess(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
        virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
        double getDensity(unsigned int den); // 计算下一跳节点周围的密度
        Vector getPosition(int t, int nodeid);
        double DoGetDistance(Vector p, Vector q);
        void updateDensity_delete_entry(AquaSimAddress local);
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
        int m_enableRouting; // if true, QTVGR can perform routing functionality. Otherwise, not perform
        // int m_useOverhear;
        double a1, a2, a3, a4;
        double b1, b2, b3, b4;
        double g;
        double m_priority;
        bool m_measureStatus;
        double theta;
        // V值计算时公式中的参数
        // int m_portNumber;
        AquaSimPktlocalTableQTVGR PktlocalTable;

        double m_width;
        // the width is used to test if the node is close enough to the path specified by the packet
        Ptr<UniformRandomVariable> m_rand;
        void Terminate();
        void Reset();
        void ConsiderNew(Ptr<Packet> pkt);
        void SetDelayTimer(Ptr<Packet>, double);
        void Timeout(Ptr<Packet>);
        double SuccEnergy(unsigned int energy);                            // 计算传输成功能量
        double DefeatEnergy();                                             // 计算传输失败能量
        double SuccAvgEnergy(unsigned int energy, unsigned int avgenergy); // 计算传输成功能量分布
        double DefeatAvgEnergy();                                          // 计算传输失败能量分布
        double Avgenergy();
        double Sendsucc(unsigned int sendnum, unsigned int sendsucc); // 计算传输成功概率
        double Distance(Ptr<Packet>);
        void SetMeasureTimer(Ptr<Packet>, double);
        bool IsTarget(Ptr<Packet>);

        double Th(int myid, int nodeid);
        double Td(AquaSimAddress TAddr, AquaSimAddress Curadd, Ptr<Packet> pkt);

        void MACprepareF(Ptr<Packet> pkt); // MACPrepareF函数计算V值
        double calculateAvgEn();           // 计算能量分布
        Ptr<Packet> CreatePacket();
        // Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type);
        void PrepareMessage();
        void iniNodetable();
        void iniNodetable1();
        int iscloseenough(Vector p, Vector q);
        void DataForSink(Ptr<Packet> pkt);
        void StopSource();
        // 发送完后更新自己的表中能量和平均能量值
        void updateEnergyAfterSend(AquaSimAddress local);
        // 接收完后更新自己的表中平均能量值
        void updateAvgEnergyAfterrecv(AquaSimAddress local);
        // 接收以后更新当前节点的密度,删除不在传输范围内的条目

        // print V log
        void MACprepare(Ptr<Packet> pkt);
        void MACsend(Ptr<Packet> pkt, double delay = 0);
        double getlocalV(AquaSimAddress source);
        void printEnergy(AquaSimAddress local, int pktnum);
        void printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktnum);
        void printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktnum);
        void printEndtoendRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktnum);
        void printEndtoendSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktnum);

        int f(Ptr<Packet> pkt, AquaSimAddress source, AquaSimAddress target);
        int LinkDuralTime(AquaSimAddress source, AquaSimAddress target);

        virtual void DoDispose();
    }; // class AquaSimQTVGR

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_QTVGRS_H */