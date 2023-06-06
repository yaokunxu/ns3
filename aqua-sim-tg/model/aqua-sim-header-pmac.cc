//
// Created by chhhh on 2021/2/26.
//

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

#include "aqua-sim-header-pmac.h"

//testing


using namespace ns3;

PmacHeader::PmacHeader()
{
}

PmacHeader::~PmacHeader()
{
}

TypeId
PmacHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PmacHeader")
            .SetParent<MacHeader>()
            .AddConstructor<PmacHeader>();

    return tid;
}


void
PmacHeader::SetPktType(uint8_t pktType)
{
	m_PktType = pktType;
}


void
PmacHeader::SetPktId(uint8_t pktId)
{
    this->pktId = pktId;
}


int
PmacHeader::GetPktType()
{
    return m_PktType;
}


uint8_t
PmacHeader::GetPktId()
{
    return pktId;
}


uint32_t
PmacHeader::GetSerializedSize(void) const
{
    return 1 + 1 + MacHeader::GetSerializedSize();
}

void
PmacHeader::Serialize (Buffer::Iterator start) const
{
    MacHeader::Serialize(start);

    start.Next(MacHeader::GetSerializedSize());

    start.WriteU8 (m_PktType);
    start.WriteU8 (pktId);
}

uint32_t
PmacHeader::Deserialize (Buffer::Iterator start)
{

    MacHeader::Deserialize(start);

    start.Next(MacHeader::GetSerializedSize());

    Buffer::Iterator i = start;

    m_PktType = i.ReadU8();
    pktId = i.ReadU8();

    return GetSerializedSize();
}

void
PmacHeader::Print (std::ostream &os) const
{
    switch(m_PktType)
    {
        case DATA: os << "DATA"; break;
        case ACK: os << "ACK"; break;
    }
    os << "\n";
}

TypeId
PmacHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}






PmacInitHeader::PmacInitHeader()
{
}

PmacInitHeader::~PmacInitHeader()
{
}

TypeId
PmacInitHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PmacInitHeader")
            .SetParent<Header>()
            .AddConstructor<PmacInitHeader>()
    ;
    return tid;
}



uint32_t
PmacInitHeader::GetSerializedSize(void) const
{
    return 7 * 8;
}

void
PmacInitHeader::Serialize (Buffer::Iterator start) const
{
    start.WriteU64 (sendProbe);
    start.WriteU64 (recvProbe);
    start.WriteU64 (sendAckProbe);
    start.WriteU64 (recvAckProbe);
    start.WriteU64 (maxSlot);
    start.WriteU64 (sendTimeAlignToStartTime);
    start.WriteU64 (token);
}

uint32_t
PmacInitHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    sendProbe = i.ReadU64();
    recvProbe = i.ReadU64();
    sendAckProbe = i.ReadU64();
    recvAckProbe = i.ReadU64();
    maxSlot = i.ReadU64();
    sendTimeAlignToStartTime = i.ReadU64();
    token = i.ReadU64();

    return GetSerializedSize();
}

void
PmacInitHeader::Print (std::ostream &os) const
{
    os << "\n";
}

TypeId
PmacInitHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

