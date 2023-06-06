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

#include "aqua-sim-routing-vbf.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-propagation.h"
#include "aqua-sim-trailer.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include "Logger.h"
#include <iostream>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimVBF");
//NS_OBJECT_ENSURE_REGISTERED(AquaSimPktHashTable);

AquaSimPktHashTable::AquaSimPktHashTable() {
  NS_LOG_FUNCTION(this);
  m_windowSize=WINDOW_SIZE;
  //  lower_counter=0;
  // 50 items in the hash table, however, because it begins by 0, so, minus 1
  //Tcl_InitHashTable(&m_htable, 3);
}

AquaSimPktHashTable::~AquaSimPktHashTable()
{
  NS_LOG_FUNCTION(this);
  for (std::map<hash_entry,vbf_neighborhood*>::iterator it=m_htable.begin(); it!=m_htable.end(); ++it) {

    delete it->second;
  }
  m_htable.clear();
}

void
AquaSimPktHashTable::Reset()
{
  m_htable.clear();
  /*
	vbf_neighborhood *hashPtr;
	Tcl_HashEntry *entryPtr;
	Tcl_HashSearch searchPtr;

	entryPtr = Tcl_FirstHashEntry(&m_htable, &searchPtr);
	while (entryPtr != NULL) {
		hashPtr = (vbf_neighborhood *)Tcl_GetHashValue(entryPtr);
		delete hashPtr;
		Tcl_DeleteHashEntry(entryPtr);
		entryPtr = Tcl_NextHashEntry(&searchPtr);
	}
  */
}

vbf_neighborhood*
AquaSimPktHashTable::GetHash(AquaSimAddress senderAddr, unsigned int pk_num)
{
  hash_entry entry = std::make_pair (senderAddr,pk_num);
  std::map<hash_entry,vbf_neighborhood*>::iterator it;

  it = m_htable.find(entry);

  if (it == m_htable.end())
    return NULL;

  return it->second;
  /*
  unsigned int key[3];

	key[0] = senderAddr;
	key[1] = 0; //sender_id.port_;
	key[2] = pk_num;

	Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&m_htable, (char *)key);

	if (entryPtr == NULL )
		return NULL;

	return (vbf_neighborhood *)Tcl_GetHashValue(entryPtr);
  */
}

void
AquaSimPktHashTable::PutInHash(AquaSimAddress sAddr, unsigned int pkNum)
{
	//Tcl_HashEntry *entryPtr;
	// Pkt_Hash_Entry    *hashPtr;
	vbf_neighborhood* hashPtr;
  //unsigned int key[3];
	//bool newPtr = true;

	//key[1]=0; //(vbh->sender_id).port_;
  hash_entry entry = std::make_pair (sAddr,pkNum);
  std::map<hash_entry,vbf_neighborhood*>::iterator it;

	int k=pkNum-m_windowSize;
	if(k>0)    //TODO verify this in future work
	{
		for (int i=0; i<k; i++)
		{
      entry.second = i;
      it = m_htable.find(entry);
      if(it != m_htable.end())
      {
        hashPtr = it->second;
        delete hashPtr;
        m_htable.erase(it);
      }
		}
	}

  entry.second = pkNum;
  hashPtr = GetHash(sAddr,pkNum);
  //entryPtr = Tcl_CreateHashEntry(&m_htable, (char *)key, &newPtr);
	if (hashPtr != NULL) {
		int m=hashPtr->number;
		if (m<MAX_NEIGHBOR) {
			hashPtr->number++;
			hashPtr->neighbor[m].x=0;
			hashPtr->neighbor[m].y=0;
			hashPtr->neighbor[m].z=0;
		}
		return;
	}
	hashPtr=new vbf_neighborhood[1];
	hashPtr[0].number=1;
	hashPtr[0].neighbor[0].x=0;
	hashPtr[0].neighbor[0].y=0;
	hashPtr[0].neighbor[0].z=0;
  std::pair<hash_entry,vbf_neighborhood*> newPair;
  newPair.first=entry; newPair.second=hashPtr;
  if (m_htable.insert(newPair).second == false)
  {
    delete newPair.second;
  }
	//Tcl_SetHashValue(entryPtr, hashPtr);
}

void
AquaSimPktHashTable::PutInHash(AquaSimAddress sAddr, unsigned int pkNum, Vector p)
{
  NS_LOG_DEBUG("PutinHash begin:" << sAddr << "," << pkNum << ",(" << p.x << "," << p.y << "," << p.z << ")");
	//Tcl_HashEntry *entryPtr;
	// Pkt_Hash_Entry    *hashPtr;
	vbf_neighborhood* hashPtr;
	//unsigned int key[3];
	//bool newPtr = true;

	//key[1]=0; //(vbh->sender_id).port_;
  hash_entry entry = std::make_pair (sAddr,pkNum);
  std::map<hash_entry,vbf_neighborhood*>::iterator it;
	int k=pkNum-m_windowSize;
	if(k>0)
	{
		for (int i=0; i<k; i++)
		{
      entry.second = i;
      it = m_htable.find(entry);
      if(it != m_htable.end())
      {
        hashPtr = it->second;
        delete hashPtr;
        m_htable.erase(it);
      }
		}
	}

  entry.second = pkNum;
  hashPtr = GetHash(sAddr,pkNum);
  //entryPtr = Tcl_CreateHashEntry(&m_htable, (char *)key, &newPtr);
	if (hashPtr != NULL)
	{
		int m=hashPtr->number;
		// printf("hash_table: this is not old item, there are %d item inside\n",m);
		if (m<MAX_NEIGHBOR) {
			hashPtr->number++;
			hashPtr->neighbor[m].x=p.x;
			hashPtr->neighbor[m].y=p.y;
			hashPtr->neighbor[m].z=p.z;
		}
		return;
	}
	hashPtr=new vbf_neighborhood[1];
	hashPtr[0].number=1;
	hashPtr[0].neighbor[0].x=p.x;
	hashPtr[0].neighbor[0].y=p.y;
	hashPtr[0].neighbor[0].z=p.z;

  std::pair<hash_entry,vbf_neighborhood*> newPair;
  newPair.first=entry; newPair.second=hashPtr;
  if (m_htable.insert(newPair).second == false)
  {
    delete newPair.second;
  }
	//Tcl_SetHashValue(entryPtr, hashPtr);
}

AquaSimDataHashTable::AquaSimDataHashTable() {
  NS_LOG_FUNCTION(this);
  Reset();
  //Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
}

AquaSimDataHashTable::~AquaSimDataHashTable()
{
  NS_LOG_FUNCTION(this);
  Reset();
}

void
AquaSimDataHashTable::Reset()
{
  for (std::map<int*,int*>::iterator it=m_htable.begin(); it!=m_htable.end(); ++it) {
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

int*
AquaSimDataHashTable::GetHash(int* attr)
{
  std::map<int*,int*>::iterator it;
  it = m_htable.find(attr);
  if (it == m_htable.end())
    return NULL;

  return it->second;
}

void
AquaSimDataHashTable::PutInHash(int* attr)
{
	//bool newPtr = true;

	//Tcl_HashEntry* entryPtr=Tcl_CreateHashEntry(&htable, (char *)attr, &newPtr);
	if (m_htable.count(attr)>0)
		return;

	int *hashPtr=new int[1];
	hashPtr[0]=1;
  m_htable.insert(std::pair<int*,int*>(attr,hashPtr));
	//Tcl_SetHashValue(entryPtr, hashPtr);
}

NS_OBJECT_ENSURE_REGISTERED(AquaSimVBF);

AquaSimVBF::AquaSimVBF()
{
	// Initialize variables.
	//  printf("VB initialized\n");
	m_pkCount = 0;
	//TODO
	m_width=1000;
	//m_width=0;
	m_counter=0;
	m_priority=1.5;
	//m_priority=2;
	//m_useOverhear = 0;
	m_enableRouting = 1;
  m_targetPos = Vector();
  m_rand = CreateObject<UniformRandomVariable> ();
}

TypeId
AquaSimVBF::GetTypeId(void)
{
  static TypeId tid = TypeId ("ns3::AquaSimVBF")
    .SetParent<AquaSimRouting> ()
    .AddConstructor<AquaSimVBF> ()
    .AddAttribute ("HopByHop", "Hop by hop VBF setting. Default 0 is false.",
      IntegerValue(0),
      MakeIntegerAccessor(&AquaSimVBF::m_hopByHop),
      MakeIntegerChecker<int>())
    .AddAttribute ("EnableRouting", "Enable routing VBF setting. Default 1 is true.",
      IntegerValue(1),
      MakeIntegerAccessor(&AquaSimVBF::m_enableRouting),
      MakeIntegerChecker<int>())
    .AddAttribute ("Width", "Width of VBF. Default is 100.",
      DoubleValue(3000),
	  //DoubleValue(100),
      MakeDoubleAccessor(&AquaSimVBF::m_width),
      MakeDoubleChecker<double>())
    .AddAttribute ("TargetPos", "Position of target sink (x,y,z).",
      Vector3DValue(),
      MakeVector3DAccessor(&AquaSimVBF::m_targetPos),
      MakeVector3DChecker())
  ;
  return tid;
  //bind("m_useOverhear_", &m_useOverhear);
}

int64_t
AquaSimVBF::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}
//传输层调用Txprocess
bool AquaSimVBF::TxProcess(Ptr< Packet > packet, const Address &dest, uint16_t protocolNumber)
{
  VBHeader vbh;
  
  vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
	packet->AddHeader(vbh);

    if(packet->GetUid()>100){
            std::ofstream outfile("Nodedelay.txt", std::ios::app);
			double curtime=Simulator::Now().GetSeconds();
			outfile<<"pktID:"<<packet->GetUid()<<" sendTime "<<curtime<<"\n";
			outfile.close();
	}
	return Recv(packet, dest, protocolNumber);
}
//MAC层的包调用Recv
bool
AquaSimVBF::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION(this);
  std::cout<<"================Recv=================\n";
  std::cout<<"Current node:"<<AquaSimAddress::ConvertFrom(m_device->GetAddress())<<"\n";
  VBHeader vbh;
  AquaSimTrailer tra;

  packet->RemoveHeader(vbh);
  packet->RemoveTrailer(tra);
  //std::cout<<"Recv packet:"<<packet<<"\n";
  //std::cout<<"ash.GetNumForwards:"<<ash.GetNumForwards()<<"\n";
  if (vbh.GetMessType()!=AS_DATA) {  //no headers //TODO create specalized Application instead of using this hack.// how many times this pkt was forwarded
	   tra.SetNextHop(AquaSimAddress::GetBroadcast());
	   tra.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	   tra.SetDAddr(AquaSimAddress::ConvertFrom(dest));

    vbh.SetMessType(AS_DATA);
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
    vbh.SetPkNum(packet->GetUid());

    //set position error    //todo

    Ptr<Object> sObject = GetNetDevice()->GetNode();
    Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel> ();

    //todo
    //GetNetDevice()->SetRouting(this);
    vbh.SetOriginalSource(sModel->GetPosition());
    vbh.SetExtraInfo_f(sModel->GetPosition());

    vbh.SetExtraInfo_t(m_targetPos);
    vbh.SetExtraInfo_o(sModel->GetPosition());
    std::cout<<"source position:x:"<<sModel->GetPosition().x << " y:"<<sModel->GetPosition().y<<" z:"<<sModel->GetPosition().z<< std::endl;
    std::cout<<"Recv-addheader\n";

    packet->AddHeader(vbh);
  } else {
	packet->AddHeader(vbh);
    packet->PeekHeader(vbh);
    //ignoring forward iterator, but this can be simply changed if necessary
  }

  packet->AddTrailer(tra);
	//unsigned char msg_type =vbh.GetMessType();  //unused
	//unsigned int dtype = vbh.GetDataType();  //unused
	//double t1=vbh.GetTs();  //unused

	if( !m_enableRouting ) {
		if( vbh.GetMessType() != AS_DATA ) {
			packet=0;
			std::cout<< "vbf recv vbh.GetMessType() != AS_DATA vbh.GetMessType():"<<vbh.GetMessType()<<std::endl;
			return false;
		}
		std::cout<<"vbh.GetTargetAddr():"<<vbh.GetTargetAddr()<<" GetNetDevice()->GetAddress()"<<GetNetDevice()->GetAddress()<<std::endl;

		if( vbh.GetSenderAddr() == GetNetDevice()->GetAddress() ) {

			MACprepare(packet);
			MACsend(packet, (m_rand->GetValue()*JITTER));
			std::cout<<"Recv macsend\n";
		}
		else if( vbh.GetTargetAddr() == GetNetDevice()->GetAddress() )  {
			DataForSink(packet);
			std::cout<<"Recv data for sink\n";
		}
		return true;
	}

	vbf_neighborhood *hashPtr= PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());
	// Received this packet before ?

	if (hashPtr != NULL) {
		PktTable.PutInHash(vbh.GetSenderAddr(), vbh.GetPkNum(),vbh.GetExtraInfo().f);
		packet=0;
		Ptr<Object> sObject = GetNetDevice()->GetNode();
		    Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel> ();
		std::cout<<"hashPtr != NULL position.x:"<<sModel->GetPosition().x<<"position.y:"<<sModel->GetPosition().y<<"position.z:"<<sModel->GetPosition().z<<std::endl;
		std::cout<< "vbf hashPtr != null"<<std::endl;
    return false;
		// printf("vectrobasedforward: this is duplicate packet\n");
	}
	else {
		// Never receive it before ? Put in hash table.
		//printf("vectrobasedforward: this is new packet\n");
		std::cout<<"Recv nevev receive it before\n";
		PktTable.PutInHash(vbh.GetSenderAddr(), vbh.GetPkNum(),vbh.GetExtraInfo().f);

    Ptr<Object> sObject = GetNetDevice()->GetNode();
    Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel> ();
     //Todo
    std::cout<<"position.x:"<<sModel->GetPosition().x<<"position.y:"<<sModel->GetPosition().y<<"position.z:"<<sModel->GetPosition().z<<std::endl;
    		//todo end
    Vector forwarder = vbh.GetExtraInfo().f;

    packet->RemoveHeader(vbh);
    Vector d = Vector(sModel->GetPosition().x - forwarder.x,
                      sModel->GetPosition().y - forwarder.y,
                      sModel->GetPosition().z - forwarder.z);
    vbh.SetExtraInfo_d(d);
    packet->AddHeader(vbh);

		ConsiderNew(packet);
	}

  return true;
}

void
AquaSimVBF::ConsiderNew(Ptr<Packet> pkt)
{
	//std::cout << "===========vbf: considerNew===========\n";
  NS_LOG_FUNCTION(this);

  VBHeader vbh;
  AquaSimTrailer tra;

  pkt->PeekHeader(vbh);
  //Todo
  double tx=vbh.GetExtraInfo().t.x;
  	double ty=vbh.GetExtraInfo().t.y;
  	double tz=vbh.GetExtraInfo().t.z;
  	std::cout<<"consider new begin : Advance target position is:"<<tx<<","<<ty<<","<<tz<<'\n';
    Vector pos = GetNetDevice()->GetPosition();
    std::cout<<"consider new begin : Advance current position is:"<<pos.x<<","<<pos.y<<","<<pos.z<<'\n';
  //todo end


	unsigned char msg_type =vbh.GetMessType();
	//unsigned int dtype = vbh.GetDataType();  //unused
	double l;//,h;  //unused
    //JITTER=0.1;use to generate random
	//Pkt_Hash_Entry *hashPtr;
	vbf_neighborhood *hashPtr;
	//  Agent_List *agentPtr;
	// PrvCurPtr  RetVal;
	AquaSimAddress from_nodeAddr;//, forward_nodeAddr;zhuan fa qi

	Ptr<Packet> gen_pkt;
	VBHeader gen_vbh;

	//  printf("Vectorbasedforward:oops!\n");
	switch (msg_type) {
	case INTEREST:
		std::cout << "===========vbf: INTEREST===========\n";
		// printf("Vectorbasedforward:it is interest packet!\n");
		hashPtr = PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());
    NS_LOG_INFO("ConsiderNew: INTEREST, hashptr #=" << hashPtr->number);
		// Check if it comes from sink agent of this node
		// If so we have to keep it in sink list

		from_nodeAddr = vbh.GetSenderAddr();
		//forward_nodeAddr = vbh.GetForwardAddr();
		//  printf("Vectorbasedforward:it the from_nodeaddr is %d %d  and theb this node id is %d ,%d!\n", from_nodeAddr,from_nodeID.port_,THIS_NODE.addr_,THIS_NODE.port_ );

		if (GetNetDevice()->GetAddress() == from_nodeAddr) {

			MACprepare(pkt);
			MACsend(pkt,m_rand->GetValue()*JITTER);
			//  printf("vectorbasedforward: after MACprepare(pkt)\n");
		}
		else
		{
			//CalculatePosition(pkt); not necessary with mobilitymodel
			//printf("vectorbasedforward: This packet is from different node\n");
			if (IsTarget(pkt))
			{
				// If this node is target?
				l=Advance(pkt);//l=0?

				//    if (!SendUp(p))
      	//	     NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
				//  printf("vectorbasedforward:%d send out the source-discovery \n",here_.addr_);
      //  pkt->RemoveHeader(ash);
        pkt->RemoveHeader(vbh);
        vbh.SetMessType(SOURCE_DISCOVERY);
        pkt->AddHeader(vbh);
       // pkt->AddHeader(ash);
				SetDelayTimer(pkt,l*JITTER);
				// !!! need to re-think
			}
			else{
				// CalculatePosition(pkt);
				// No the target forwared
				l=Advance(pkt);
				//h=Projection(pkt);  //never used...
				if (IsCloseEnough(pkt)) {
					// printf("vectorbasedforward:%d I am close enough for the interest\n",here_.addr_);
					MACprepare(pkt);
					MACsend(pkt,m_rand->GetValue()*JITTER);//!!!! need to re-think
				}
				else {
					//  printf("vectorbasedforward:%d I am not close enough for the interest  \n",here_.addr_);
					pkt=0;
				}
			}
		}
		// pkt=0;
		return;




	case TARGET_DISCOVERY:
		std::cout << "===========vbf: Target_DISCOVERY==========\n";
		// from other nodes hitted by the packet, it is supposed
		// to be the one hop away from the sink

		// printf("Vectorbasedforward(%d,%d):it is target-discovery  packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh.GetPkNum(),vbh.GetTargetAddr(),vbh.GetExtraInfo().t.x, vbh.GetExtraInfo().t.y,vbh.GetExtraInfo().t.z,vbh.GetRange());
		if (GetNetDevice()->GetAddress()==vbh.GetTargetAddr()) {
			//printf("Vectorbasedforward(%d,??%d):it is target-discovery  packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh.GetPkNum(),vbh.GetTargetAddr(),vbh.GetExtraInfo().t.x, vbh.GetExtraInfo().t.y,vbh.GetExtraInfo().t.z,vbh.GetRange());
			// AquaSimAddress *hashPtr= PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());
			// Received this packet before ?
			// if (hashPtr == NULL) {

			//CalculatePosition(pkt); not necessary with mobilitymodel
			DataForSink(pkt);
      NS_LOG_INFO("AquaSimVBF::ConsiderNew: target is " << GetNetDevice()->GetAddress());
			// } //New data Process this data
			//
		} else  {pkt=0;}
		return;

	case SOURCE_DISCOVERY:
		std::cout << "===========vbf: source_discovery===========\n";
		pkt=0;
		// other nodes already claim to be the source of this interest
		//   SourceTable.PutInHash(vbh);
		return;


	case DATA_READY:
		std::cout << "===========vbf:data_ready==========\n";
		//  printf("Vectorbasedforward(%d,%d):it is data ready packet(%d)! it target id is %d \n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_);
		from_nodeAddr = vbh.GetSenderAddr();
		if (GetNetDevice()->GetAddress() == from_nodeAddr) {
			// come from the same node, broadcast it
			MACprepare(pkt);
			MACsend(pkt,m_rand->GetValue()*JITTER);
			return;
		}
		//CalculatePosition(pkt); not necessary with mobilitymodel
		if (GetNetDevice()->GetAddress()==vbh.GetTargetAddr())
		{
		  NS_LOG_INFO("AquaSimVBF::ConsiderNew: target is " << GetNetDevice()->GetAddress());
			DataForSink(pkt); // process it
		}
		else{
			// printf("Vectorbasedforward: %d is the not  target\n", here_.addr_);
			MACprepare(pkt);
			MACsend(pkt, m_rand->GetValue()*JITTER);
		}
		return;

	/*
	   case DATA_READY :

	   // put source_agent in source list of routing table
	   agentPtr = new Agent_List;
	   AGT_ADDR(agentPtr) = vbh.GetSenderAddr();
	   agentPtr->next = routing_table[dtype].source;
	   routing_table[dtype].source = agentPtr;

	   // !! this part will be modified later
	   God::instance()->AddSource(dtype, (vbh.GetSenderAddr()));

	   gen_pkt = PrepareMessage(dtype, vbh.GetSenderAddr(), DATA_REQUEST);
	   gen_vbh = HDR_UWVB(gen_pkt);
	   //      gen_vbh->report_rate = ORIGINAL;
     if (!SendUp(p))
        NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
	   pkt=0;
	   return;
	 */

	case AS_DATA:
		std::cout << "===========vbf: as data===========\n";
    NS_LOG_INFO("AquaSimVBF::ConsiderNew: data packet");

   // std::cout<<"considerNew as_data current node:"<<GetNetDevice()->GetNode()->GetObject<Node>()->GetId()<<"\n";
    //std::cout<< "Node "<<AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()<<" MAC Layer Send down"<<std::endl;
    //std::cout<<"considerNew as_data current node position:"<<GetNetDevice()->GetAddress()<<"\n";
		// printf("Vectorbasedforward(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);
		//  printf("Vectorbasedforward(%d):it is data packet(%d)\n",here_.addr_,vbh->pk_num);
		from_nodeAddr = vbh.GetSenderAddr();
		printEnergy(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        //TODO
		if (GetNetDevice()->GetAddress() == from_nodeAddr) {
			// come from the same node, broadcast it
			std::cout<<"considerNew_MACsssend\n";
			MACprepare(pkt);
			MACsend(pkt,0);
			return;
		}
		//CalculatePosition(pkt); not necessary with mobilitymodel
		//  printf("vectorbasedforward: after MACprepare(pkt)\n");
		l=Advance(pkt);
		//h=Projection(pkt);  //never used...
		std::cout<<"considerNew as_data forwardnodeaddr:"<<vbh.GetSenderAddr()<<"\n";
        std::cout<<"considerNew_as_data l:"<<l<<"\n";

		if (GetNetDevice()->GetAddress()==vbh.GetTargetAddr())
		{
			// printf("Vectorbasedforward: %d is the target\n", here_.addr_);
			if(pkt->GetUid()>100){
                std::ofstream delayT("Nodedelay.txt", std::ios::app);
				double curtime=Simulator::Now().GetSeconds();
			    delayT<<"pktID:"<<pkt->GetUid()<<" RecvTime "<<curtime<<"\n";
			    delayT.close();
	        }
			std::ofstream outfile("packet.txt", std::ios::app);
			outfile<<pkt->GetUid()<<"\n";
			outfile.close();
			DataForSink(pkt); // process it
			std::cout<<"considerNew_as_data data for sink\n";
		}

		else{
			//  printf("Vectorbasedforward: %d is the not  target\n", here_.addr_);
			//TODO iscloseenough return false,surely,so pkt=0
			if (IsCloseEnough(pkt)) {
		std::cout<<"ConsiderNew as_data isclosenough\n";
        Vector * p1;
        p1=new Vector[1];
        p1[0].x=vbh.GetExtraInfo().f.x;
        p1[0].y=vbh.GetExtraInfo().f.y;
        p1[0].z=vbh.GetExtraInfo().f.z;
				double delay=CalculateDelay(pkt,p1);  //TODO should just pass the Vector for easier memory management here.
        delete p1;
				double d2=(Distance(pkt)-m_device->GetPhy()->GetTransRange())/ns3::SOUND_SPEED_IN_WATER; //gettransrange=0
				//printf("Vectorbasedforward: I am  not  target delay is %f d2=%f distance=%f\n",(sqrt(delay)*DELAY+d2*2),d2,Distance(pkt));
        //std::cout << "DElay is " << (sqrt(delay)*DELAY+d2*2) << " d2 is " << d2 << " distance is " << Distance(pkt) << "\n";
        SetDelayTimer(pkt,(sqrt(delay)*DELAY+d2*2));//set Tdelay=1.0
		 //SetDelayTimer(pkt,(sqrt(delay)*DELAY+d2));//set Tdelay=1.0
			}
			else { pkt=0; }
		}
		return;

	default:

		pkt=0;
		break;
	}
}


void
AquaSimVBF::Reset()
{
	PktTable.Reset();
	/*
	   for (int i=0; i<MAX_DATA_TYPE; i++) {
	   routing_table[i].Reset();
	   }
	 */
}

void
AquaSimVBF::Terminate()
{
  NS_LOG_DEBUG("AquaSimVBF::Terminate: Node=" << GetNetDevice()->GetAddress() <<
    ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<
    ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
}

void
AquaSimVBF::StopSource()
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
AquaSimVBF::CreatePacket()
{
  NS_LOG_FUNCTION(this);

	Ptr<Packet> pkt = Create<Packet>();

	if (pkt==NULL) return NULL;


  VBHeader vbh;
  AquaSimTrailer tra;
	//ash.SetSize(36);
	vbh.SetTs(Simulator::Now().ToDouble(Time::S));

	//!! I add new part

	vbh.SetExtraInfo_o(GetNetDevice()->GetPosition());
	vbh.SetExtraInfo_f(GetNetDevice()->GetPosition());

  pkt->AddHeader(vbh);
  pkt->AddTrailer(tra);
	return pkt;
}


Ptr<Packet>
AquaSimVBF::PrepareMessage(unsigned int dtype,
                            AquaSimAddress to_addr,
                            int msg_type)
{
	std::cout << "-----------vbf PrepareMessage-----------\n";
	Ptr<Packet> pkt = Create<Packet>();
  VBHeader vbh;
 

	vbh.SetMessType(msg_type);
	vbh.SetPkNum(m_pkCount);
	m_pkCount++;
	vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
	vbh.SetDataType(dtype);
	vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

	vbh.SetTs(Simulator::Now().ToDouble(Time::S));
	//    vbh->num_next = 1;
	// I am not sure if we need this
	// vbh->next_nodes[0] = to_addr;


	// I am not sure if we need it?
	/*
	   iph->src_ = here_;
	   iph->dst_ = to_addr;
	 */
  pkt->AddHeader(vbh);

	return pkt;
}

void
AquaSimVBF::MACprepare(Ptr<Packet> pkt)
{
	//std::cout << "-----------vbf MACprepare-----------\n";
  VBHeader vbh;
  AquaSimTrailer tra;

  pkt->RemoveHeader(vbh);
  pkt->RemoveTrailer(tra);
  vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));


	// printf("vectorbased: the mac_Broadcast is:%d\n",MAC_BROADCAST);
	tra.SetNextHop(AquaSimAddress::GetBroadcast());
	//ash.addr_type() = NS_AF_ILINK;
	// ash.SetTxTime(Seconds(0));
	// printf("vectorbased: the address type is :%d and suppose to be %d and  nexthop %d MAC_BROAD %d\n", ash->addr_type(),NS_AF_ILINK,ash->next_hop(),MAC_BROADCAST);
	//ash.SetDirection(AquaSimHeader::DOWN);
   
	/*if(!GetNetDevice()->GetSinkStatus()) {       //!! I add new part
    f = Vector(GetNetDevice()->CX(),
                  GetNetDevice()->CY(),
                  GetNetDevice()->CZ());
	}
	else{*/
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
	//}
  vbh.SetExtraInfo_f(model->GetPosition());

  pkt->AddHeader(vbh);
  pkt->AddTrailer(tra);
	// printf("vectorbasedforward: last line MACprepare\n");
}


void
AquaSimVBF::MACsend(Ptr<Packet> pkt, double delay)
{
	//std::cout << "-----------vbf MACsend-----------\n";
  NS_LOG_INFO("MACsend: delay " << delay << " at time " << Simulator::Now().GetSeconds());

  VBHeader vbh;
  AquaSimTrailer tra;

  pkt->RemoveTrailer(tra);
  pkt->RemoveHeader(vbh);
  //pkt->RemovePacketTag(ptag);

	// I don't understand why it works like this way
	/*
	   if (vbh.GetMessType() == AS_DATA)
	   ash->size() = (God::instance()->data_pkt_size) + 4*(vbh.GetPkNum() - 1);
	   else
	   ash->size() = 36 + 4*(vbh.GetPkNum() -1);
	 */


	/*
	   if (vbh.GetMessType() == AS_DATA)
	   // ash->size() = (God::instance()->data_pkt_size)+12 ;
	   ash->size() = 65+12 ;
	   else
	   ash->size() =32;
	 */



    std::cout<<"Node:"<<vbh.GetForwardAddr()<<" forward packet!\n";
	//if(!ll) printf("ok, the LL is empty\n");
	//ptag.SetPacketType(PT_UWVB);
	//printf("vectorbased: the address type is :%d uid is %d\n", ash->addr_type(),pkt->uid_);
	//printf("vectorbased: the packet type is :%d\n", ptag.GetPacketType());
	// ll->handle(pkt);
  pkt->AddHeader(vbh);
  pkt->AddTrailer(tra);

  Simulator::Schedule(Seconds(delay),&AquaSimRouting::SendDown,this,
                        pkt,AquaSimAddress::GetBroadcast(),Seconds(0));
}


void
AquaSimVBF::DataForSink(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->RemoveHeader(vbh);
  if (!SendUp(pkt))
    NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}


void
AquaSimVBF::SetDelayTimer(Ptr<Packet> pkt, double c)
{
  NS_LOG_FUNCTION(this << c);
  if(c<0)c=0;
  Simulator::Schedule(Seconds(c),&AquaSimVBF::Timeout,this,pkt);
}

void
AquaSimVBF::Timeout(Ptr<Packet> pkt)
{
  VBHeader vbh;

  pkt->PeekHeader(vbh);

	unsigned char msg_type =vbh.GetMessType();
	std::cout<<"Timeout:"<<msg_type<<"\n";
	vbf_neighborhood  *hashPtr;
	//Ptr<Packet> p1;

	switch (msg_type) {

	case AS_DATA:
		hashPtr= PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());
		if (hashPtr != NULL) {
			int num_neighbor=hashPtr->number;
			std::cout<<"Timeout as_data n-neighbor:"<<num_neighbor<<"\n";
			// printf("vectorbasedforward: node %d have received %d when wake up at %f\n",here_.addr_,num_neighbor,NOW);
			if (num_neighbor!=1) {

				/*AlohaOverhear can guarantee that the packet be successfully deliveried to next hop,
				 * so we release the pkt if overhearing other neighbors send this pkt.
				 */
				/* if( m_useOverhear )
				   {
				         pkt=0;
				         return;
				   }*/

				// Some guys transmit the data before me
				if (num_neighbor==MAX_NEIGHBOR) {
					//I have too many neighbors, I quit
					pkt=0;
					return;
				}
				else //I need to calculate my delay time again
				{
					int i=0;
					Vector* tp;
					tp=new Vector[1];

					tp[0].x=hashPtr->neighbor[i].x;
					tp[0].y=hashPtr->neighbor[i].y;
					tp[0].z=hashPtr->neighbor[i].z;
					double tdelay=CalculateDelay(pkt,tp);
					//neighbor's tdelay
					// double tdelay=5;
					i++;
					double c=1;
					while (i<num_neighbor) {
						c=c*2;
						tp[0].x=hashPtr->neighbor[i].x;
						tp[0].y=hashPtr->neighbor[i].y;
						tp[0].z=hashPtr->neighbor[i].z;
						double t2delay=CalculateDelay(pkt,tp);
						if (t2delay<tdelay)
							tdelay=t2delay;
						i++;
						std::cout<<"Timeout i:"<<i<<"calculatedelay:"<<tdelay<<"\n";
					}

					delete tp;
					if(tdelay<=(m_priority/c)) {
						MACprepare(pkt);
						MACsend(pkt,0);
						std::cout<<"Timeout delay=a less than priority,then macsend\n";
					}
					else{
						pkt=0; //to much overlap, don;t send
                        }
				}// end of calculate my new delay time
			}
			else{// I am the only neighbor
				Vector* tp;
				tp=new Vector[1];
				tp[0].x=vbh.GetExtraInfo().f.x;
				tp[0].y=vbh.GetExtraInfo().f.y;
				tp[0].z=vbh.GetExtraInfo().f.z;
				double delay=CalculateDelay(pkt,tp);
                std::cout<<"Timeout only neighbor delay:"<<delay<<"\n";
				delete tp;
				if (delay<=m_priority) {
					// printf("vectorbasedforward: !!%f\n",delay);
					MACprepare(pkt);
					//MACsend(pkt,0);
					MACsend(pkt,0);
					std::cout<<"Timeout only neighbor and delay=a less than priority,then macsend\n";
				}
				else  {pkt=0; }
				// printf("vectorbasedforward:  I%d am the only neighbor, I send it out at %f\n",here_.addr_,NOW);
				return;
			}
		}
		break;
	default:
		pkt=0;
		break;
	}
}

void
AquaSimVBF::CalculatePosition(Ptr<Packet> pkt)
{
  //not used.
  VBHeader vbh;
  pkt->PeekHeader(vbh);

	double fx=vbh.GetExtraInfo().f.x;
	double fy=vbh.GetExtraInfo().f.y;
	double fz=vbh.GetExtraInfo().f.z;

	double dx=vbh.GetExtraInfo().d.x;
	double dy=vbh.GetExtraInfo().d.y;
	double dz=vbh.GetExtraInfo().d.z;

	GetNetDevice()->CX()=fx+dx;
	GetNetDevice()->CY()=fy+dy;
	GetNetDevice()->CZ()=fz+dz;
	// printf("vectorbased: my position is computed as (%f,%f,%f)\n",GetNetDevice()->CX_, GetNetDevice()->CY_,GetNetDevice()->CZ_);
}

double
AquaSimVBF::CalculateDelay(Ptr<Packet> pkt,Vector* p1)//calculate a,rather than delay
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);

	double fx=p1->x;
	double fy=p1->y;
	double fz=p1->z;

  Vector pos = GetNetDevice()->GetPosition();
	double dx=pos.x-fx;
	double dy=pos.y-fy;
	double dz=pos.z-fz;

	double tx=vbh.GetExtraInfo().t.x;
	double ty=vbh.GetExtraInfo().t.y;
	double tz=vbh.GetExtraInfo().t.z;

	double dtx=tx-fx;
	double dty=ty-fy;
	double dtz=tz-fz;

	double dp=dx*dtx+dy*dty+dz*dtz;

	// double a=Advance(pkt);
	double p=Projection(pkt);
	double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
	double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
  double cos_theta;
  if (d ==0 || l==0) cos_theta=0;
  else cos_theta=dp/(d*l);
	// double delay=(TRANSMISSION_DISTANCE-d*cos_theta)/TRANSMISSION_DISTANCE;
	double delay=(p/m_width) +((m_device->GetPhy()->GetTransRange()-d*cos_theta)/m_device->GetPhy()->GetTransRange());
	// double delay=(p/m_width) +((TRANSMISSION_DISTANCE-d)/TRANSMISSION_DISTANCE)+(1-cos_theta);
	//printf("vectorbased: node(%d) projection is %f, and cos is %f, and d is %f)\n",here_.addr_,p, cos_theta, d);
  NS_LOG_DEBUG("CalculateDelay(" << GetNetDevice()->GetAddress() << ") projection is "
      << p << ", cos is " << cos_theta << " and d is " << d << " and total delay is " << delay);
  return delay;
}

void AquaSimVBF::printEnergy(AquaSimAddress local){
	std::ofstream outfile("NodeResiEnergy.txt", std::ios::app);
	double resiEn=GetNetDevice()->EnergyModel()->GetEnergy();
			 outfile<</*"PktID:"<<pktID<<*/"Node:"<<local/*<<"  ResiEnergy:"*/<<" "<<resiEn<<"\n";
	 outfile.close();
}
double
AquaSimVBF::Distance(Ptr<Packet> pkt)//d
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);

  double tx=vbh.GetExtraInfo().f.x;
	double ty=vbh.GetExtraInfo().f.y;
	double tz=vbh.GetExtraInfo().f.z;
	// printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
  Vector pos = GetNetDevice()->GetPosition();
	//double x=GetNetDevice()->CX(); //change later
	//double y=GetNetDevice()->CY();// printf(" Vectorbasedforward: I am in advanced\n");
	//double z=GetNetDevice()->CZ();
	// printf("the target is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
	return sqrt((tx-pos.x)*(tx-pos.x)+(ty-pos.y)*(ty-pos.y)+ (tz-pos.z)*(tz-pos.z));
}

double
AquaSimVBF::Advance(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);

	double tx=vbh.GetExtraInfo().t.x;
	double ty=vbh.GetExtraInfo().t.y;
	double tz=vbh.GetExtraInfo().t.z;
	//TODO
	std::cout<<"Advance target position is:"<<tx<<","<<ty<<","<<tz<<'\n';
	// printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
  Vector pos = GetNetDevice()->GetPosition();
  std::cout<<"Advance current position is:"<<pos.x<<","<<pos.y<<","<<pos.z<<'\n';
	//double x=GetNetDevice()->CX(); //change later
	//double y=GetNetDevice()->CY();// printf(" Vectorbasedforward: I am in advanced\n");
	//double z=GetNetDevice()->CZ();
	// printf("the target is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
	return sqrt((tx-pos.x)*(tx-pos.x)+(ty-pos.y)*(ty-pos.y)+ (tz-pos.z)*(tz-pos.z));
}

double
AquaSimVBF::Projection(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);

	double tx=vbh.GetExtraInfo().t.x;
	double ty=vbh.GetExtraInfo().t.y;
	double tz=vbh.GetExtraInfo().t.z;

  Vector o;
	//double ox, oy, oz;
std::cout <<"m_hopByHop:"<<m_hopByHop << std::endl;
	if( !m_hopByHop )
	{
		std::cout<<"enter !m_hopByHop" <<std::endl;
		//printf("vbf is used\n");
		o.x=vbh.GetExtraInfo().o.x;
		o.y=vbh.GetExtraInfo().o.y;
		o.z=vbh.GetExtraInfo().o.z;
	}
	else{
		std::cout<<"not enter !m_hopByHop" <<std::endl;
		//printf("m_hopByHop vbf is used\n");
		o.x=vbh.GetExtraInfo().f.x;
		o.y=vbh.GetExtraInfo().f.y;
		o.z=vbh.GetExtraInfo().f.z;
	}

  //NOTE below may not work if the nodes are mobile.
	/*double x=GetNetDevice()->CX();
	double y=GetNetDevice()->CY();
	double z=GetNetDevice()->CZ();
  */
  Vector myPos = GetNetDevice()->GetPosition();

	double wx=tx-o.x;
	double wy=ty-o.y;
	double wz=tz-o.z;

	double vx=myPos.x-o.x;
	double vy=myPos.y-o.y;
	double vz=myPos.z-o.z;
    //xiang liang cha cheng qiu mian ji,ci chu qiu wan hai shi xiang liang
	double cross_product_x=vy*wz-vz*wy;
	double cross_product_y=vz*wx-vx*wz;
	double cross_product_z=vx*wy-vy*wx;
	std::cout<<"cross_product_x:"<<cross_product_x<<std::endl;
	std::cout<<"cross_product_y:"<<cross_product_y<<std::endl;
	std::cout<<"cross_product_z:"<<cross_product_z<<std::endl;

	std::cout<<"vx-vy-vz:"<<vx<<"-"<<vy<<"-"<<vz<<std::endl;
	std::cout<<"wx-wy-wz:"<<wx<<"-"<<wy<<"-"<<wz<<std::endl;
	std::cout<<"vy*wz:"<<vy*wz<<std::endl;
	std::cout<<"vz*wy:"<<vz*wy<<std::endl;
    //mian ji wei cha cheng hou de mo
	double area=sqrt(cross_product_x*cross_product_x+
	                 cross_product_y*cross_product_y+cross_product_z*cross_product_z);
	double length=sqrt((tx-o.x)*(tx-o.x)+(ty-o.y)*(ty-o.y)+ (tz-o.z)*(tz-o.z));
	// printf("vectorbasedforward: the area is %f and length is %f\n",area,length);
  NS_LOG_DEBUG("Projection: area is " << area << " length is " << length);
  std::cout<<"Projection: area is " << area << " length is " << length<<std::endl;
  if (length==0) return 0;
  std::cout<<"Projection="<<area/length<<std::endl;
	return area/length;
}

bool
AquaSimVBF::IsTarget(Ptr<Packet> pkt)
{
  VBHeader vbh;
  pkt->PeekHeader(vbh);

  //TODO
    //if (vbh.GetTargetAddr().GetAsInt()==1) {
	if (vbh.GetTargetAddr().GetAsInt()==0) {

		//  printf("vectorbased: advanced is %lf and my range is %f\n",Advance(pkt),vbh.GetRange());
		return (Advance(pkt)<vbh.GetRange());
	}
	else return(GetNetDevice()->GetAddress()==vbh.GetTargetAddr());
}


bool
AquaSimVBF::IsCloseEnough(Ptr<Packet> pkt)
{
  //VBHeader vbh;
  //AquaSimHeader ash;
  //pkt->RemoveHeader(ash);
  //pkt->PeekHeader(vbh);
  //pkt->AddHeader(ash);

  //double range=vbh.GetRange();  //unused
	//double range=m_width;

	//  printf("underwatersensor: The m_width is %f\n",range);
	//double ox=vbh.GetExtraInfo().o.x;  //unused
	//double oy=vbh.GetExtraInfo().o.y;  //unused
	//double oz=vbh.GetExtraInfo().o.z;  //unused

	/*
  double tx=vbh.GetExtraInfo().t.x;
	double ty=vbh.GetExtraInfo().t.y;
	double tz=vbh.GetExtraInfo().t.z;

	double fx=vbh.GetExtraInfo().f.x;
	double fy=vbh.GetExtraInfo().f.y;
	double fz=vbh.GetExtraInfo().f.z;
  */  //currently unused...

	//double x=GetNetDevice()->CX();  //unused  //change later
	//double y=GetNetDevice()->CY();  //unused
	//double z=GetNetDevice()->CZ();  //unused

	//double d=sqrt((tx-fx)*(tx-fx)+(ty-fy)*(ty-fy)+(tz-fz)*(tz-fz));  //unused
	//double l=sqrt((tx-ox)*(tx-ox)+(ty-oy)*(ty-oy)+ (tz-oz)*(tz-oz));
	//double l=Advance(pkt);  //unused
	// if (l<range)
	// {
	// printf("vectorbasedforward: IsClose?too close! it should be target!\n");
	// return true;
	// }
	// else {
	//double c=d/l;
	double c=1;
	// if ((d<=range)&&((z-oz)<0.01))  return true;
	//TODO
	//if ((Projection(pkt)>=(c*m_width)))  return true;
	std::cout<<"enter isclose enough"<< std::endl;
	std::cout<<"c*m_width"<<c*m_width<<std::endl;
	if ((Projection(pkt)<=(c*m_width)))
		std::cout<<"AquaSimVBF::IsCloseEnough : Projection(pkt):"<<Projection(pkt)<< std::endl;
		return true;
	return false;

}

void AquaSimVBF::DoDispose()
{
  m_rand=0;
  AquaSimRouting::DoDispose();
}

void AquaSimVBF::SetTargetPos(Vector pos)
{
  m_targetPos = pos;
}


// Some methods for Flooding Entry

/*
   void Vectorbasedforward_Entry::reset()
   {
   clear_agentlist(source);
   clear_agentlist(sink);
   source = NULL;
   sink = NULL;
   }

   void Vectorbasedforward_Entry::clear_agentlist(Agent_List *list)
   {
   Agent_List *cur=list;
   Agent_List *temp = NULL;

   while (cur != NULL) {
   temp = AGENT_NEXT(cur);
   delete cur;
   cur = temp;
   }
   }

 */
