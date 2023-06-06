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

#include "aqua-sim-mac-phy-fama.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-address.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header-transport.h"
#include <ctime>
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/integer.h"
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimPhyFama");
NS_OBJECT_ENSURE_REGISTERED(AquaSimPhyFama);

AquaSimPhyFama::AquaSimPhyFama() :
		m_NDPeriod(4.0), m_maxBurst(1), m_dataPktInterval(0.00001), m_estimateError(
				0.001), m_dataPktSize(1600), m_waitCTSTimer(
				Timer::CANCEL_ON_DESTROY) {
	m_transmitDistance = 1000.0;
	m_maxPropDelay = Seconds(m_transmitDistance / 1500.0);		//2
	m_RTSTxTime = m_maxPropDelay;		//2
	m_CTSTxTime = m_RTSTxTime + 2 * m_maxPropDelay;		//6
	m_maxDataTxTime = MilliSeconds(m_dataPktSize / m_bitRate); //1600bits/10kbps   0.16
	m_rand = CreateObject<UniformRandomVariable>();
	PktQ = std::queue<Ptr<Packet>>();
}

AquaSimPhyFama::~AquaSimPhyFama() {
}

TypeId AquaSimPhyFama::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::AquaSimPhyFama").SetParent<AquaSimMac>().AddConstructor<
					AquaSimPhyFama>().AddAttribute("MaxBurst",
					"The maximum number of packet burst. default is 1",
					IntegerValue(1),
					MakeIntegerAccessor(&AquaSimPhyFama::m_maxBurst),
					MakeIntegerChecker<int>());
	return tid;
}

int64_t AquaSimPhyFama::AssignStreams(int64_t stream) {
	NS_LOG_FUNCTION(this << stream);
	m_rand->SetStream(stream);
	return 1;
}

void AquaSimPhyFama::SendPkt(Ptr<Packet> pkt) {

	MacHeader mach;
	AquaSimHeader asHeader;
	AquaSimPtTag ptag;
	pkt->RemoveHeader(mach);
	pkt->RemoveHeader(asHeader);
	//pkt->RemovePacketTag(ptag);
	asHeader.SetDirection(AquaSimHeader::DOWN);
	Time txtime = asHeader.GetTxTime();

	switch (m_device->GetTransmissionStatus()) {
	case SLEEP:
		PowerOn();
	case NIDLE:
		asHeader.SetTimeStamp(Simulator::Now());
		//pkt->AddPacketTag(ptag);
		pkt->AddHeader(asHeader);
		pkt->AddHeader(mach);
		//PktQ.pop();
		SendDown(pkt);
		break;
	case RECV:
		NS_LOG_WARN("RECV-SEND Collision!!!!!");
		pkt = 0;
		break;
	default:
		std::cout << "SendPkt node:" << m_device->GetNode()
				<< " send data too fast" << std::endl;
		pkt = 0;
	}
	return;
}

bool AquaSimPhyFama::TxProcess(Ptr<Packet> pkt) {
	//std::cout<<"AquaSimPhyFama::TxProcess"<<std::endl;
	//callback to higher level, should be implemented differently
	Ptr<Packet> packet = pkt->Copy();
	Ptr<Packet> tpack = pkt->Copy();
	AquaSimHeader asHeader;
	AquaSimPtTag ptag;
	PhyFamaHeader FamaH;
	MacHeader mach;
	pkt->RemoveHeader(asHeader);
	pkt->RemovePacketTag(ptag);

	if (ptag.GetPacketType() == AquaSimPtTag::PT_HalfReality_CTS) {
//		std::cout << "Node " << m_device->GetAddress()
//				<< " MAC layer receive CTS packet from Route layer"
//				<< std::endl;
		mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
		mach.SetDA(asHeader.GetNextHop());
		pkt->AddPacketTag(ptag);
		pkt->AddHeader(asHeader);
		pkt->AddHeader(mach);
		SendPkt(pkt);
	} else if (ptag.GetPacketType() == AquaSimPtTag::PT_HalfReality_RTS) {
//		std::cout << "Node " << m_device->GetAddress()
//				<< " MAC layer receive RTS packet from Route layer"
//				<< std::endl;
		mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
		mach.SetDA(asHeader.GetNextHop());
		pkt->AddPacketTag(ptag);
		pkt->AddHeader(asHeader);
		pkt->AddHeader(mach);
		SendPkt(pkt);
	} else if (ptag.GetPacketType() == AquaSimPtTag::PT_HalfReality_ACK) {
//		std::cout << "Node " << m_device->GetAddress()
//				<< " MAC layer receive ACK packet from Route layer"
//				<< std::endl;
		mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
		mach.SetDA(asHeader.GetNextHop());
		pkt->AddPacketTag(ptag);
		pkt->AddHeader(asHeader);
		pkt->AddHeader(mach);
		SendPkt(pkt);
	} else { //send data packet
		if (asHeader.GetSAddr()
				== AquaSimAddress::ConvertFrom(m_device->GetAddress())) {

			Ptr<Packet> temppkt = packet->Copy();
//		 			   std::cout<<"data before push";
//			        		  temppkt->Print(std::cout);
//			        		  std::cout<<"\n";

			AquaSimHeader ash;
			TransportHeader tsh;
			temppkt->RemoveHeader(ash);
			temppkt->PeekHeader(tsh);
			temppkt->AddHeader(ash);
			Ptr<Packet> apkt = Create<Packet>();
			AquaSimHeader asHeader;
			TransportHeader tsHeader;
			//AquaSimPtTag ptag;
			asHeader.SetSAddr(
					AquaSimAddress::ConvertFrom(m_device->GetAddress()));
			// std::cout<<"####SourceAddress####"<<asHeader.GetSAddr()<<"\n";
			asHeader.SetDAddr(
					AquaSimAddress::ConvertFrom(m_device->GetAddress()));
			// std::cout<<"####DestAddress####"<<asHeader.GetDAddr()<<"\n";
			asHeader.SetSize(asHeader.GetSize() + tsHeader.GetSerializedSize());
			//asHeader.SetTxTime(m_device->GetMac()->GetTxTime(asHeader.GetSerializedSize() + tsHeader.GetSerializedSize()));
			asHeader.SetErrorFlag(false);
			asHeader.SetDirection(AquaSimHeader::UP);

			/*	// previous code -- spinach 
			tsHeader.SetPType(TransportHeader::ACK);
			tsHeader.SetSourceAddress(
					AquaSimAddress::ConvertFrom(m_device->GetAddress()));
			tsHeader.SetDestAddress(
					AquaSimAddress::ConvertFrom(m_device->GetAddress()));
			tsHeader.SetAckNumber(tsh.GetSequenceNumber() + 1);
			*/
			apkt->AddHeader(tsHeader);
			apkt->AddHeader(asHeader);
			SendUp(apkt);
		}
//		std::cout << "Node " << m_device->GetAddress()
//				<< " MAC layer receive data packet from Route layer"
//				<< std::endl;
		AquaSimPtTag ptag1;
		MacHeader mach1;
		AquaSimHeader asHeader1;
		TransportHeader tsh;
		packet->RemoveHeader(asHeader1);
		packet->RemoveHeader(tsh);
		packet->RemovePacketTag(ptag1);
		mach1.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
		mach1.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
		mach1.SetDA(asHeader1.GetNextHop());
		bool needRTS = false;
		if (asHeader1.GetSAddr()
				== AquaSimAddress::ConvertFrom(m_device->GetAddress())) {
			needRTS = true;
		}
		packet->AddPacketTag(ptag1);
		packet->AddHeader(tsh);
		packet->AddHeader(asHeader1);
		packet->AddHeader(mach1);
		//m_txBuffer.AddNewPacket(packet);
		if (asHeader.GetSAddr()
				== AquaSimAddress::ConvertFrom(m_device->GetAddress())) {
//			std::cout << "data before push ";
//

			PktQ.push(tpack);
//			tpack->Print(std::cout);
			std::cout << "\n";
		}

		if (CarrierDected()) {
			Simulator::Schedule(Seconds(m_rand->GetValue(0.0, m_NDPeriod)),
					&AquaSimPhyFama::TxProcess, this, packet);
		} else {
			if (needRTS) {
				if (AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() //Ghost Node don't need to send RTS
				== 1) {

					SendPkt(packet);
				} else {
					std::cout << "Node "
							<< AquaSimAddress::ConvertFrom(
									m_device->GetAddress()).GetAsInt()
							<< " Send a RTS Packet" << std::endl;
					SendRTS(2 * m_maxPropDelay);
					Simulator::Schedule(Seconds(10),
																&AquaSimPhyFama::ReSendRTS, this);

				}
			} else {
//				std::cout<<"SendPkt before"<<std::endl;
//		      packet->Print(std::cout);
//		        std::cout<<std::endl;
				SendPkt(packet);
			}
		}
	}
	return true;
}

bool AquaSimPhyFama::RecvProcess(Ptr<Packet> pkt) {
//	std::cout << "Node "
//			<< AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()
//			<< " receive packet from Phy layer" << std::endl;
//	pkt->Print(std::cout);
//	std::cout<<std::endl;
	MacHeader mach;
	AquaSimHeader asHeader;
	AquaSimPtTag ptag;
	pkt->RemoveHeader(mach);
	pkt->RemoveHeader(asHeader);
	pkt->RemovePacketTag(ptag);

	pkt->AddPacketTag(ptag);
	pkt->AddHeader(asHeader);

	AquaSimAddress dst = asHeader.GetDAddr();    	  //check--if it is null
	AquaSimAddress sa = mach.GetSA();
	AquaSimAddress ds = mach.GetDA();
	if (ds != m_device->GetAddress())
		return false;

	if (asHeader.GetErrorFlag()) {
		pkt = 0;
		std::cout << "GetErrorFlag!" << std::endl;
		return false;
	}

//receive from CsmaNetDevice

	if (ptag.GetPacketType() == AquaSimPtTag::PT_HalfReality_RTS)
		SendUp(pkt);

	else if (ptag.GetPacketType() == AquaSimPtTag::PT_HalfReality_CTS)
		if (dst == m_device->GetAddress()) {
			std::cout << "Node "
					<< AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()
					<< " receive CTS" << std::endl;
			if (FamaStatus == WAIT_CTS) {
//				std::cout << "I am SendDataPkt" << std::endl;
				//FamaStatus=PASSIVE;
				FamaStatus = PASSIVE;
				SendDataPkt();
			}
		} else {
			SendUp(pkt);
		}
	else if (ptag.GetPacketType() == AquaSimPtTag::PT_HalfReality_ACK) {
		SendUp(pkt);
	} else {
		if (dst == m_device->GetAddress()) {
			AquaSimHeader asHeader;
			AquaSimPtTag ptag;
			MacHeader mach;
			pkt->RemoveHeader(asHeader);
			pkt->RemovePacketTag(ptag);

			pkt->AddPacketTag(ptag);
			pkt->AddHeader(asHeader);
			SendUp(pkt);
			if (AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() != 1)
				ProcessData(sa);
			return true;
		} else {
			SendUp(pkt);
		}
	}
	pkt = 0;
	return true;
}

void AquaSimPhyFama::SendDataPkt() {
	Time StartTime = Simulator::Now();
	Ptr<Packet> tmp;

	if (PktQ.size() != 0) {
		tmp = PktQ.front()->Copy();
		AquaSimPtTag ptag1;
		MacHeader mach1;
		AquaSimHeader asHeader1;
		TransportHeader tsh;
		tmp->RemoveHeader(asHeader1);
		tmp->RemoveHeader(tsh);
		tmp->RemovePacketTag(ptag1);
		mach1.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
		mach1.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
		mach1.SetDA(asHeader1.GetNextHop());
		tmp->AddPacketTag(ptag1);
		tmp->AddHeader(tsh);
		tmp->AddHeader(asHeader1);
		tmp->AddHeader(mach1);
		PktQ.pop();
		if(PktQ.size() != 0){
			std::cout<<"packet send  RTS"<<std::endl;
			FamaStatus = WAIT_CTS;
			Simulator::Schedule(Seconds(5),
								&AquaSimPhyFama::SendRTS, this, 2 * m_maxPropDelay);
			Simulator::Schedule(Seconds(180),
											&AquaSimPhyFama::ReSendRTS, this);
		}
		SendPkt(tmp);


	}
//FamaStatus = WAIT_DATA_FINISH;
}

Ptr<Packet> AquaSimPhyFama::MakeRTS(AquaSimAddress Recver) {
	std::string content = "1";
	uint16_t Device_number =
			AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt();
	if (Device_number < 10) {
		content = content + "0" + std::to_string(Device_number);
	} else {
		content = content + std::to_string(Device_number);
	}
	std::ostringstream interest;
	interest << content << '\0';
	Ptr<Packet> pkt = Create<Packet>((uint8_t*) interest.str().c_str(),
			interest.str().length());
	AquaSimHeader asHeader;
	AquaSimPtTag ptag;
	MacHeader mach;
	asHeader.SetTxTime(m_RTSTxTime);
	asHeader.SetErrorFlag(false);
	asHeader.SetDirection(AquaSimHeader::DOWN);
	mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	mach.SetDA(Recver);
	ptag.SetPacketType(AquaSimPtTag::PT_HalfReality_RTS);
	asHeader.SetNextHop(Recver);
	asHeader.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	asHeader.SetDAddr(
			AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(01)));
//	std::cout << "RTS SA :" << asHeader.GetSAddr() << " DA:"
//			<< asHeader.GetDAddr() << std::endl;
	pkt->AddPacketTag(ptag);
	pkt->AddHeader(asHeader);
	pkt->AddHeader(mach);
	return pkt;
}

void AquaSimPhyFama::ReSendRTS() {
	if(FamaStatus == WAIT_CTS && PktQ.size() != 0)
	{
		Simulator::Schedule(Seconds(65.0),
														&AquaSimPhyFama::ReSendRTS, this);
	   MacHeader mach;
	   AquaSimHeader asHeader;
	   Ptr<Packet> pkt = PktQ.front();
		pkt->PeekHeader(asHeader);
		SendPkt(MakeRTS(asHeader.GetNextHop()));
	}
}

void AquaSimPhyFama::SendRTS(Time DeltaTime) {
	if(PktQ.size() != 0){
		MacHeader mach;
		AquaSimHeader asHeader;
		Ptr<Packet> pkt = PktQ.front();
	//pkt->RemoveHeader(mach);
		pkt->PeekHeader(asHeader);
	//pkt->AddHeader(mach);
		SendPkt(MakeRTS(asHeader.GetNextHop()));
		FamaStatus = WAIT_CTS;
		m_waitCTSTimer.SetDelay(DeltaTime);
	}
}

void AquaSimPhyFama::ProcessData(AquaSimAddress sa) {
	SendPkt(MakeACKDATA(sa));
}

Ptr<Packet> AquaSimPhyFama::MakeACKDATA(AquaSimAddress DATA_Sender) {
	std::string content = "4";
	uint16_t Device_number =
			AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt();
	if (Device_number < 10) {
		content = content + "0" + std::to_string(Device_number);
	} else {
		content = content + std::to_string(Device_number);
	}
	std::ostringstream interest;
	interest << content << '\0';
	Ptr<Packet> pkt = Create<Packet>((uint8_t*) interest.str().c_str(),
			interest.str().length());
	MacHeader mach;
	AquaSimHeader asHeader;
	AquaSimPtTag ptag;
	mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
	asHeader.SetTxTime(m_CTSTxTime);
	asHeader.SetErrorFlag(false);
	asHeader.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_HalfReality_ACK);
	asHeader.SetNextHop(DATA_Sender);
	asHeader.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	asHeader.SetDAddr(
			AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(01)));
	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	mach.SetDA(DATA_Sender);
	pkt->AddPacketTag(ptag);
	pkt->AddHeader(asHeader);
	pkt->AddHeader(mach);
	return pkt;
}

bool AquaSimPhyFama::CarrierDected() {
	if (m_device->GetTransmissionStatus() == RECV
			|| m_device->GetTransmissionStatus() == SEND) {
		return true;
	}
	return false;
}
} // namespace ns3
