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
    uint16_t pj = 0;  // the max num of recved pktID from this node
    uint16_t pji = 0; // the num of pkt recved from
    double value;
    double Pij = 0; // From Pkt
    double Pji = 0; // By calculate
  };

  class AquaSimPktlocalTableCARMA
  {
  public:
    std::map<AquaSimAddress, carma_neighbor *> m_htable;
    AquaSimPktlocalTableCARMA();
    ~AquaSimPktlocalTableCARMA();
    int m_windowSize;
    void Reset();
    void PutInHash(AquaSimAddress fAddr, double Vvalue, uint16_t ppj, double PPij);
    void UpdateHash(AquaSimAddress local, double Vvalue);
    void UpdateSuccnum(AquaSimAddress previoushop, AquaSimAddress cur);
    carma_neighbor *GetHash(AquaSimAddress forwarderAddr);
  };

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

  protected:
    int m_k;//最大的重传次数
    int m_pkCount;
    int m_senderCount;
    int m_targetAddress;
    double V;
    double theta;
    Ptr<UniformRandomVariable> m_rand;
    AquaSimPktlocalTableCARMA PktlocalTable;
    std::map<std::pair<AquaSimAddress, uint16_t>, int> packet;

    void Reset();
    void Terminate();
    void StopSource();
    
    void ConsiderNew(Ptr<Packet> pkt);
    void MACprepareF(Ptr<Packet> pkt);
    void DataForSink(Ptr<Packet> pkt);
    void Processonpkt(Ptr<Packet> pkt);
    void MACsend(Ptr<Packet> pkt, double delay = 0);
    void PktRecv(AquaSimAddress addr,uint16_t num);
    void setPacketRec(AquaSimAddress addr,uint16_t num);
    void Combination(std::vector<uint8_t> &a, std::vector<uint8_t> &b, std::vector<std::vector<uint8_t>> &c, int l, int m, int M);
    double getlocalV(AquaSimAddress source);
    double GetNisa(std::vector<uint8_t> a);
    double GetLisa(std::vector<uint8_t> a);
    double GetPisa(std::vector<uint8_t> a);
    int getstatus(AquaSimAddress addr,uint16_t num);
    int getstatusreadonly(AquaSimAddress addr,uint16_t num);
    bool IsOneofNexthop(uint16_t addr, uint8_t num, std::vector<uint8_t> relay);
    Ptr<Packet> CreatePacket();

    void print();
    void printSourceV(int pktsendnum);
    void printHopbyhopRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktnum);
    void printHopbyhopSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktnum);
    void printEndtoendRecvdelay(AquaSimAddress local, AquaSimAddress previous, int pktnum);
    void printEndtoendSenddelay(AquaSimAddress local, AquaSimAddress nexthop, int pktnum);
    virtual void DoDispose();
  }; // class AquaSimCARMA

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_CARMA_H */