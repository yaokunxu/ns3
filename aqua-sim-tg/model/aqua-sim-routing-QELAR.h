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

#ifndef AQUA_SIM_ROUTING_QELAR_H
#define AQUA_SIM_ROUTING_QELAR_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-channel.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"

#include <map>

namespace ns3
{

  class QELARHeader;

  // qelar neighbor
  struct qelar_neighbor
  {
    // unsigned int value;
    double value;
    unsigned int energy;
    unsigned int avgenergy;
    unsigned int density;
    unsigned int sendnum;
    unsigned int sendsucc;
  };
  typedef std::pair<AquaSimAddress, Vector> local_entry;

  class AquaSimPktlocalTableQELAR
  {
  public:
    std::map<local_entry, qelar_neighbor *> m_htable;
    AquaSimPktlocalTableQELAR();
    ~AquaSimPktlocalTableQELAR();

    int m_windowSize;
    void Reset();
    // void PutInHash(AquaSimAddress fAddr, unsigned int pkNum);
    /*void PutInHash(AquaSimAddress fAddr, unsigned int Vvalue, Vector p,unsigned int energy,unsigned int avgenergy,unsigned int density,
    unsigned int sendnum,unsigned int sendsucc);*/
    void PutInHash(AquaSimAddress fAddr, double Vvalue, Vector p, unsigned int energy, unsigned int avgenergy, unsigned int density,
                   unsigned int sendnum, unsigned int sendsucc);
    // void UpdateHash(AquaSimAddress nexthop,unsigned int Vvalue);
    void UpdateHash(AquaSimAddress local, AquaSimAddress nexthop, double Vvalue);
    void UpdateSuccnum(AquaSimAddress previoushop, AquaSimAddress cur);
    void updateOwn(double avgenergy, double density, AquaSimAddress cur);
    qelar_neighbor *GetHash(AquaSimAddress forwarderAddr, Vector p);
  };
  /**
   * \ingroup aqua-sim-ng
   *
   * \brief Packet Hash table for qelar to assist in specialized tables.
   */
  class AquaSimDataHashTableSQELAR
  {
  public:
    std::map<int *, int *> m_htable;
    // Tcl_HashTable htable;
    AquaSimDataHashTableSQELAR();
    ~AquaSimDataHashTableSQELAR();

    void Reset();
    void PutInHash(int *attr);
    int *GetHash(int *attr);
  }; // class AquaSimDataHashTableQELAR

  /**
   * \brief qelar
   * JinLin University   author:HanCheng
   */
  class AquaSimQELAR : public AquaSimRouting
  {
  public:
    AquaSimQELAR(); // 初始化本地表//定期调用 创建包发包
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);
    virtual bool TxProcess(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    void SetTargetPos(Vector pos);
    // AquaSimVBF_Entry routing_table[MAX_DATA_TYPE];

  protected:
    int m_pkCount;
    int m_counter;
    int m_hopByHop;
    int m_enableRouting; // if true, qelar can perform routing functionality. Otherwise, not perform
    // int m_useOverhear;
    double m_priority;
    bool m_measureStatus;
    // V值计算时公式中的参数
    double a1, a2, a3, a4;
    double b1, b2, b3, b4;
    double g;
    double theta;
    // int m_portNumber;
    AquaSimPktlocalTableQELAR PktlocalTable;

    double m_width;
    // the width is used to test if the node is close enough to the path specified by the packet
    Vector m_targetPos;
    Ptr<UniformRandomVariable> m_rand;

    void Terminate();
    void Reset();
    void ConsiderNew(Ptr<Packet> pkt);
    void SetDelayTimer(Ptr<Packet>, double);
    void Timeout(Ptr<Packet>);
    double Advance(Ptr<Packet>);
    double Distance(Ptr<Packet>);
    double Projection(Ptr<Packet>, Vector p);
    void SetMeasureTimer(Ptr<Packet>, double);
    bool IsTarget(Ptr<Packet>);
    double SuccEnergy(unsigned int energy);                            // 计算传输成功能量
    double DefeatEnergy();                                             // 计算传输失败能量
    double SuccAvgEnergy(unsigned int energy, unsigned int avgenergy); // 计算传输成功能量分布
    double DefeatAvgEnergy();                                          // 计算传输失败能量分布
    double getDensity(unsigned int den);                               // 计算下一跳节点周围的密度
    double Avgenergy();                                                // 计算节点平均能量
    double Sendsucc(unsigned int sendnum, unsigned int sendsucc);      // 计算传输成功概率
    // double SendDefeat();//计算传输失败概率=1-传输成功概率
    void MACprepareF(Ptr<Packet> pkt); // MACPrepareF函数计算V值

    double calculateAvgEn(); // 计算能量分布
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

    // 发送完后更新自己的表中能量和平均能量值
    void updateEnergyAfterSend(AquaSimAddress local);
    // 接收完后更新自己的表中平均能量值
    void updateAvgEnergyAfterrecv(AquaSimAddress local);
    // 接收以后更新当前节点的密度,删除不在传输范围内的条目
    void updateDensity_delete_entry(AquaSimAddress local);
    // print V log
    void printSourceV(int pktsendnum);
    void printEnergy(AquaSimAddress local, int pktnum);
    void printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktnum);
    void printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktnum);
    void printEndtoendRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktnum);
    void printEndtoendSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktnum);
    virtual void DoDispose();
  }; // class AquaSimqelar

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_QELAR_H */
