/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *
 * Date:2021.4.7
 * Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */

#ifndef AQUA_SIM_NG_MASTER_AQUA_SIM_HEADER_PMAC_H
#define AQUA_SIM_NG_MASTER_AQUA_SIM_HEADER_PMAC_H


#include <iostream>


#include "ns3/header.h"
#include "ns3/vector.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-header-mac.h"


namespace ns3 {

class PmacHeader : public MacHeader
{
public:
    enum PacketType {
        DATA,
        DATA_ACK,
        ACK,
        PROBE,
        ACK_PROBE,
        TRIGGER,
        ACK_TRIGGER,
        TIME_ALIGN,
        ACK_TIME_ALIGN
    };

    PmacHeader();
    virtual ~PmacHeader();

    static TypeId GetTypeId(void);

    void SetPktType(uint8_t pktType);
    void SetPktId(uint8_t pktId);


    int GetPktType();
    uint8_t GetPktId();

    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
    virtual void Print (std::ostream &os) const;
    virtual TypeId GetInstanceTypeId(void) const;
private:
    uint8_t m_PktType;
    uint8_t pktId;
};  // class PmacHeader


class PmacInitHeader : public Header
{
public:

    PmacInitHeader();
    virtual ~PmacInitHeader();

    static TypeId GetTypeId(void);


    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
    virtual void Print (std::ostream &os) const;
    virtual TypeId GetInstanceTypeId(void) const;



    uint64_t sendProbe;
    uint64_t recvProbe;
    uint64_t sendAckProbe;
    uint64_t recvAckProbe;

    uint64_t maxSlot;

    uint64_t sendTimeAlignToStartTime;

    uint64_t token;


};  // class PmacHeader

}






#endif //AQUA_SIM_NG_MASTER_AQUA_SIM_HEADER_PMAC_H
