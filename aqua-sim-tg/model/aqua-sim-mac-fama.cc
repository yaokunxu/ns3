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

#include "aqua-sim-mac-fama.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-address.h"
#include "aqua-sim-header-routing.h"
#include <ctime>
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/integer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimFama");
NS_OBJECT_ENSURE_REGISTERED(AquaSimFama);


AquaSimFama::AquaSimFama(): FamaStatus(PASSIVE), m_NDPeriod(4.0), m_maxBurst(1),
		m_dataPktInterval(0.00001), m_estimateError(0.001),m_dataPktSize(1600),
		m_neighborId(0), m_waitCTSTimer(Timer::CANCEL_ON_DESTROY),
		m_backoffTimer(Timer::CANCEL_ON_DESTROY), m_remoteTimer(Timer::CANCEL_ON_DESTROY),
		m_remoteExpireTime(-1), m_famaNDCounter(4)
		//, backoff_timer(this), status_handler(this), NDTimer(this),
		//WaitCTSTimer(this),DataBackoffTimer(this),RemoteTimer(this), CallBack_Handler(this)
{

  m_transmitDistance=1000.0;
  m_maxPropDelay = Seconds(m_transmitDistance/1500.0);//2
  m_RTSTxTime = m_maxPropDelay;//2
  m_CTSTxTime = m_RTSTxTime + 2*m_maxPropDelay;//6

  m_maxDataTxTime = MilliSeconds(m_dataPktSize/m_bitRate);  //1600bits/10kbps   0.16

  m_rand = CreateObject<UniformRandomVariable> ();
  //a=Seconds(m_rand->GetValue(0.0,m_NDPeriod)+0.000001);
Time a;
a=Seconds(0);

  Simulator::Schedule(a, &AquaSimFama::NDTimerExpire, this);
}

AquaSimFama::~AquaSimFama()
{
}

TypeId
AquaSimFama::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimFama")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimFama>()
      .AddAttribute("MaxBurst", "The maximum number of packet burst. default is 1",
	IntegerValue(1),
	MakeIntegerAccessor (&AquaSimFama::m_maxBurst),
	MakeIntegerChecker<int>())
    ;
  return tid;
}

int64_t
AquaSimFama::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

void
AquaSimFama::NDTimerExpire()
{
  NS_LOG_FUNCTION(this);
  SendPkt(MakeND());
  m_famaNDCounter--;

  if (m_famaNDCounter > 0) {
    Simulator::Schedule(Seconds(m_rand->GetValue(0.0,m_NDPeriod)), &AquaSimFama::NDTimerExpire, this);
	}
}

void
AquaSimFama::SendPkt(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this);
  AquaSimHeader asHeader;
  pkt->RemoveHeader(asHeader);

  asHeader.SetDirection(AquaSimHeader::DOWN);

  Time txtime = asHeader.GetTxTime();

  switch( m_device->GetTransmissionStatus() ) {
    case SLEEP:
      PowerOn();
    case NIDLE:
      //m_device->SetTransmissionStatus(SEND);
      asHeader.SetTimeStamp(Simulator::Now());
      pkt->AddHeader(asHeader);

      SendDown(pkt);

      break;
    case RECV:
      NS_LOG_WARN("RECV-SEND Collision!!!!!");
      pkt=0;
      break;
    default:
      //status is SEND
      NS_LOG_INFO("SendPkt node:" << m_device->GetNode() << " send data too fast");
      std::cout<<"default"<<std::endl;
      pkt=0;
  }
  return;
}

bool
AquaSimFama::TxProcess(Ptr<Packet> pkt)
{
  //callback to higher level, should be implemented differently
  //Scheduler::instance().schedule(&CallBack_Handler, &m_callbackEvent, CALLBACK_DELAY);

  if( NeighborList.empty() ) {
      pkt=0;
      return false;
  }
  //std::cout<<"size!"<<std::endl;

  //std::cout<<NeighborList.size()<<std::endl;
  //figure out how to cache the packet will be sent out!!!!!!!
  AquaSimHeader asHeader;
	VBHeader vbh;
  FamaHeader FamaH;
	MacHeader mach;
	AquaSimPtTag ptag;
  pkt->RemoveHeader(asHeader);
	pkt->RemoveHeader(mach);
  pkt->RemoveHeader(FamaH);
	pkt->RemovePacketTag(ptag);

	asHeader.SetSize(m_dataPktSize);
  asHeader.SetTxTime(m_maxDataTxTime);
  asHeader.SetErrorFlag(false);
  asHeader.SetDirection(AquaSimHeader::DOWN);
  //UpperLayerPktType = ptag.GetPacketType();

  asHeader.SetNextHop(NeighborList[m_neighborId]);
  m_neighborId = (m_neighborId+1)%NeighborList.size();
	ptag.SetPacketType(AquaSimPtTag::PT_FAMA);

  vbh.SetTargetAddr(asHeader.GetNextHop());

  FamaH.SetPType(FamaHeader::FAMA_DATA);
  FamaH.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  FamaH.SetDA(asHeader.GetNextHop());

	pkt->AddHeader(FamaH);
	pkt->AddHeader(mach);
  pkt->AddHeader(asHeader);
	pkt->AddPacketTag(ptag);
	PktQ.push(pkt);
  //fill the next hop when sending out the packet;
  if( (PktQ.size() == 1) /*the pkt is the first one*/
    && FamaStatus == PASSIVE ) {
      if( CarrierDected() ) {
   //m_maxDataTxTime
	 //DoRemote(2*m_maxPropDelay+m_estimateError);
    	DoRemote(2*m_maxPropDelay+m_maxDataTxTime);
      }
      else{
	 //SendRTS(2*m_maxPropDelay+m_CTSTxTime+m_RTSTxTime+m_estimateError);
     SendRTS(2*m_maxPropDelay);

      }

  }
  else
  {
	  Simulator::Schedule(Seconds(m_rand->GetValue(0.0,m_NDPeriod)), &AquaSimFama::TxProcess, this,pkt);
  }
  return true;
}


bool
AquaSimFama::RecvProcess(Ptr<Packet> pkt)
{
	NS_LOG_FUNCTION(this);

  AquaSimHeader asHeader;
	MacHeader mach;
  FamaHeader FamaH;
	AquaSimPtTag ptag;
  pkt->RemoveHeader(asHeader);
	pkt->RemoveHeader(mach);
  pkt->PeekHeader(FamaH);
	pkt->PeekPacketTag(ptag);
	pkt->AddHeader(mach);
	pkt->AddHeader(asHeader);
  AquaSimAddress dst = FamaH.GetDA();


//  RTS,	//the previous forwarder thinks this is DATA-ACK
//  CTS,
//  FAMA_DATA,
//  ND		//neighbor discovery. need know neighbors, so it can be used as next hop.
//} packet_type;


/*  if(FamaH.GetPType()!=FamaHeader::ND)
  {
	  //std::cout<<"hhh"<<std::endl;
     // std::cout<<"hhh SA"<<FamaH.GetSA()<<"  DA"<<FamaH.GetDA()<<std::endl;
     // std::cout<<"And I am "<<m_device->GetAddress()<<std::endl;
      switch (FamaH.GetPType()){
      case FamaHeader::RTS:
        break;
      case FamaHeader::CTS:
        break;
      case FamaHeader::FAMA_DATA:
        break;
      default:
        std::cout<<"I do not know!!"<<std::endl;


      }
      //std::cout<<"hhh"<<std::endl;
  }*/
  if( m_backoffTimer.IsRunning() ) {
      m_backoffTimer.Cancel();
      //DoRemote(2*m_maxPropDelay+m_estimateError);//4.0001
      DoRemote(2*m_maxPropDelay+m_maxDataTxTime);
  } else if( m_remoteTimer.IsRunning() ) {
      m_remoteTimer.Cancel();
      m_remoteExpireTime = Seconds(-1);
  }

  /*ND is not a part of AquaSimFama. We just want to use it to get next hop
   *So we do not care wether it collides with others
   */
  if( ( ptag.GetPacketType() == AquaSimPtTag::PT_FAMA)&& (FamaH.GetPType()==FamaHeader::ND) ) {
      ProcessND(FamaH.GetSA());
      pkt=0;
      return false;
  }


  if( asHeader.GetErrorFlag() )
  {
      NS_LOG_INFO("RecvProcess: pkt:" << pkt << " is corrupted at time " << Simulator::Now().GetSeconds());
      //if(drop_)
	//drop_->recv(pkt,"Error/Collision");
    //else
	pkt=0;
    std::cout<<"GetErrorFlag!"<<std::endl;
      DoRemote(2*m_maxPropDelay+m_estimateError);//4.0001
      return false;
  }
  if( !m_waitCTSTimer.IsRunning() &&FamaH.GetPType()==FamaHeader::CTS) {
	//  std::cout<<m_device->GetAddress()<<" m_waitCTSTimer is not running!"<<std::endl;
	 // std::cout<<"so now i am "<<FamaStatus<<std::endl;
  }

  if( FamaStatus==WAIT_CTS ) {
	  //printf("%f: node %d receive RTS\n", NOW, index_);
	  //std::cout<<m_device->GetAddress()<<" waiting Timer is running!"<<std::endl;
      m_waitCTSTimer.Cancel();
      if(( ptag.GetPacketType() == AquaSimPtTag::PT_FAMA )&&(FamaH.GetPType()==FamaHeader::CTS)
	      && (asHeader.GetNextHop() == m_device->GetAddress())) {
	  //receiving the CTS
     //std::cout<<m_device->GetAddress()<<" recieve CTS from "<<FamaH.GetSA()<<" !"<<std::endl;
    //  std::cout<<m_device->GetAddress()<<" is going to send data to "<<FamaH.GetSA()<<std::endl;
	  SendDataPkt();

	//  std::cout<<m_device->GetAddress()<<" end send dataPackage!"<<std::endl;
      }
      else {
	  DoBackoff();
      }
      pkt=0;
      return true;
  }


  if( ptag.GetPacketType() == AquaSimPtTag::PT_FAMA ) {
      switch( FamaH.GetPType() ) {
	case FamaHeader::RTS:
	  //printf("%f: node %d receive RTS\n", NOW, index_);
	  if( dst == m_device->GetAddress() ) {
		//  std::cout<<m_device->GetAddress()<<" recieve RTS from "<<FamaH.GetSA()<<" !"<<std::endl;
	      ProcessRTS(FamaH.GetSA());
	  }
	  DoRemote(m_CTSTxTime+2*m_maxPropDelay+m_estimateError);//10.001
	  break;

	case FamaHeader::CTS:
	  //printf("%f: node %d receive CTS\n", NOW, index_);
	  // this CTS must not be for this node
	  //DoRemote(2*m_maxPropDelay+m_estimateError);//4.001
		DoRemote(2*m_maxPropDelay+m_maxDataTxTime);//4.001
	  break;
	default:
	  //printf("%f: node %d receive DATA\n", NOW, index_);
	  //process Data packet
	  if( dst == m_device->GetAddress() ) {
	      //ptag.SetPacketType(UpperLayerPktType);
		//  std::cout<<m_device->GetAddress()<<" recieve data from "<<FamaH.GetSA()<<std::endl;
	    	SendUp(pkt);
	    //	std::cout<<m_device->GetAddress()<<" recieve data from "<<FamaH.GetSA()<<" then send it to routing!"<<std::endl;
	    	return true;
	  }
	  else {
		DoRemote(m_maxPropDelay+m_estimateError);//2.001
	  }
      }
  }
  pkt=0;
  return true;
}

void
AquaSimFama::SendDataPkt()
{
	NS_LOG_FUNCTION(this);

     int PktQ_Size = PktQ.size();
     int SentPkt = 0;
     Time StartTime = Simulator::Now();


    AquaSimHeader asHeader;
    PktQ.front()->PeekHeader(asHeader);
    AquaSimAddress recver = asHeader.GetNextHop();
    Ptr<Packet> tmp;
//    std::cout<<m_device->GetAddress()<<" is sending dataPkt to "<<recver<<std::endl;



  for(int i=0; i<PktQ_Size && SentPkt<m_maxBurst; i++) {
    tmp = PktQ.front();
    tmp->PeekHeader(asHeader);

  	MacHeader mach;
  	tmp->RemoveHeader(asHeader);
  	tmp->RemoveHeader(mach);
  	mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
  	tmp->AddHeader(mach);
  	tmp->AddHeader(asHeader);
 //   std::cout<<"SendDataPkt: "<<"SA "<<asHeader.GetSAddr()<<" DA "<<asHeader.GetDAddr()<<std::endl;




    PktQ.pop();
    if( asHeader.GetNextHop() == recver ) {
	SentPkt++;
	Simulator::Schedule(StartTime-Simulator::Now(),&AquaSimFama::ProcessDataSendTimer, this, tmp);
	if(!PktQ.empty())
	PktQ.front()->PeekHeader(asHeader);
	if( !PktQ.empty() ) {
	  StartTime += asHeader.GetTxTime() + m_dataPktInterval;
	}
	else {
	  break;
	}
    }
    else{
	PktQ.push(tmp);
    }
  }

  FamaStatus = WAIT_DATA_FINISH;

  Simulator::Schedule(m_maxPropDelay+StartTime-Simulator::Now(),&AquaSimFama::ProcessDataBackoffTimer,this);
}

void
AquaSimFama::ProcessDataSendTimer(Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION(this << pkt);
 // std::cout<<"ProcessDataSendTimer: i am really sending data!"<<std::endl;
  SendPkt(pkt);
 // std::cout<<"ProcessDataSendTimer: i am really sending data!"<<std::endl;
}


void
AquaSimFama::ProcessDataBackoffTimer()
{
  if( !PktQ.empty() )
    DoBackoff();
  else
    FamaStatus = PASSIVE;
}


Ptr<Packet>
AquaSimFama::MakeND()
{
	NS_LOG_FUNCTION(this);

  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader asHeader;
	MacHeader mach;
  FamaHeader FamaH;
	AquaSimPtTag ptag;

	asHeader.SetSize(2*sizeof(AquaSimAddress)+1);
  asHeader.SetTxTime(GetTxTime(asHeader.GetSize()));
  asHeader.SetErrorFlag(false);
  asHeader.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_FAMA);
  asHeader.SetNextHop(AquaSimAddress::GetBroadcast());

  FamaH.SetPType(FamaHeader::ND);
  FamaH.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  FamaH.SetDA(AquaSimAddress::GetBroadcast());


	pkt->AddHeader(FamaH);
	pkt->AddHeader(mach);
  pkt->AddHeader(asHeader);
	pkt->AddPacketTag(ptag);
  return pkt;
}


void
AquaSimFama::ProcessND(AquaSimAddress sa)
{
  //FamaHeader FamaH;
  //pkt->PeekHeader(FamaH);
	NeighborList.push_back(sa);
}


Ptr<Packet>
AquaSimFama::MakeRTS(AquaSimAddress Recver)
{
  NS_LOG_FUNCTION(this);

  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader asHeader;
	MacHeader mach;
  FamaHeader FamaH;
	AquaSimPtTag ptag;

  asHeader.SetSize(GetSizeByTxTime(m_RTSTxTime.ToDouble(Time::S)));
  asHeader.SetTxTime(m_RTSTxTime);
  asHeader.SetErrorFlag(false);
  asHeader.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_FAMA);
  asHeader.SetNextHop(Recver);

  FamaH.SetPType(FamaHeader::RTS);
  FamaH.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  FamaH.SetDA(Recver);

	pkt->AddHeader(FamaH);
	pkt->AddHeader(mach);
  pkt->AddHeader(asHeader);
	pkt->AddPacketTag(ptag);
  return pkt;
}


void
AquaSimFama::SendRTS(Time DeltaTime)
{
  NS_LOG_FUNCTION(this);

  AquaSimHeader asHeader;
  PktQ.front()->PeekHeader(asHeader);
  SendPkt( MakeRTS(asHeader.GetNextHop()) );
 // std::cout<<"SendRTS:  "<<m_device->GetAddress()<<" is sending RTS to "<<asHeader.GetNextHop()<<"!"<<std::endl;
  FamaStatus = WAIT_CTS;

  m_waitCTSTimer.SetDelay(DeltaTime);
  m_waitCTSTimer.SetFunction(&AquaSimFama::DoBackoff,this);

}


void
AquaSimFama::ProcessRTS(AquaSimAddress sa)
{
  //FamaHeader FamaH;
  //pkt->PeekHeader(FamaH);
//	std::cout<<m_device->GetAddress()<<" is sending CTS to "<<sa<<std::endl;
  SendPkt( MakeCTS(sa) );
  FamaStatus = WAIT_DATA;
}



Ptr<Packet>
AquaSimFama::MakeCTS(AquaSimAddress RTS_Sender)
{
  NS_LOG_FUNCTION(this << RTS_Sender);

  Ptr<Packet> pkt = Create<Packet>();
  AquaSimHeader asHeader;
	MacHeader mach;
	FamaHeader FamaH;
	AquaSimPtTag ptag;

  asHeader.SetSize(GetSizeByTxTime(m_CTSTxTime.ToDouble(Time::S)));
  asHeader.SetTxTime(m_CTSTxTime);
  asHeader.SetErrorFlag(false);
  asHeader.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_FAMA);
  asHeader.SetNextHop(RTS_Sender);

  FamaH.SetPType(FamaHeader::CTS);
  FamaH.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
  FamaH.SetDA(RTS_Sender);

	pkt->AddHeader(FamaH);
	pkt->AddHeader(mach);
  pkt->AddHeader(asHeader);
	pkt->AddPacketTag(ptag);
  return pkt;
}


bool
AquaSimFama::CarrierDected()
{
  if( m_device->GetTransmissionStatus() == RECV
	  || m_device->GetTransmissionStatus() == SEND )  {
	  return true;
  }
	return false;
}

void
AquaSimFama::DoBackoff()
{
  Time backoffTime = MilliSeconds(m_rand->GetValue(0.0,10 * m_RTSTxTime.ToDouble(Time::MS)));
  FamaStatus = BACKOFF;
  if( m_backoffTimer.IsRunning() ) {
      m_backoffTimer.Cancel();
  }

  Simulator::Schedule(backoffTime,&AquaSimFama::BackoffTimerExpire,this);
 // m_backoffTimer.SetDelay(backoffTime);
 // m_backoffTimer.SetFunction(&AquaSimFama::BackoffTimerExpire,this);
}


void
AquaSimFama::DoRemote(Time DeltaTime)
{
  FamaStatus = REMOTE;
  if( Simulator::Now()+DeltaTime > m_remoteExpireTime ) {
      m_remoteExpireTime = Simulator::Now()+DeltaTime;
      if( m_remoteTimer.IsRunning() ) {
	  m_remoteTimer.Cancel();
      }
          //  Time a;
           // a=Seconds(0.1);
Simulator::Schedule(DeltaTime,&AquaSimFama::ProcessRemoteTimer,this);


//      m_remoteTimer.SetDelay(a);
//     m_remoteTimer.SetFunction(&AquaSimFama::ProcessRemoteTimer,this);



  }
}


void
AquaSimFama::ProcessRemoteTimer()
{
  if( PktQ.empty() ) {
    FamaStatus = PASSIVE;
  }
  else {
    DoBackoff();
    //SendRTS(2*m_maxPropDelay+m_CTSTxTime+m_RTSTxTime+m_estimateError);
  }
}

void
AquaSimFama::BackoffTimerExpire()
{
  //SendRTS(2*m_maxPropDelay + m_RTSTxTime + m_CTSTxTime +m_estimateError);
	SendRTS(2*m_maxPropDelay);
}

void AquaSimFama::DoDispose()
{
	m_rand=0;
	while(!PktQ.empty()) {
		PktQ.front()=0;
		PktQ.pop();
	}
	AquaSimMac::DoDispose();
}

} // namespace ns3
