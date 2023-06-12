/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Connecticut
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Robert Martin <robert.martin@engr.uconn.edu>
 */

#include "aqua-sim-header-routing.h"
#include <map>
#include "ns3/log.h"
#include "ns3/buffer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RoutingHeader");
NS_OBJECT_ENSURE_REGISTERED(RoutingHeader);

RoutingHeader::RoutingHeader(/* args */)
{
}

RoutingHeader::~RoutingHeader()
{
}

TypeId
RoutingHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::RoutingHeader")
                          .SetParent<Header>()
                          .AddConstructor<RoutingHeader>();
  return tid;
}
int RoutingHeader::GetSize()
{
  return 4; // src:2  dst:2
}

uint32_t
RoutingHeader::GetSerializedSize(void) const
{
  return GetSize();
}
void RoutingHeader::Serialize(Buffer::Iterator start) const
{
  start.WriteU16(m_sa.GetAsInt());
  start.WriteU16(m_da.GetAsInt());
  start.WriteU16(m_numForwards);
}
uint32_t
RoutingHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_sa = (AquaSimAddress)i.ReadU16();
  m_da = (AquaSimAddress)i.ReadU16();
  m_numForwards = i.ReadU16();
  return GetSerializedSize();
}
void RoutingHeader::Print(std::ostream &os) const
{
  os << "Routing Header is: source = " << m_sa
     << ", destination = " << m_da << "\n";
}
TypeId
RoutingHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void RoutingHeader::SetSA(AquaSimAddress sa)
{
  m_sa = sa;
}
void RoutingHeader::SetDA(AquaSimAddress da)
{
  m_da = da;
}
void RoutingHeader::SetNumForwards(uint16_t numForwards)
{
  m_numForwards = numForwards;
}

AquaSimAddress
RoutingHeader::GetSA()
{
  return m_sa;
}
AquaSimAddress
RoutingHeader::GetDA()
{
  return m_da;
}
uint16_t
RoutingHeader::GetNumForwards()
{
  return m_numForwards;
}

/*
 * Dynamic Routing
 */
// NS_LOG_COMPONENT_DEFINE("DRoutingHeader");
NS_OBJECT_ENSURE_REGISTERED(DRoutingHeader);

DRoutingHeader::DRoutingHeader()
{
}

DRoutingHeader::~DRoutingHeader()
{
}

TypeId
DRoutingHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::DRoutingHeader")
                          .SetParent<Header>()
                          .AddConstructor<DRoutingHeader>();
  return tid;
}

uint32_t
DRoutingHeader::Deserialize(Buffer::Iterator start)
{
  std::cout << "DSerialize" << std::endl;
  Buffer::Iterator i = start;
  m_pktSrc = (AquaSimAddress)i.ReadU16();
  m_pktDst = (AquaSimAddress)i.ReadU16();
  numForward = i.ReadU16();
  m_pktLen = i.ReadU16();
  m_pktSeqNum = i.ReadU16();
  m_entryNum = i.ReadU32();
  std::cout << "DSerialize出" << std::endl;

  return GetSerializedSize();
} // 改

uint32_t
DRoutingHeader::GetSerializedSize(void) const
{
  // reserved bytes for header
  // return (2+3+1+4);
  return (2 + 2 + 2 + 2 + 2 + 4);
} // 改

void DRoutingHeader::Serialize(Buffer::Iterator start) const
{
  std::cout << "Serialize" << std::endl;
  Buffer::Iterator i = start;
  i.WriteU16(m_pktSrc.GetAsInt());
  i.WriteU16(m_pktDst.GetAsInt());
  i.WriteU16(numForward);
  i.WriteU16(m_pktLen);
  i.WriteU16(m_pktSeqNum);
  i.WriteU32(m_entryNum);
  std::cout << "Serialize出" << std::endl;
} // 改

void DRoutingHeader::Print(std::ostream &os) const
{
  os << "Dynamic Routing Header is: PktSrc=" << m_pktSrc << " PktDst=" << m_pktDst << " numForward=" << numForward << " PktLen=" << m_pktLen << " PktSeqNum=" << m_pktSeqNum << " EntryNum=" << m_entryNum << "\n";
}

TypeId
DRoutingHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

AquaSimAddress
DRoutingHeader::GetPktSrc()
{
  return m_pktSrc;
}
AquaSimAddress
DRoutingHeader::GetPktDst()
{
  return m_pktDst;
}
uint16_t
DRoutingHeader::GetNumForwards()
{
  return numForward;
}
uint16_t
DRoutingHeader::GetPktLen()
{
  return m_pktLen;
}
uint16_t
DRoutingHeader::GetPktSeqNum()
{
  return m_pktSeqNum;
}
uint32_t
DRoutingHeader::GetEntryNum()
{
  return m_entryNum;
}

void DRoutingHeader::SetPktSrc(AquaSimAddress pktSrc)
{
  m_pktSrc = pktSrc;
}
void DRoutingHeader::SetPktDst(AquaSimAddress pktDst)
{
  m_pktDst = pktDst;
}
void DRoutingHeader::SetNumForwards(uint16_t number)
{
  numForward = number;
}
void DRoutingHeader::SetPktLen(uint16_t pktLen)
{
  m_pktLen = pktLen;
}
void DRoutingHeader::SetPktSeqNum(uint16_t pktSeqNum)
{
  m_pktSeqNum = pktSeqNum;
}
void DRoutingHeader::SetEntryNum(uint32_t entryNum)
{
  m_entryNum = entryNum;
}

/*
 * Vector Based Routing
 */
NS_OBJECT_ENSURE_REGISTERED(VBHeader);

VBHeader::VBHeader() : m_messType(0)
{
}

VBHeader::~VBHeader()
{
}

TypeId
VBHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::VBHeader")
                          .SetParent<Header>()
                          .AddConstructor<VBHeader>();
  return tid;
}

uint32_t
VBHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_messType = i.ReadU8();
  m_pkNum = i.ReadU32();
  m_targetAddr = (AquaSimAddress)i.ReadU16();
  m_senderAddr = (AquaSimAddress)i.ReadU16();
  m_forwardAddr = (AquaSimAddress)i.ReadU16();
  m_dataType = i.ReadU8();
  m_numForwards = i.ReadU16();
  m_originalSource.x = ((double)i.ReadU32()) / 1000.0;
  m_originalSource.y = ((double)i.ReadU32()) / 1000.0;
  m_originalSource.z = ((double)i.ReadU32()) / 1000.0;
  m_token = ((uint32_t)i.ReadU32()) / 1000.0;
  m_ts = ((uint32_t)i.ReadU32()) / 1000.0;
  m_range = ((uint32_t)i.ReadU32()) / 1000.0;

  // This is bloated.
  m_info.o.x = ((double)i.ReadU32()) / 1000.0;
  m_info.o.y = ((double)i.ReadU32()) / 1000.0;
  m_info.o.z = ((double)i.ReadU32()) / 1000.0;
  m_info.f.x = ((double)i.ReadU32()) / 1000.0;
  m_info.f.y = ((double)i.ReadU32()) / 1000.0;
  m_info.f.z = ((double)i.ReadU32()) / 1000.0;
  m_info.t.x = ((double)i.ReadU32()) / 1000.0;
  m_info.t.y = ((double)i.ReadU32()) / 1000.0;
  m_info.t.z = ((double)i.ReadU32()) / 1000.0;
  m_info.d.x = ((double)i.ReadU32()) / 1000.0;
  m_info.d.y = ((double)i.ReadU32()) / 1000.0;
  m_info.d.z = ((double)i.ReadU32()) / 1000.0;

  return GetSerializedSize();
}

uint32_t
VBHeader::GetSerializedSize(void) const
{
  // reserved bytes for header
  return (1 + 4 + 2 + 2 + 2 + 1 + 2 + 12 + 4 + 4 + 4 + 48);
}

void VBHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_messType);
  i.WriteU32(m_pkNum);
  i.WriteU16(m_targetAddr.GetAsInt());
  i.WriteU16(m_senderAddr.GetAsInt());
  i.WriteU16(m_forwardAddr.GetAsInt());
  i.WriteU8(m_dataType);
  i.WriteU16(m_numForwards);

  // Messy...
  i.WriteU32((uint32_t)(m_originalSource.x * 1000.0 + 0.5)); //+0.5 for uint32_t typecast
  i.WriteU32((uint32_t)(m_originalSource.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_originalSource.z * 1000.0 + 0.5));

  i.WriteU32((uint32_t)(m_token * 1000.0));
  i.WriteU32((uint32_t)(m_ts * 1000.0));
  i.WriteU32((uint32_t)(m_range * 1000.0));

  // bloated.
  i.WriteU32((uint32_t)(m_info.o.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.o.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.o.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.z * 1000.0 + 0.5));
}

void VBHeader::Print(std::ostream &os) const
{
  os << "Vector Based Routing Header is: messType=";
  switch (m_messType)
  {
  case INTEREST:
    os << "INTEREST";
    break;
  case AS_DATA:
    os << "DATA";
    break;
  case DATA_READY:
    os << "DATA_READY";
    break;
  case SOURCE_DISCOVERY:
    os << "SOURCE_DISCOVERY";
    break;
  case SOURCE_TIMEOUT:
    os << "SOURCE_TIMEOUT";
    break;
  case TARGET_DISCOVERY:
    os << "TARGET_DISCOVERY";
    break;
  case TARGET_REQUEST:
    os << "TARGET_REQUEST";
    break;
  case SOURCE_DENY:
    os << "SOURCE_DENY";
    break;
  case V_SHIFT:
    os << "V_SHIFT";
    break;
  case FLOODING:
    os << "FLOODING";
    break;
  case DATA_TERMINATION:
    os << "DATA_TERMINATION";
    break;
  case BACKPRESSURE:
    os << "BACKPRESSURE";
    break;
  case BACKFLOODING:
    os << "BACKFLOODING";
    break;
  case EXPENSION:
    os << "EXPENSION";
    break;
  case V_SHIFT_DATA:
    os << "V_SHIFT_DATA";
    break;
  case EXPENSION_DATA:
    os << "EXPENSION_DATA";
    break;
  }

  os << " pkNum=" << m_pkNum << " targetAddr=" << m_targetAddr << " senderAddr=" << m_senderAddr << " forwardAddr=" << m_forwardAddr << " dataType=" << m_dataType << " originalSource=" << m_originalSource.x << "," << m_originalSource.y << "," << m_originalSource.z << " token=" << m_token << " ts=" << m_ts << " range=" << m_range;

  os << "   ExtraInfo= StartPoint(" << m_info.o << ") ForwardPos(" << m_info.f << ") EndPoint(" << m_info.t << ") RecvToForwarder(" << m_info.d << ")\n";
}

TypeId
VBHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void VBHeader::SetMessType(uint8_t messType)
{
  m_messType = messType;
}
void VBHeader::SetPkNum(uint32_t pkNum)
{
  m_pkNum = pkNum;
}
void VBHeader::SetTargetAddr(AquaSimAddress targetAddr)
{
  m_targetAddr = targetAddr;
}
void VBHeader::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}
void VBHeader::SetForwardAddr(AquaSimAddress forwardAddr)
{
  m_forwardAddr = forwardAddr;
}
void VBHeader::SetDataType(uint8_t dataType)
{
  m_dataType = dataType;
}
void VBHeader::SetNumForwards(uint16_t numforwards)
{
  m_numForwards = numforwards;
}
void VBHeader::SetOriginalSource(Vector originalSource)
{
  m_originalSource = originalSource;
}
void VBHeader::SetToken(uint32_t token)
{
  m_token = token;
}
void VBHeader::SetTs(uint32_t ts)
{
  m_ts = ts;
}
void VBHeader::SetRange(uint32_t range)
{
  m_range = range;
}
void VBHeader::SetExtraInfo(uw_extra_info info)
{
  m_info = info;
}
void VBHeader::SetExtraInfo_o(Vector position_o)
{
  m_info.o = position_o;
}
void VBHeader::SetExtraInfo_f(Vector position_f)
{
  m_info.f = position_f;
}
void VBHeader::SetExtraInfo_t(Vector position_t)
{
  m_info.t = position_t;
}
void VBHeader::SetExtraInfo_d(Vector position_d)
{
  m_info.d = position_d;
}

uint8_t
VBHeader::GetMessType()
{
  return m_messType;
}
uint32_t
VBHeader::GetPkNum()
{
  return m_pkNum;
}
AquaSimAddress
VBHeader::GetTargetAddr()
{
  return m_targetAddr;
}
AquaSimAddress
VBHeader::GetSenderAddr()
{
  return m_senderAddr;
}
AquaSimAddress
VBHeader::GetForwardAddr()
{
  return m_forwardAddr;
}
uint8_t
VBHeader::GetDataType()
{
  return m_dataType;
}
uint16_t
VBHeader::GetNumForwards()
{
  return m_numForwards;
}
Vector
VBHeader::GetOriginalSource()
{
  return m_originalSource;
}
uint32_t
VBHeader::GetToken()
{
  return m_token;
}
uint32_t
VBHeader::GetTs()
{
  return m_ts;
}
uint32_t
VBHeader::GetRange()
{
  return m_range;
}
uw_extra_info
VBHeader::GetExtraInfo()
{
  return m_info;
}

/*
 *  Depth Based Routing Header
 */
NS_OBJECT_ENSURE_REGISTERED(DBRHeader);

DBRHeader::DBRHeader()
{
}

DBRHeader::~DBRHeader()
{
}

TypeId
DBRHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::DBRHeader")
                          .SetParent<Header>()
                          .AddConstructor<DBRHeader>();
  return tid;
}

int DBRHeader::Size()
{
  // not quite right
  /*int sz;
  sz = 4 * sizeof(int);
  sz += 3 * sizeof(double);
  return sz;*/

  return GetSerializedSize();
}

uint32_t
DBRHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_position.x = ((double)i.ReadU32()) / 1000.0;
  m_position.y = ((double)i.ReadU32()) / 1000.0;
  m_position.z = ((double)i.ReadU32()) / 1000.0;
  m_packetID = i.ReadU32();
  m_mode = i.ReadU8();
  m_nhops = i.ReadU16();
  m_prevHop = (AquaSimAddress)i.ReadU16();
  m_owner = (AquaSimAddress)i.ReadU16();
  m_target = (AquaSimAddress)i.ReadU16();
  m_numForward = i.ReadU16();
  m_depth = ((double)i.ReadU32()) / 1000.0;

  return GetSerializedSize();
}

uint32_t
DBRHeader::GetSerializedSize(void) const
{
  // reserved bytes for header
  return (12 + 4 + 1 + 2 + 2 + 2 + 2 + 2 + 4);
}

void DBRHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU32((uint32_t)(m_position.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_position.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_position.z * 1000.0 + 0.5));
  i.WriteU32(m_packetID);
  i.WriteU8(m_mode);
  i.WriteU16(m_nhops);
  i.WriteU16(m_prevHop.GetAsInt());
  i.WriteU16(m_owner.GetAsInt());
  i.WriteU16(m_target.GetAsInt());
  i.WriteU16(m_numForward);
  i.WriteU32((uint32_t)(m_depth * 1000.0 + 0.5));
}

void DBRHeader::Print(std::ostream &os) const
{
  os << "Depth Based Routing Header is: position=(" << m_position.x << "," << m_position.y << "," << m_position.z << ") packetID=" << m_packetID << " mode=";
  switch (m_mode)
  {
  case DBRH_DATA_GREEDY:
    os << "DBRH_DATA_GREEDY";
    break;
  case DBRH_DATA_RECOVER:
    os << "DBRH_DATA_RECOVER";
    break;
  case DBRH_BEACON:
    os << "DBRH_BEACON";
    break;
  }
  os << " maxNumHops=" << m_nhops << " prevHopAddr=" << m_prevHop << " ownerAddr=" << m_owner << " depth=" << m_depth << "\n";
}

TypeId
DBRHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void DBRHeader::SetPosition(Vector position)
{
  m_position = position;
}
void DBRHeader::SetPacketID(uint32_t packetID)
{
  m_packetID = packetID;
}
void DBRHeader::SetMode(uint8_t mode)
{
  m_mode = mode;
}
void DBRHeader::SetNHops(uint16_t hops)
{
  m_nhops = hops;
}
void DBRHeader::SetPrevHop(AquaSimAddress prevHop)
{
  m_prevHop = prevHop;
}
void DBRHeader::SetTarget(AquaSimAddress target)
{
  m_target = target;
}
void DBRHeader::SetNumForward(uint16_t numforward)
{
  m_numForward = numforward;
}
void DBRHeader::SetOwner(AquaSimAddress owner)
{
  m_owner = owner;
}
void DBRHeader::SetDepth(double depth)
{
  m_depth = depth;
}

Vector
DBRHeader::GetPosition()
{
  return m_position;
}
uint32_t
DBRHeader::GetPacketID()
{
  return m_packetID;
}
uint8_t
DBRHeader::GetMode()
{
  return m_mode;
}
uint16_t
DBRHeader::GetNHops()
{
  return m_nhops;
}
AquaSimAddress
DBRHeader::GetPrevHop()
{
  return m_prevHop;
}
AquaSimAddress
DBRHeader::GetTarget()
{
  return m_target;
}
uint16_t
DBRHeader::GetNumForward()
{
  return m_numForward;
}
AquaSimAddress
DBRHeader::GetOwner()
{
  return m_owner;
}
double
DBRHeader::GetDepth()
{
  return m_depth;
}

/*
 * DDoS Routing Header
 */
NS_OBJECT_ENSURE_REGISTERED(DDOSHeader);

DDOSHeader::DDOSHeader()
{
}

DDOSHeader::~DDOSHeader()
{
}

TypeId
DDOSHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::DDOSHeader")
                          .SetParent<Header>()
                          .AddConstructor<DDOSHeader>();
  return tid;
}

uint32_t
DDOSHeader::Deserialize(Buffer::Iterator start)
{
  m_pt = start.ReadU8();
  m_index = (uint32_t)start.ReadU8();
  return GetSerializedSize();
}

uint32_t
DDOSHeader::GetSerializedSize() const
{
  return 2;
}

void DDOSHeader::Serialize(Buffer::Iterator start) const
{
  start.WriteU8(m_pt);
  start.WriteU8((uint8_t)m_index);
}

void DDOSHeader::Print(std::ostream &os) const
{
  os << "DDoS Header is: PacketType=";
  switch (m_pt)
  {
  case Interest:
    os << "INTEREST";
    break;
  case Data:
    os << "DATA";
    break;
  case NACK:
    os << "NACK";
    break;
  case Alert:
    os << "ALERT";
    break;
  }
  os << " RowIndex=" << m_index << "\n";
}

TypeId
DDOSHeader::GetInstanceTypeId() const
{
  return GetTypeId();
}

uint8_t
DDOSHeader::GetPacketType()
{
  return m_pt;
}

uint32_t
DDOSHeader::GetRowIndex()
{
  return m_index;
}

void DDOSHeader::SetPacketType(uint8_t pt)
{
  m_pt = pt;
}

void DDOSHeader::SetRowIndex(uint32_t index)
{
  m_index = index;
}

/*
 * QLRPS
 */
NS_OBJECT_ENSURE_REGISTERED(QLRPSHeader);

QLRPSHeader::QLRPSHeader() : m_messType(0)
{
}

QLRPSHeader::~QLRPSHeader()
{
}

TypeId
QLRPSHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::QLRPSHeader")
                          .SetParent<Header>()
                          .AddConstructor<QLRPSHeader>();
  return tid;
}

uint32_t
QLRPSHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_messType = i.ReadU8();
  m_pkNum = i.ReadU32();
  m_targetAddr = (AquaSimAddress)i.ReadU16();
  m_senderAddr = (AquaSimAddress)i.ReadU16();
  m_forwardAddr = (AquaSimAddress)i.ReadU16();
  m_nexthopAddr = (AquaSimAddress)i.ReadU16();
  m_previoushop = (AquaSimAddress)i.ReadU16();
  // Value=i.ReadU32();
  Value = ((double)i.ReadU32()) / 1000;
  m_resiEnergy = i.ReadU32();
  avgEnergy = i.ReadU32();
  density = i.ReadU32();
  // m_originalSource.x = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.y = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.z = ( (double) i.ReadU32() ) / 1000.0;

  // This is bloated.
  m_info.o.x = ((double)i.ReadU32()) / 1000.0;
  m_info.o.y = ((double)i.ReadU32()) / 1000.0;
  m_info.o.z = ((double)i.ReadU32()) / 1000.0;
  m_info.f.x = ((double)i.ReadU32()) / 1000.0;
  m_info.f.y = ((double)i.ReadU32()) / 1000.0;
  m_info.f.z = ((double)i.ReadU32()) / 1000.0;
  m_info.t.x = ((double)i.ReadU32()) / 1000.0;
  m_info.t.y = ((double)i.ReadU32()) / 1000.0;
  m_info.t.z = ((double)i.ReadU32()) / 1000.0;
  m_info.d.x = ((double)i.ReadU32()) / 1000.0;
  m_info.d.y = ((double)i.ReadU32()) / 1000.0;
  m_info.d.z = ((double)i.ReadU32()) / 1000.0;

  return GetSerializedSize();
}

uint32_t
QLRPSHeader::GetSerializedSize(void) const
{
  // reserved bytes for header
  // return (1+4+2+2+2+1+2+12+4+4+4+48);
  // todo
  return (1 + 4 + 2 + 2 + 2 + 2 + 2 + 4 + 4 + 4 + 4 + 48);
}

void QLRPSHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_messType);
  i.WriteU32(m_pkNum);
  i.WriteU16(m_targetAddr.GetAsInt());
  i.WriteU16(m_senderAddr.GetAsInt());
  i.WriteU16(m_forwardAddr.GetAsInt());
  i.WriteU16(m_nexthopAddr.GetAsInt());
  i.WriteU16(m_previoushop.GetAsInt());
  // i.WriteU32(Value);
  i.WriteU32((uint32_t)(Value * 1000));
  i.WriteU32(m_resiEnergy);
  i.WriteU32(avgEnergy);
  i.WriteU32(density);
  // Messy...
  // i.WriteU32 ((uint32_t)(m_originalSource.x*1000.0+0.5)); //+0.5 for uint32_t typecast
  // i.WriteU32 ((uint32_t)(m_originalSource.y*1000.0+0.5));
  // i.WriteU32 ((uint32_t)(m_originalSource.z*1000.0+0.5));

  // bloated.
  i.WriteU32((uint32_t)(m_info.o.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.o.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.o.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.z * 1000.0 + 0.5));
}

void QLRPSHeader::Print(std::ostream &os) const
{
  os << "Vector Based Routing Header is: messType=";
  switch (m_messType)
  {
  case INTEREST:
    os << "INTEREST";
    break;
  case AS_DATA:
    os << "DATA";
    break;
  case DATA_READY:
    os << "DATA_READY";
    break;
  case SOURCE_DISCOVERY:
    os << "SOURCE_DISCOVERY";
    break;
  case SOURCE_TIMEOUT:
    os << "SOURCE_TIMEOUT";
    break;
  case TARGET_DISCOVERY:
    os << "TARGET_DISCOVERY";
    break;
  case TARGET_REQUEST:
    os << "TARGET_REQUEST";
    break;
  case SOURCE_DENY:
    os << "SOURCE_DENY";
    break;
  case V_SHIFT:
    os << "V_SHIFT";
    break;
  case FLOODING:
    os << "FLOODING";
    break;
  case DATA_TERMINATION:
    os << "DATA_TERMINATION";
    break;
  case BACKPRESSURE:
    os << "BACKPRESSURE";
    break;
  case BACKFLOODING:
    os << "BACKFLOODING";
    break;
  case EXPENSION:
    os << "EXPENSION";
    break;
  case V_SHIFT_DATA:
    os << "V_SHIFT_DATA";
    break;
  case EXPENSION_DATA:
    os << "EXPENSION_DATA";
    break;
  }

  os << " pkNum=" << m_pkNum << " targetAddr=" << m_targetAddr << " senderAddr=" << m_senderAddr << " forwardAddr=" << m_forwardAddr << " originalSource=" /*<< m_originalSource.x << "," <<
                                                                                   m_originalSource.y << "," << m_originalSource.z*/
      ;

  os << "   ExtraInfo= StartPoint(" << m_info.o << ") ForwardPos(" << m_info.f << ") EndPoint(" << m_info.t << ") RecvToForwarder(" << m_info.d << ")\n";
}

TypeId
QLRPSHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void QLRPSHeader::SetMessType(uint8_t messType)
{
  m_messType = messType;
}
void QLRPSHeader::SetPkNum(uint32_t pkNum)
{
  m_pkNum = pkNum;
}
void QLRPSHeader::SetValue(double mvalue)
{
  Value = mvalue;
}
void QLRPSHeader::SetTargetAddr(AquaSimAddress targetAddr)
{
  m_targetAddr = targetAddr;
}
void QLRPSHeader::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}
void QLRPSHeader::SetForwardAddr(AquaSimAddress forwardAddr)
{
  m_forwardAddr = forwardAddr;
}
void QLRPSHeader::SetNexthopAddr(AquaSimAddress nexthopAddr)
{
  m_nexthopAddr = nexthopAddr;
}
void QLRPSHeader::SetPrevioushop(AquaSimAddress previoushop)
{
  m_previoushop = previoushop;
}
void QLRPSHeader::SetResiEnergy(uint32_t resienergy)
{
  m_resiEnergy = resienergy;
}
void QLRPSHeader::SetAvgEnergy(uint32_t avgenergy)
{
  avgEnergy = avgenergy;
}
void QLRPSHeader::SetDensity(uint32_t NextHopdensity)
{
  density = NextHopdensity;
}
/*void
QLRPSHeader::SetOriginalSource(Vector originalSource)
{
  m_originalSource = originalSource;
}*/
void QLRPSHeader::SetExtraInfo(uw_extra_info info)
{
  m_info = info;
}
void QLRPSHeader::SetExtraInfo_o(Vector position_o)
{
  m_info.o = position_o;
}
void QLRPSHeader::SetExtraInfo_f(Vector position_f)
{
  m_info.f = position_f;
}
void QLRPSHeader::SetExtraInfo_t(Vector position_t)
{
  m_info.t = position_t;
}
void QLRPSHeader::SetExtraInfo_d(Vector position_d)
{
  m_info.d = position_d;
}

uint8_t
QLRPSHeader::GetMessType()
{
  return m_messType;
}
uint32_t
QLRPSHeader::GetPkNum()
{
  return m_pkNum;
}

double
QLRPSHeader::GetValue()
{
  return Value;
}
AquaSimAddress
QLRPSHeader::GetTargetAddr()
{
  return m_targetAddr;
}
AquaSimAddress
QLRPSHeader::GetSenderAddr()
{
  return m_senderAddr;
}
AquaSimAddress
QLRPSHeader::GetForwardAddr()
{
  return m_forwardAddr;
}
AquaSimAddress
QLRPSHeader::GetNexthopAddr()
{
  return m_nexthopAddr;
}
AquaSimAddress
QLRPSHeader::GetPrevioushop()
{
  return m_previoushop;
}
uint32_t QLRPSHeader::GetResiEnergy()
{
  return m_resiEnergy;
}
uint32_t QLRPSHeader::GetAvgEnergy()
{
  return avgEnergy;
}
uint32_t QLRPSHeader::GetDensity()
{
  return density;
}
/*Vector
QLRPSHeader::GetOriginalSource()
{
  return m_originalSource;
}*/
uw_extra_info
QLRPSHeader::GetExtraInfo()
{
  return m_info;
}

/*
 * QLRPS2
 */
NS_OBJECT_ENSURE_REGISTERED(QLRPS2Header);

QLRPS2Header::QLRPS2Header() : m_messType(0)
{
}

QLRPS2Header::~QLRPS2Header()
{
}

TypeId
QLRPS2Header::GetTypeId()
{
  static TypeId tid = TypeId("ns3::QLRPS2Header")
                          .SetParent<Header>()
                          .AddConstructor<QLRPS2Header>();
  return tid;
}

uint32_t
QLRPS2Header::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_messType = i.ReadU8();
  m_pkNum = i.ReadU32();
  m_targetAddr = (AquaSimAddress)i.ReadU16();
  m_senderAddr = (AquaSimAddress)i.ReadU16();
  m_forwardAddr = (AquaSimAddress)i.ReadU16();
  m_nexthopAddr = (AquaSimAddress)i.ReadU16();
  m_previoushop = (AquaSimAddress)i.ReadU16();
  // Value=i.ReadU32();
  Value = ((double)i.ReadU32()) / 1000;
  m_resiEnergy = i.ReadU32();
  avgEnergy = i.ReadU32();
  density = i.ReadU32();
  // m_originalSource.x = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.y = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.z = ( (double) i.ReadU32() ) / 1000.0;

  // This is bloated.
  m_info.o.x = ((double)i.ReadU32()) / 1000.0;
  m_info.o.y = ((double)i.ReadU32()) / 1000.0;
  m_info.o.z = ((double)i.ReadU32()) / 1000.0;
  m_info.f.x = ((double)i.ReadU32()) / 1000.0;
  m_info.f.y = ((double)i.ReadU32()) / 1000.0;
  m_info.f.z = ((double)i.ReadU32()) / 1000.0;
  m_info.t.x = ((double)i.ReadU32()) / 1000.0;
  m_info.t.y = ((double)i.ReadU32()) / 1000.0;
  m_info.t.z = ((double)i.ReadU32()) / 1000.0;
  m_info.d.x = ((double)i.ReadU32()) / 1000.0;
  m_info.d.y = ((double)i.ReadU32()) / 1000.0;
  m_info.d.z = ((double)i.ReadU32()) / 1000.0;

  return GetSerializedSize();
}

uint32_t
QLRPS2Header::GetSerializedSize(void) const
{
  // reserved bytes for header
  // return (1+4+2+2+2+1+2+12+4+4+4+48);
  // todo
  return (1 + 4 + 2 + 2 + 2 + 2 + 2 + 4 + 4 + 4 + 4 + 48);
}

void QLRPS2Header::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_messType);
  i.WriteU32(m_pkNum);
  i.WriteU16(m_targetAddr.GetAsInt());
  i.WriteU16(m_senderAddr.GetAsInt());
  i.WriteU16(m_forwardAddr.GetAsInt());
  i.WriteU16(m_nexthopAddr.GetAsInt());
  i.WriteU16(m_previoushop.GetAsInt());
  // i.WriteU32(Value);
  i.WriteU32((uint32_t)(Value * 1000));
  i.WriteU32(m_resiEnergy);
  i.WriteU32(avgEnergy);
  i.WriteU32(density);
  // Messy...
  // i.WriteU32 ((uint32_t)(m_originalSource.x*1000.0+0.5)); //+0.5 for uint32_t typecast
  // i.WriteU32 ((uint32_t)(m_originalSource.y*1000.0+0.5));
  // i.WriteU32 ((uint32_t)(m_originalSource.z*1000.0+0.5));

  // bloated.
  i.WriteU32((uint32_t)(m_info.o.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.o.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.o.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.z * 1000.0 + 0.5));
}

void QLRPS2Header::Print(std::ostream &os) const
{
  os << "Vector Based Routing Header is: messType=";
  switch (m_messType)
  {
  case INTEREST:
    os << "INTEREST";
    break;
  case AS_DATA:
    os << "DATA";
    break;
  case DATA_READY:
    os << "DATA_READY";
    break;
  case SOURCE_DISCOVERY:
    os << "SOURCE_DISCOVERY";
    break;
  case SOURCE_TIMEOUT:
    os << "SOURCE_TIMEOUT";
    break;
  case TARGET_DISCOVERY:
    os << "TARGET_DISCOVERY";
    break;
  case TARGET_REQUEST:
    os << "TARGET_REQUEST";
    break;
  case SOURCE_DENY:
    os << "SOURCE_DENY";
    break;
  case V_SHIFT:
    os << "V_SHIFT";
    break;
  case FLOODING:
    os << "FLOODING";
    break;
  case DATA_TERMINATION:
    os << "DATA_TERMINATION";
    break;
  case BACKPRESSURE:
    os << "BACKPRESSURE";
    break;
  case BACKFLOODING:
    os << "BACKFLOODING";
    break;
  case EXPENSION:
    os << "EXPENSION";
    break;
  case V_SHIFT_DATA:
    os << "V_SHIFT_DATA";
    break;
  case EXPENSION_DATA:
    os << "EXPENSION_DATA";
    break;
  }

  os << " pkNum=" << m_pkNum << " targetAddr=" << m_targetAddr << " senderAddr=" << m_senderAddr << " forwardAddr=" << m_forwardAddr << " originalSource=" /*<< m_originalSource.x << "," <<
                                                                                   m_originalSource.y << "," << m_originalSource.z*/
      ;

  os << "   ExtraInfo= StartPoint(" << m_info.o << ") ForwardPos(" << m_info.f << ") EndPoint(" << m_info.t << ") RecvToForwarder(" << m_info.d << ")\n";
}

TypeId
QLRPS2Header::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void QLRPS2Header::SetMessType(uint8_t messType)
{
  m_messType = messType;
}
void QLRPS2Header::SetPkNum(uint32_t pkNum)
{
  m_pkNum = pkNum;
}
void QLRPS2Header::SetValue(double mvalue)
{
  Value = mvalue;
}
void QLRPS2Header::SetTargetAddr(AquaSimAddress targetAddr)
{
  m_targetAddr = targetAddr;
}
void QLRPS2Header::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}
void QLRPS2Header::SetForwardAddr(AquaSimAddress forwardAddr)
{
  m_forwardAddr = forwardAddr;
}
void QLRPS2Header::SetNexthopAddr(AquaSimAddress nexthopAddr)
{
  m_nexthopAddr = nexthopAddr;
}
void QLRPS2Header::SetPrevioushop(AquaSimAddress previoushop)
{
  m_previoushop = previoushop;
}
void QLRPS2Header::SetResiEnergy(uint32_t resienergy)
{
  m_resiEnergy = resienergy;
}
void QLRPS2Header::SetAvgEnergy(uint32_t avgenergy)
{
  avgEnergy = avgenergy;
}
void QLRPS2Header::SetDensity(uint32_t NextHopdensity)
{
  density = NextHopdensity;
}
/*void
QLRPS2Header::SetOriginalSource(Vector originalSource)
{
  m_originalSource = originalSource;
}*/
void QLRPS2Header::SetExtraInfo(uw_extra_info info)
{
  m_info = info;
}
void QLRPS2Header::SetExtraInfo_o(Vector position_o)
{
  m_info.o = position_o;
}
void QLRPS2Header::SetExtraInfo_f(Vector position_f)
{
  m_info.f = position_f;
}
void QLRPS2Header::SetExtraInfo_t(Vector position_t)
{
  m_info.t = position_t;
}
void QLRPS2Header::SetExtraInfo_d(Vector position_d)
{
  m_info.d = position_d;
}

uint8_t
QLRPS2Header::GetMessType()
{
  return m_messType;
}
uint32_t
QLRPS2Header::GetPkNum()
{
  return m_pkNum;
}

double
QLRPS2Header::GetValue()
{
  return Value;
}
AquaSimAddress
QLRPS2Header::GetTargetAddr()
{
  return m_targetAddr;
}
AquaSimAddress
QLRPS2Header::GetSenderAddr()
{
  return m_senderAddr;
}
AquaSimAddress
QLRPS2Header::GetForwardAddr()
{
  return m_forwardAddr;
}
AquaSimAddress
QLRPS2Header::GetNexthopAddr()
{
  return m_nexthopAddr;
}
AquaSimAddress
QLRPS2Header::GetPrevioushop()
{
  return m_previoushop;
}
uint32_t QLRPS2Header::GetResiEnergy()
{
  return m_resiEnergy;
}
uint32_t QLRPS2Header::GetAvgEnergy()
{
  return avgEnergy;
}
uint32_t QLRPS2Header::GetDensity()
{
  return density;
}
/*Vector
QLRPS2Header::GetOriginalSource()
{
  return m_originalSource;
}*/
uw_extra_info
QLRPS2Header::GetExtraInfo()
{
  return m_info;
}

/*
 * QDTR
 */
NS_OBJECT_ENSURE_REGISTERED(QDTRHeader);

QDTRHeader::QDTRHeader() : m_messType(0)
{
}

QDTRHeader::~QDTRHeader()
{
}

TypeId
QDTRHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::QDTRHeader")
                          .SetParent<Header>()
                          .AddConstructor<QDTRHeader>();
  return tid;
}

uint32_t
QDTRHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_messType = i.ReadU8();
  m_pkNum = i.ReadU32();
  m_Type = i.ReadU8();
  m_targetAddr = (AquaSimAddress)i.ReadU16();
  m_senderAddr = (AquaSimAddress)i.ReadU16();
  m_forwardAddr = (AquaSimAddress)i.ReadU16();
  m_nexthopAddr = (AquaSimAddress)i.ReadU16();
  m_previoushop = (AquaSimAddress)i.ReadU16();
  // Value=i.ReadU32();
  // Value = ((double)i.ReadU32()) / 1000;
  T = ((double)i.ReadU32()) / 1000;
  T_forward = i.ReadU32();
  // m_originalSource.x = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.y = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.z = ( (double) i.ReadU32() ) / 1000.0;

  // This is bloated.

  return GetSerializedSize();
}

uint32_t
QDTRHeader::GetSerializedSize(void) const
{
  // reserved bytes for header
  // return (1+4+2+2+2+1+2+12+4+4+4+48);
  // todo
  // return (1 + 4 + 1 + 2 + 2 + 2 + 2 + 2 + 4 + 4 + 4);
  return (1 + 4 + 1 + 2 + 2 + 2 + 2 + 2 + 4 + 4);
}

void QDTRHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_messType);
  i.WriteU32(m_pkNum);
  i.WriteU8(m_Type);
  i.WriteU16(m_targetAddr.GetAsInt());
  i.WriteU16(m_senderAddr.GetAsInt());
  i.WriteU16(m_forwardAddr.GetAsInt());
  i.WriteU16(m_nexthopAddr.GetAsInt());
  i.WriteU16(m_previoushop.GetAsInt());
  // i.WriteU32(Value);
  // i.WriteU32((uint32_t)(Value * 1000));
  i.WriteU32((uint32_t)(T * 1000));
  i.WriteU32(T_forward);
  // Messy...
  // i.WriteU32 ((uint32_t)(m_originalSource.x*1000.0+0.5)); //+0.5 for uint32_t typecast
  // i.WriteU32 ((uint32_t)(m_originalSource.y*1000.0+0.5));
  // i.WriteU32 ((uint32_t)(m_originalSource.z*1000.0+0.5));
  // bloated.
}

void QDTRHeader::Print(std::ostream &os) const
{
  os << "Vector Based Routing Header is: messType=";
  switch (m_messType)
  {
  case INTEREST:
    os << "INTEREST";
    break;
  case AS_DATA:
    os << "DATA";
    break;
  case DATA_READY:
    os << "DATA_READY";
    break;
  case SOURCE_DISCOVERY:
    os << "SOURCE_DISCOVERY";
    break;
  case SOURCE_TIMEOUT:
    os << "SOURCE_TIMEOUT";
    break;
  case TARGET_DISCOVERY:
    os << "TARGET_DISCOVERY";
    break;
  case TARGET_REQUEST:
    os << "TARGET_REQUEST";
    break;
  case SOURCE_DENY:
    os << "SOURCE_DENY";
    break;
  case V_SHIFT:
    os << "V_SHIFT";
    break;
  case FLOODING:
    os << "FLOODING";
    break;
  case DATA_TERMINATION:
    os << "DATA_TERMINATION";
    break;
  case BACKPRESSURE:
    os << "BACKPRESSURE";
    break;
  case BACKFLOODING:
    os << "BACKFLOODING";
    break;
  case EXPENSION:
    os << "EXPENSION";
    break;
  case V_SHIFT_DATA:
    os << "V_SHIFT_DATA";
    break;
  case EXPENSION_DATA:
    os << "EXPENSION_DATA";
    break;
  }

  os << " pkNum=" << m_pkNum << " targetAddr=" << m_targetAddr << " senderAddr=" << m_senderAddr << " forwardAddr=" << m_forwardAddr << " originalSource=" /*<< m_originalSource.x << "," <<
                                                                                   m_originalSource.y << "," << m_originalSource.z*/
      ;
}

TypeId
QDTRHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void QDTRHeader::SetMessType(uint8_t messType)
{
  m_messType = messType;
}
void QDTRHeader::SetPkNum(uint32_t pkNum)
{
  m_pkNum = pkNum;
}
void QDTRHeader::SetMType(uint8_t m_type = 1)
{
  m_Type = m_type;
}
/*void QDTRHeader::SetValue(double mvalue)
{
  Value = mvalue;
}*/
void QDTRHeader::SetTargetAddr(AquaSimAddress targetAddr)
{
  m_targetAddr = targetAddr;
}
void QDTRHeader::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}
void QDTRHeader::SetForwardAddr(AquaSimAddress forwardAddr)
{
  m_forwardAddr = forwardAddr;
}
void QDTRHeader::SetNexthopAddr(AquaSimAddress nexthopAddr)
{
  m_nexthopAddr = nexthopAddr;
}
void QDTRHeader::SetPrevioushop(AquaSimAddress previoushop)
{
  m_previoushop = previoushop;
}

void QDTRHeader::SetT(double t = 0)
{
  T = t;
}

void QDTRHeader::SetT_forward(uint32_t Tt_forward)
{
  T_forward = Tt_forward;
}
/*void
QDTRHeader::SetOriginalSource(Vector originalSource)
{
  m_originalSource = originalSource;
}*/

uint8_t
QDTRHeader::GetMessType()
{
  return m_messType;
}
uint32_t
QDTRHeader::GetPkNum()
{
  return m_pkNum;
}
uint8_t QDTRHeader::GetMType()
{
  return m_Type;
}

/*double
QDTRHeader::GetValue()
{
  return Value;
}*/

double QDTRHeader::GetT()
{
  return T;
}

uint32_t QDTRHeader::GetT_forward()
{
  return T_forward;
}
AquaSimAddress
QDTRHeader::GetTargetAddr()
{
  return m_targetAddr;
}
AquaSimAddress
QDTRHeader::GetSenderAddr()
{
  return m_senderAddr;
}
AquaSimAddress
QDTRHeader::GetForwardAddr()
{
  return m_forwardAddr;
}
AquaSimAddress
QDTRHeader::GetNexthopAddr()
{
  return m_nexthopAddr;
}
AquaSimAddress
QDTRHeader::GetPrevioushop()
{
  return m_previoushop;
}
/*
 * QTVGR
 */
NS_OBJECT_ENSURE_REGISTERED(QTVGRHeader);

QTVGRHeader::QTVGRHeader() : m_messType(0)
{
}

QTVGRHeader::~QTVGRHeader()
{
}

TypeId
QTVGRHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::QTVGRHeader")
                          .SetParent<Header>()
                          .AddConstructor<QTVGRHeader>();
  return tid;
}

uint32_t
QTVGRHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_messType = i.ReadU8();
  m_pkNum = i.ReadU32();
  m_Type = i.ReadU8();
  m_targetAddr = (AquaSimAddress)i.ReadU16();
  m_senderAddr = (AquaSimAddress)i.ReadU16();
  m_forwardAddr = (AquaSimAddress)i.ReadU16();
  m_nexthopAddr = (AquaSimAddress)i.ReadU16();
  m_previoushop = (AquaSimAddress)i.ReadU16();
  // Value=i.ReadU32();
  m_resiEnergy = i.ReadU32();
  avgEnergy = i.ReadU32();
  density = i.ReadU32();
  T = ((double)i.ReadU32()) / 1000;
  T_forward = i.ReadU32();
  // m_originalSource.x = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.y = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.z = ( (double) i.ReadU32() ) / 1000.0;

  // This is bloated.

  return GetSerializedSize();
}

uint32_t
QTVGRHeader::GetSerializedSize(void) const
{
  // reserved bytes for header
  // return (1+4+2+2+2+1+2+12+4+4+4+48);
  // todo
  return (1 + 4 + 1 + 2 + 2 + 2 + 2 + 2 + 4 + 4 + 4 + 4 + 4);
}

void QTVGRHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_messType);
  i.WriteU32(m_pkNum);
  i.WriteU8(m_Type);
  i.WriteU16(m_targetAddr.GetAsInt());
  i.WriteU16(m_senderAddr.GetAsInt());
  i.WriteU16(m_forwardAddr.GetAsInt());
  i.WriteU16(m_nexthopAddr.GetAsInt());
  i.WriteU16(m_previoushop.GetAsInt());
  // i.WriteU32(Value);
  i.WriteU32((uint32_t)(T * 1000));
  i.WriteU32(T_forward);
  i.WriteU32(m_resiEnergy);
  i.WriteU32(avgEnergy);
  i.WriteU32(density);
  // Messy...
  // i.WriteU32 ((uint32_t)(m_originalSource.x*1000.0+0.5)); //+0.5 for uint32_t typecast
  // i.WriteU32 ((uint32_t)(m_originalSource.y*1000.0+0.5));
  // i.WriteU32 ((uint32_t)(m_originalSource.z*1000.0+0.5));
  // bloated.
}

void QTVGRHeader::Print(std::ostream &os) const
{
  os << "Vector Based Routing Header is: messType=";
  switch (m_messType)
  {
  case INTEREST:
    os << "INTEREST";
    break;
  case AS_DATA:
    os << "DATA";
    break;
  case DATA_READY:
    os << "DATA_READY";
    break;
  case SOURCE_DISCOVERY:
    os << "SOURCE_DISCOVERY";
    break;
  case SOURCE_TIMEOUT:
    os << "SOURCE_TIMEOUT";
    break;
  case TARGET_DISCOVERY:
    os << "TARGET_DISCOVERY";
    break;
  case TARGET_REQUEST:
    os << "TARGET_REQUEST";
    break;
  case SOURCE_DENY:
    os << "SOURCE_DENY";
    break;
  case V_SHIFT:
    os << "V_SHIFT";
    break;
  case FLOODING:
    os << "FLOODING";
    break;
  case DATA_TERMINATION:
    os << "DATA_TERMINATION";
    break;
  case BACKPRESSURE:
    os << "BACKPRESSURE";
    break;
  case BACKFLOODING:
    os << "BACKFLOODING";
    break;
  case EXPENSION:
    os << "EXPENSION";
    break;
  case V_SHIFT_DATA:
    os << "V_SHIFT_DATA";
    break;
  case EXPENSION_DATA:
    os << "EXPENSION_DATA";
    break;
  }

  os << " pkNum=" << m_pkNum << " targetAddr=" << m_targetAddr << " senderAddr=" << m_senderAddr << " forwardAddr=" << m_forwardAddr << " originalSource=" /*<< m_originalSource.x << "," <<
                                                                                   m_originalSource.y << "," << m_originalSource.z*/
      ;
}

TypeId
QTVGRHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void QTVGRHeader::SetMessType(uint8_t messType)
{
  m_messType = messType;
}
void QTVGRHeader::SetPkNum(uint32_t pkNum)
{
  m_pkNum = pkNum;
}
void QTVGRHeader::SetMType(uint8_t m_type = 1)
{
  m_Type = m_type;
}

void QTVGRHeader::SetTargetAddr(AquaSimAddress targetAddr)
{
  m_targetAddr = targetAddr;
}
void QTVGRHeader::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}
void QTVGRHeader::SetForwardAddr(AquaSimAddress forwardAddr)
{
  m_forwardAddr = forwardAddr;
}
void QTVGRHeader::SetNexthopAddr(AquaSimAddress nexthopAddr)
{
  m_nexthopAddr = nexthopAddr;
}
void QTVGRHeader::SetPrevioushop(AquaSimAddress previoushop)
{
  m_previoushop = previoushop;
}

void QTVGRHeader::SetT(double t = 0)
{
  T = t;
}

void QTVGRHeader::SetT_forward(uint32_t Tt_forward)
{
  T_forward = Tt_forward;
}
/*void
QTVGRHeader::SetOriginalSource(Vector originalSource)
{
  m_originalSource = originalSource;
}*/

uint8_t
QTVGRHeader::GetMessType()
{
  return m_messType;
}
uint32_t
QTVGRHeader::GetPkNum()
{
  return m_pkNum;
}
uint8_t QTVGRHeader::GetMType()
{
  return m_Type;
}

double QTVGRHeader::GetT()
{
  return T;
}

uint32_t QTVGRHeader::GetT_forward()
{
  return T_forward;
}
AquaSimAddress
QTVGRHeader::GetTargetAddr()
{
  return m_targetAddr;
}
AquaSimAddress
QTVGRHeader::GetSenderAddr()
{
  return m_senderAddr;
}
AquaSimAddress
QTVGRHeader::GetForwardAddr()
{
  return m_forwardAddr;
}
AquaSimAddress
QTVGRHeader::GetNexthopAddr()
{
  return m_nexthopAddr;
}
AquaSimAddress
QTVGRHeader::GetPrevioushop()
{
  return m_previoushop;
}
uint32_t QTVGRHeader::GetResiEnergy()
{
  return m_resiEnergy;
}
uint32_t QTVGRHeader::GetAvgEnergy()
{
  return avgEnergy;
}
uint32_t QTVGRHeader::GetDensity()
{
  return density;
}
void QTVGRHeader::SetResiEnergy(uint32_t resienergy)
{
  m_resiEnergy = resienergy;
}
void QTVGRHeader::SetAvgEnergy(uint32_t avgenergy)
{
  avgEnergy = avgenergy;
}
void QTVGRHeader::SetDensity(uint32_t NextHopdensity)
{
  density = NextHopdensity;
}
/*
 * QELAR
 */
NS_OBJECT_ENSURE_REGISTERED(QELARHeader);

QELARHeader::QELARHeader() : m_messType(0)
{
}

QELARHeader::~QELARHeader()
{
}

TypeId
QELARHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::QELARHeader")
                          .SetParent<Header>()
                          .AddConstructor<QELARHeader>();
  return tid;
}

uint32_t
QELARHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_messType = i.ReadU8();
  m_pkNum = i.ReadU32();
  m_targetAddr = (AquaSimAddress)i.ReadU16();
  m_senderAddr = (AquaSimAddress)i.ReadU16();
  m_forwardAddr = (AquaSimAddress)i.ReadU16();
  m_nexthopAddr = (AquaSimAddress)i.ReadU16();
  m_previoushop = (AquaSimAddress)i.ReadU16();
  // Value=i.ReadU32();
  Value = ((double)i.ReadU32()) / 1000;
  m_resiEnergy = i.ReadU32();
  avgEnergy = i.ReadU32();
  density = i.ReadU32();
  // m_originalSource.x = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.y = ( (double) i.ReadU32() ) / 1000.0;
  // m_originalSource.z = ( (double) i.ReadU32() ) / 1000.0;

  // This is bloated.
  m_info.o.x = ((double)i.ReadU32()) / 1000.0;
  m_info.o.y = ((double)i.ReadU32()) / 1000.0;
  m_info.o.z = ((double)i.ReadU32()) / 1000.0;
  m_info.f.x = ((double)i.ReadU32()) / 1000.0;
  m_info.f.y = ((double)i.ReadU32()) / 1000.0;
  m_info.f.z = ((double)i.ReadU32()) / 1000.0;
  m_info.t.x = ((double)i.ReadU32()) / 1000.0;
  m_info.t.y = ((double)i.ReadU32()) / 1000.0;
  m_info.t.z = ((double)i.ReadU32()) / 1000.0;
  m_info.d.x = ((double)i.ReadU32()) / 1000.0;
  m_info.d.y = ((double)i.ReadU32()) / 1000.0;
  m_info.d.z = ((double)i.ReadU32()) / 1000.0;

  return GetSerializedSize();
}

uint32_t
QELARHeader::GetSerializedSize(void) const
{
  // reserved bytes for header
  // return (1+4+2+2+2+1+2+12+4+4+4+48);
  // todo
  return (1 + 4 + 2 + 2 + 2 + 2 + 2 + 4 + 4 + 4 + 4 + 48);
}

void QELARHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_messType);
  i.WriteU32(m_pkNum);
  i.WriteU16(m_targetAddr.GetAsInt());
  i.WriteU16(m_senderAddr.GetAsInt());
  i.WriteU16(m_forwardAddr.GetAsInt());
  i.WriteU16(m_nexthopAddr.GetAsInt());
  i.WriteU16(m_previoushop.GetAsInt());
  // i.WriteU32(Value);
  i.WriteU32((uint32_t)(Value * 1000));
  i.WriteU32(m_resiEnergy);
  i.WriteU32(avgEnergy);
  i.WriteU32(density);
  // Messy...
  // i.WriteU32 ((uint32_t)(m_originalSource.x*1000.0+0.5)); //+0.5 for uint32_t typecast
  // i.WriteU32 ((uint32_t)(m_originalSource.y*1000.0+0.5));
  // i.WriteU32 ((uint32_t)(m_originalSource.z*1000.0+0.5));

  // bloated.
  i.WriteU32((uint32_t)(m_info.o.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.o.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.o.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.f.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.t.z * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.x * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.y * 1000.0 + 0.5));
  i.WriteU32((uint32_t)(m_info.d.z * 1000.0 + 0.5));
}

void QELARHeader::Print(std::ostream &os) const
{
  os << "Vector Based Routing Header is: messType=";
  switch (m_messType)
  {
  case INTEREST:
    os << "INTEREST";
    break;
  case AS_DATA:
    os << "DATA";
    break;
  case DATA_READY:
    os << "DATA_READY";
    break;
  case SOURCE_DISCOVERY:
    os << "SOURCE_DISCOVERY";
    break;
  case SOURCE_TIMEOUT:
    os << "SOURCE_TIMEOUT";
    break;
  case TARGET_DISCOVERY:
    os << "TARGET_DISCOVERY";
    break;
  case TARGET_REQUEST:
    os << "TARGET_REQUEST";
    break;
  case SOURCE_DENY:
    os << "SOURCE_DENY";
    break;
  case V_SHIFT:
    os << "V_SHIFT";
    break;
  case FLOODING:
    os << "FLOODING";
    break;
  case DATA_TERMINATION:
    os << "DATA_TERMINATION";
    break;
  case BACKPRESSURE:
    os << "BACKPRESSURE";
    break;
  case BACKFLOODING:
    os << "BACKFLOODING";
    break;
  case EXPENSION:
    os << "EXPENSION";
    break;
  case V_SHIFT_DATA:
    os << "V_SHIFT_DATA";
    break;
  case EXPENSION_DATA:
    os << "EXPENSION_DATA";
    break;
  }

  os << " pkNum=" << m_pkNum << " targetAddr=" << m_targetAddr << " senderAddr=" << m_senderAddr << " forwardAddr=" << m_forwardAddr << " originalSource=" /*<< m_originalSource.x << "," <<
                                                                                   m_originalSource.y << "," << m_originalSource.z*/
      ;

  os << "   ExtraInfo= StartPoint(" << m_info.o << ") ForwardPos(" << m_info.f << ") EndPoint(" << m_info.t << ") RecvToForwarder(" << m_info.d << ")\n";
}

TypeId
QELARHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void QELARHeader::SetMessType(uint8_t messType)
{
  m_messType = messType;
}
void QELARHeader::SetPkNum(uint32_t pkNum)
{
  m_pkNum = pkNum;
}
void QELARHeader::SetValue(double mvalue)
{
  Value = mvalue;
}
void QELARHeader::SetTargetAddr(AquaSimAddress targetAddr)
{
  m_targetAddr = targetAddr;
}
void QELARHeader::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}
void QELARHeader::SetForwardAddr(AquaSimAddress forwardAddr)
{
  m_forwardAddr = forwardAddr;
}
void QELARHeader::SetNexthopAddr(AquaSimAddress nexthopAddr)
{
  m_nexthopAddr = nexthopAddr;
}
void QELARHeader::SetPrevioushop(AquaSimAddress previoushop)
{
  m_previoushop = previoushop;
}
void QELARHeader::SetResiEnergy(uint32_t resienergy)
{
  m_resiEnergy = resienergy;
}
void QELARHeader::SetAvgEnergy(uint32_t avgenergy)
{
  avgEnergy = avgenergy;
}
void QELARHeader::SetDensity(uint32_t NextHopdensity)
{
  density = NextHopdensity;
}
/*void
QELARHeader::SetOriginalSource(Vector originalSource)
{
  m_originalSource = originalSource;
}*/
void QELARHeader::SetExtraInfo(uw_extra_info info)
{
  m_info = info;
}
void QELARHeader::SetExtraInfo_o(Vector position_o)
{
  m_info.o = position_o;
}
void QELARHeader::SetExtraInfo_f(Vector position_f)
{
  m_info.f = position_f;
}
void QELARHeader::SetExtraInfo_t(Vector position_t)
{
  m_info.t = position_t;
}
void QELARHeader::SetExtraInfo_d(Vector position_d)
{
  m_info.d = position_d;
}

uint8_t
QELARHeader::GetMessType()
{
  return m_messType;
}
uint32_t
QELARHeader::GetPkNum()
{
  return m_pkNum;
}

double
QELARHeader::GetValue()
{
  return Value;
}
AquaSimAddress
QELARHeader::GetTargetAddr()
{
  return m_targetAddr;
}
AquaSimAddress
QELARHeader::GetSenderAddr()
{
  return m_senderAddr;
}
AquaSimAddress
QELARHeader::GetForwardAddr()
{
  return m_forwardAddr;
}
AquaSimAddress
QELARHeader::GetNexthopAddr()
{
  return m_nexthopAddr;
}
AquaSimAddress
QELARHeader::GetPrevioushop()
{
  return m_previoushop;
}
uint32_t QELARHeader::GetResiEnergy()
{
  return m_resiEnergy;
}
uint32_t QELARHeader::GetAvgEnergy()
{
  return avgEnergy;
}
uint32_t QELARHeader::GetDensity()
{
  return density;
}
/*Vector
QELARHeader::GetOriginalSource()
{
  return m_originalSource;
}*/
uw_extra_info
QELARHeader::GetExtraInfo()
{
  return m_info;
}
/*
 * CARMA
 */
NS_OBJECT_ENSURE_REGISTERED(CARMAHeader);
CARMAHeader::CARMAHeader() : m_messType(0)
{
}
CARMAHeader::~CARMAHeader()
{
}
TypeId
CARMAHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::CARMAHeader")
                          .SetParent<Header>()
                          .AddConstructor<CARMAHeader>();
  return tid;
}
uint32_t
CARMAHeader::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_messType = i.ReadU8();
  m = i.ReadU8();
  for (int j = 0; j < m; j++)
  {
    Relay.push_back(i.ReadU8());
  }
  n = i.ReadU8();
  for (int j = 0; j < n; j++)
  {
    std::pair<uint16_t, double> newPair;
    newPair.first = i.ReadU16();
    newPair.second = ((double)i.ReadU32()) / 1000;
    P.insert(newPair);
  }
  m_pkNum = i.ReadU16();
  m_forNum = i.ReadU16();
  m_forwardAddr = (AquaSimAddress)i.ReadU16();
  m_previousAddr = (AquaSimAddress)i.ReadU16();
  m_senderAddr = (AquaSimAddress)i.ReadU16();
  Value = ((double)i.ReadU32()) / 1000;

  return GetSerializedSize();
}
uint32_t
CARMAHeader::GetSerializedSize(void) const
{
  // reserved bytes for header
  // return (1+4+2+2+2+1+2+12+4+4+4+48);
  // todo
  return (1 + 1 + m + 1 + 2 * n + 4 * n + 2 + 2 + 2 + 2 + 2 + 4);
}
void CARMAHeader::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(m_messType);
  i.WriteU8(m);
  for (auto j : Relay)
  {
    i.WriteU8(j);
  }
  i.WriteU8(n);
  for (std::map<uint16_t, double>::const_iterator it = P.begin(); it != P.end(); it++)
  {
    i.WriteU16(it->first);
    i.WriteU32((uint32_t)(it->second * 1000));
  }
  i.WriteU16(m_pkNum);
  i.WriteU16(m_forNum);
  i.WriteU16(m_forwardAddr.GetAsInt());
  i.WriteU16(m_previousAddr.GetAsInt());
  i.WriteU16(m_senderAddr.GetAsInt());
  i.WriteU32((uint32_t)(Value * 1000));
}
void CARMAHeader::Print(std::ostream &os) const
{
  os << "Vector Based Routing Header is: messType=";
  switch (m_messType)
  {
  case INTEREST:
    os << "INTEREST";
    break;
  case AS_DATA:
    os << "DATA";
    break;
  case DATA_READY:
    os << "DATA_READY";
    break;
  case SOURCE_DISCOVERY:
    os << "SOURCE_DISCOVERY";
    break;
  case SOURCE_TIMEOUT:
    os << "SOURCE_TIMEOUT";
    break;
  case TARGET_DISCOVERY:
    os << "TARGET_DISCOVERY";
    break;
  case TARGET_REQUEST:
    os << "TARGET_REQUEST";
    break;
  case SOURCE_DENY:
    os << "SOURCE_DENY";
    break;
  case V_SHIFT:
    os << "V_SHIFT";
    break;
  case FLOODING:
    os << "FLOODING";
    break;
  case DATA_TERMINATION:
    os << "DATA_TERMINATION";
    break;
  case BACKPRESSURE:
    os << "BACKPRESSURE";
    break;
  case BACKFLOODING:
    os << "BACKFLOODING";
    break;
  case EXPENSION:
    os << "EXPENSION";
    break;
  case V_SHIFT_DATA:
    os << "V_SHIFT_DATA";
    break;
  case EXPENSION_DATA:
    os << "EXPENSION_DATA";
    break;
  }

  os << " m=" << (int)m << " n=" << (int)n << " pkNum=" << m_pkNum << " forNum" << m_forNum << " forwardAddr=" << m_forwardAddr << " previousAddr=" << m_previousAddr << " senderAddr" << m_senderAddr << " Value=" << Value << std::endl;
  for (auto i : Relay)
  {
    os << "Node:" << (int)i << std::endl;
  }
  for (std::map<uint16_t, double>::const_iterator it = P.begin(); it != P.end(); it++)
  {
    os << "Node:" << it->first << " P:" << it->second << std::endl;
  }
}
TypeId
CARMAHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}
void CARMAHeader::SetMessType(uint8_t messType)
{
  m_messType = messType;
}
void CARMAHeader::SetM(uint8_t Mm)
{
  m = Mm;
}
void CARMAHeader::SetRelay(std::vector<uint8_t> relay)
{
  for (auto i : relay)
  {
    Relay.push_back(i);
  }
}
void CARMAHeader::SetN(uint8_t Nn)
{
  n = Nn;
}
void CARMAHeader::SetP(uint16_t num, double Pp)
{
  std::pair<uint16_t, double> newPair;
  newPair.first = num;
  newPair.second = Pp;
  P.insert(newPair);
}
void CARMAHeader::SetPkNum(uint16_t pkNum)
{
  m_pkNum = pkNum;
}
void CARMAHeader::SetForNum(uint16_t forNum)
{
  m_forNum = forNum;
}
void CARMAHeader::SetValue(double mvalue)
{
  Value = mvalue;
}
void CARMAHeader::SetForwardAddr(AquaSimAddress forwardAddr)
{
  m_forwardAddr = forwardAddr;
}
void CARMAHeader::SetPrevioushop(AquaSimAddress previousAddr)
{
  m_previousAddr = previousAddr;
}
void CARMAHeader::SetSenderAddr(AquaSimAddress senderAddr)
{
  m_senderAddr = senderAddr;
}

uint8_t
CARMAHeader::GetMessType()
{
  return m_messType;
}
uint8_t CARMAHeader::GetM()
{
  return m;
}
std::vector<uint8_t> CARMAHeader::GetRelay()
{
  return Relay;
}
uint8_t CARMAHeader::GetN()
{
  return n;
}
double CARMAHeader::GetP(uint16_t num)
{
  std::map<uint16_t, double>::iterator it;
  for (it = P.begin(); it != P.end(); it++)
  {
    if (it->first == num)
    {
      return it->second;
    }
  }
  return 0;
}
uint16_t
CARMAHeader::GetPkNum()
{
  return m_pkNum;
}
uint16_t
CARMAHeader::GetForNum()
{
  return m_forNum;
}
double
CARMAHeader::GetValue()
{
  return Value;
}
AquaSimAddress
CARMAHeader::GetForwardAddr()
{
  return m_forwardAddr;
}
AquaSimAddress
CARMAHeader::GetPrevioushop()
{
  return m_previousAddr;
}
AquaSimAddress
CARMAHeader::GetSenderAddr()
{
  return m_senderAddr;
}
void CARMAHeader::clear()
{
  std::vector<uint8_t>().swap(Relay);
  P.clear();
}