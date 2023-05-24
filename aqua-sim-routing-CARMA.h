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

#ifndef AQUA_SIM_ROUTING_CARMA_H
#define AQUA_SIM_ROUTING_CARMA_H

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

  class CARMAHeader;

  // carma neighbor
  struct carma_neighbor
  {
    // unsigned int value;
    uint16_t pj = 0;  // the max num of recved pktID from this node
    uint16_t pji = 0; // the num of pkt recved from
    double value;
    double Pij = 0; // From Pkt
    double Pji = 0; // By calculate
  };

  class AquaSimPktlocalTable
  {
  public:
    std::map<AquaSimAddress, carma_neighbor *> m_htable;
    AquaSimPktlocalTable();
    ~AquaSimPktlocalTable();
    int m_windowSize;
    void Reset();
    // void PutInHash(AquaSimAddress fAddr, unsigned int pkNum);
    /*void PutInHash(AquaSimAddress fAddr, unsigned int Vvalue, Vector p,unsigned int energy,unsigned int avgenergy,unsigned int density,
    unsigned int sendnum,unsigned int sendsucc);*/
    void PutInHash(AquaSimAddress fAddr, double Vvalue, uint16_t ppj, double PPij);
    // void UpdateHash(AquaSimAddress nexthop,unsigned int Vvalue);
    void UpdateHash(AquaSimAddress local, double Vvalue);
    void UpdateSuccnum(AquaSimAddress previoushop, AquaSimAddress cur);
    carma_neighbor *GetHash(AquaSimAddress forwarderAddr);
  };
  /**
   * \brief carma
   * JinLin University   author:HanCheng
   */
  class AquaSimCARMA : public AquaSimRouting
  {
  public:
    AquaSimCARMA(); // 初始化本地表//定期调用 创建包发包
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);
    virtual bool TxProcess(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
    void SetTargetPos(Vector pos);
    void broadinit();
    void Retransmission(Ptr<Packet> pkt);
    // AquaSimVBF_Entry routing_table[MAX_DATA_TYPE];

  protected:
    int m_pkCount;
    // int m_useOverhear;
    int m_targetAddress;
    int m_k = 5;
    double V = 0;
    double theta = 0.5;
    // V值计算时公式中的参数
    std::map<int, int> packet;
    // int m_portNumber;
    AquaSimPktlocalTable PktlocalTable;
    // the width is used to test if the node is close enough to the path specified by the packet
    Vector m_targetPos;
    Ptr<UniformRandomVariable> m_rand;

    void Terminate();
    void Reset();
    void ConsiderNew(Ptr<Packet> pkt);
    bool IsTarget(Ptr<Packet>);
    double SuccEnergy(unsigned int energy);                       // 计算传输成功能量
    double DefeatEnergy();                                        // 计算传输失败能量
    double Avgenergy();                                           // 计算节点平均能量
    double Sendsucc(unsigned int sendnum, unsigned int sendsucc); // 计算传输成功概率
    // double SendDefeat();//计算传输失败概率=1-传输成功概率
    void MACprepareF(Ptr<Packet> pkt); // MACPrepareF函数计算V值

    Ptr<Packet> CreatePacket();
    // Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type);
    int iscloseenough(Vector p, Vector q);
    void DataForSink(Ptr<Packet> pkt);
    void StopSource();
    void MACsend(Ptr<Packet> pkt, double delay = 0);
    double getlocalV(AquaSimAddress source);
    int getstatus(int num);
    bool IsOneofNexthop(int addr, uint8_t num, std::vector<uint8_t> relay);
    void Processonpkt(Ptr<Packet> pkt);
    void PktRecv(int num);
    void Combination(std::vector<uint8_t> &a, std::vector<uint8_t> &b, std::vector<std::vector<uint8_t>> &c, int l, int m, int M);
    double GetNisa(std::vector<uint8_t> a);
    double GetLisa(std::vector<uint8_t> a);
    double GetPisa(std::vector<uint8_t> a);
    // print V log
    void printSourceV(int pktsendnum);
    void printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktnum);
    void printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktnum);
    void printEndtoendRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktnum);
    void printEndtoendSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktnum);
    virtual void DoDispose();
  }; // class AquaSimcarma

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_CARMA_H */