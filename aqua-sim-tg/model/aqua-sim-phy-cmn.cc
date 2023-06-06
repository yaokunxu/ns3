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

#include <string>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"

#include "aqua-sim-header.h"
#include "aqua-sim-trailer.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-energy-model.h"
#include "aqua-sim-phy-cmn.h"

//Aqua Sim Phy Cmn

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimPhyCmn");
NS_OBJECT_ENSURE_REGISTERED(AquaSimPhyCmn);

AquaSimPhyCmn::AquaSimPhyCmn(void) :
		m_powerLevels(1, 18.451), /*0.660 indicates 1.6 W drained power for transmission*/
		m_sinrChecker(NULL) {
	NS_LOG_FUNCTION(this);

	m_updateEnergyTime = Simulator::Now().GetSeconds();
	m_preamble = 1.5;
	m_trigger = 0.45;
	//GetNetDevice()->SetTransmissionStatus(NIDLE);
	//SetPhyStatus(PHY_IDLE);
	//m_ant = NULL;
	m_channel.clear();
	//m_mac = NULL;

	m_ptLevel = 0;
	m_ptConsume = 5.0;
	m_prConsume = 2.0;
	m_PoweredOn = true;

	m_RXThresh = 0;
	m_CSThresh = 0;
	m_CPThresh = 10;
	m_pT = 18.451;
	m_EnergyTurnOn = 0;
	m_EnergyTurnOff = 0;
	m_lambda = 0.0;
	m_pIdle = 0.0;
	m_L = 0;
	m_K = 2.0;
	m_freq = 25;
	m_transRange = 1000;

	incPktCounter = 0;	//debugging purposes only
	outPktCounter = 0;
	pktRecvCounter = 0;
	initmodulations();


	Simulator::Schedule(Seconds(1), &AquaSimPhyCmn::UpdateIdleEnergy, this); //start energy drain
}

void AquaSimPhyCmn::initmodulations(){
	Ptr<AquaSimModulation> modulation = CreateObject<AquaSimModulation>(38, 608);
	addModulation(modulation, 1);

	modulation = CreateObject<AquaSimModulation>(80, 1280);
	addModulation(modulation, 2);

	modulation = CreateObject<AquaSimModulation>(122, 1952);
	addModulation(modulation, 3);

	modulation = CreateObject<AquaSimModulation>(164, 2624);
	addModulation(modulation, 4);

	modulation = CreateObject<AquaSimModulation>(164, 2624);
	addModulation(modulation, 5);

	modulation = CreateObject<AquaSimModulation>(164, 2624);
	addModulation(modulation, 21);
}

AquaSimPhyCmn::~AquaSimPhyCmn(void) {
	Dispose();
}

TypeId AquaSimPhyCmn::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::AquaSimPhyCmn").SetParent<AquaSimPhy>().AddConstructor<
					AquaSimPhyCmn>().AddAttribute("CPThresh",
					"Capture Threshold (db), default is 10.0 set as 10.",
					DoubleValue(10),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_CPThresh),
					MakeDoubleChecker<double>()).AddAttribute("CSThresh",
					"Carrier sense threshold (W), default is 1.559e-11 set as 0.",
					DoubleValue(0),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_CSThresh),
					MakeDoubleChecker<double>()).AddAttribute("transRange",
					"transRange(m), default is 1000.",
					DoubleValue(1000),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_transRange),
					MakeDoubleChecker<double>()).AddAttribute("RXThresh",
					"Receive power threshold (W), default is 3.652e-10 set as 0.",
					DoubleValue(0),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_RXThresh),
					MakeDoubleChecker<double>()).AddAttribute("PT",
					"Transmitted signal power (W).", DoubleValue(18.451),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_pT),
					MakeDoubleChecker<double>()).AddAttribute("Frequency",
					"The frequency, default is 25(khz).", DoubleValue(25),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_freq),
					MakeDoubleChecker<double>()).AddAttribute("L",
					"System loss default factor.", DoubleValue(0),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_L),
					MakeDoubleChecker<double>()).AddAttribute("K",
					"Energy spread factor, spherical spreading. Default is 2.0.",
					DoubleValue(2.0), MakeDoubleAccessor(&AquaSimPhyCmn::m_K),
					MakeDoubleChecker<double>()).AddAttribute("TurnOnEnergy",
					"Energy consumption for turning on the modem (J).",
					DoubleValue(0),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_EnergyTurnOn),
					MakeDoubleChecker<double>()).AddAttribute("TurnOffEnergy",
					"Energy consumption for turning off the modem (J).",
					DoubleValue(0),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_EnergyTurnOff),
					MakeDoubleChecker<double>()).AddAttribute("Preamble",
					"Duration of preamble.", DoubleValue(0),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_preamble),
					MakeDoubleChecker<double>()).AddAttribute("Trigger",
					"Duration of trigger.", DoubleValue(0),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_trigger),
					MakeDoubleChecker<double>()).AddAttribute("PTLevel",
					"Level of transmitted signal power.", UintegerValue(0),
					MakeUintegerAccessor(&AquaSimPhyCmn::m_ptLevel),
					MakeUintegerChecker<uint32_t>()).AddAttribute("PTConsume",
					"Power consumption for transmission (W). Default is 0.660 (1.6W).",
					DoubleValue(1.3),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_ptConsume),
					MakeDoubleChecker<double>()).AddAttribute("PRConsume",
					"Power consumption for reception (W). Default is 0.395 (1.2W).",
					DoubleValue(0.395),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_prConsume),
					MakeDoubleChecker<double>()).AddAttribute("PIdle",
					"Idle power consumption (W). Default is 0.0 (0W).",
					DoubleValue(0.0),
					MakeDoubleAccessor(&AquaSimPhyCmn::m_pIdle),
					MakeDoubleChecker<double>()).AddAttribute("SignalCache",
					"Signal cache attached to this node.", PointerValue(),
					MakePointerAccessor(&AquaSimPhyCmn::m_sC),
					MakePointerChecker<AquaSimSignalCache>()).AddTraceSource(
					"Rx", "A packet was receieved.",
					MakeTraceSourceAccessor(&AquaSimPhyCmn::m_rxLogger),
					"ns3::AquaSimPhy::TracedCallback").AddTraceSource("Tx",
					"A packet was transmitted.",
					MakeTraceSourceAccessor(&AquaSimPhyCmn::m_txLogger),
					"ns3::AquaSimPhy::TracedCallback");
	return tid;
}

void AquaSimPhyCmn::SetTxPower(double ptConsume) {
	m_ptConsume = ptConsume;
	//NS_ASSERT(EM() != NULL);
	EM()->SetTxPower(m_ptConsume);
}

void AquaSimPhyCmn::SetRxPower(double prConsume) {
	m_prConsume = prConsume;
	//NS_ASSERT(EM() != NULL);
	EM()->SetRxPower(m_prConsume);
}

void AquaSimPhyCmn::SetIdlePower(double pIdle) {
	m_pIdle = pIdle;
	//NS_ASSERT(EM() != NULL);
	EM()->SetIdlePower(m_pIdle);
}


void AquaSimPhyCmn::SetSinrChecker(Ptr<AquaSimSinrChecker> sinrChecker) {
	m_sinrChecker = sinrChecker;
}

void AquaSimPhyCmn::SetSignalCache(Ptr<AquaSimSignalCache> sC) {
	m_sC = sC;
	AttachPhyToSignalCache(m_sC, this);
}

Ptr<AquaSimSignalCache> AquaSimPhyCmn::GetSignalCache() {
	return m_sC;
}

void AquaSimPhyCmn::addModulation(Ptr<AquaSimModulation> modulation,
		int modulationNum) {

	if (modulation == NULL)
		NS_LOG_WARN("PHY-AddModulation NULL value for modulation " 
					<< modulation << " or name " << modulationNum);
	else if (modulations.count(modulationNum) > 0)
		NS_LOG_WARN("PHY-Duplicate modulations");
	else {
		if (modulations.size() == 0) {
			this->modulationNum = modulationNum;
		}
		modulations[modulationNum] = modulation;
	}
}

/**
 * update energy for transmitting for duration of P_t
 */
void AquaSimPhyCmn::UpdateTxEnergy(Time txTime, double pT, double pIdle) {
	NS_LOG_FUNCTION(this << "Currently not implemented");
	double startTime = Simulator::Now().GetSeconds(), endTime =
			Simulator::Now().GetSeconds() + txTime.GetSeconds();

	if (NULL != EM()) {
		if (startTime >= m_updateEnergyTime) {
			EM()->DecrIdleEnergy(startTime - m_updateEnergyTime, pIdle);
			m_updateEnergyTime = startTime;
		}
		EM()->DecrTxEnergy(txTime.GetSeconds(), pT);
		m_updateEnergyTime = endTime;
	} else
		NS_LOG_FUNCTION(this << " No EnergyModel set.");
}

void AquaSimPhyCmn::UpdateRxEnergy(Time txTime, bool errorFlag) {
	NS_LOG_FUNCTION(txTime);

	double startTime = Simulator::Now().GetSeconds();
	double endTime = startTime + txTime.GetSeconds();

	if (EM() == NULL) {
		NS_LOG_FUNCTION(this << " No EnergyModel set.");
		return;
	}

	if (startTime > m_updateEnergyTime) {
		EM()->DecrIdleEnergy(startTime - m_updateEnergyTime, m_pIdle);
		EM()->DecrRcvEnergy(txTime.GetSeconds(), m_prConsume);
		m_updateEnergyTime = endTime;
	} else {
		/* In this case, this device is receiving some other packet*/
		if (endTime > m_updateEnergyTime && errorFlag) {
			EM()->DecrRcvEnergy(endTime - m_updateEnergyTime, m_prConsume);
			m_updateEnergyTime = endTime;
		}
	}

	if (EM()->GetEnergy() <= 0) {
		EM()->SetEnergy(-1);
		//GetNetDevice()->LogEnergy(0);
		//NS_LOG_INFO("AquaSimPhyCmn::UpdateRxEnergy: -t " << Simulator::Now().GetSeconds() <<
		//" -n " << GetNetDevice()->GetAddress() << " -e 0");
	}
}

void AquaSimPhyCmn::UpdateIdleEnergy() {
	if (!m_PoweredOn || EM() == NULL)
		return;

	if (Simulator::Now().GetSeconds() > m_updateEnergyTime && m_PoweredOn) {
		EM()->DecrIdleEnergy(Simulator::Now().GetSeconds() - m_updateEnergyTime,
				m_pIdle);
		m_updateEnergyTime = Simulator::Now().GetSeconds();
	}

	// log device energy
	if (EM()->GetEnergy() > 0) {
		//GetNetDevice()->LogEnergy(1);
		//NS_LOG_INFO("AquaSimPhyCmn::UpdateRxEnergy: -t " << Simulator::Now().GetSeconds() <<
		//" -n " << GetNetDevice()->GetAddress() << " -e " << EM()->GetEnergy());
	} else {
		//GetNetDevice()->LogEnergy(0);
		//NS_LOG_INFO("AquaSimPhyCmn::UpdateRxEnergy: -t " << Simulator::Now().GetSeconds() <<
		//" -n " << GetNetDevice()->GetAddress() << " -e 0");
	}

	Simulator::Schedule(Seconds(1), &AquaSimPhyCmn::UpdateIdleEnergy, this);
}

bool AquaSimPhyCmn::Decodable(double noise, double ps) {
	double epsilon = 1e-6;	//accuracy for float comparison

	if (ps < m_RXThresh) //signal is too weak
		return false;

	if (fabs(noise) < epsilon) {
		//avoid being divided by 0 when calculating SINR
		return true;
	}

	return m_sinrChecker->Decodable(ps / noise);
}

/**
 * stamp the packet with information required by channel
 * different channel model may require different information
 * overload this method if needed
 */
Ptr<Packet> AquaSimPhyCmn::StampTxInfo(Ptr<Packet> p) {
	AquaSimPacketStamp pstamp;
	pstamp.SetPt(m_pT);
	pstamp.SetPr(m_lambda);
	pstamp.SetFreq(m_freq);
	pstamp.SetTxRange(m_transRange);
	p->AddHeader(pstamp);
	return p;
}

/**
 * we will cache the incoming packet in phy layer
 * and send it to MAC layer after receiving the entire one
 */

bool AquaSimPhyCmn::txProcess(Ptr<Packet> p){
	m_device->SetTransmissionStatus(SEND);
	PktTransmit(p);
	return true;

}
bool AquaSimPhyCmn::recvProcess(Ptr<Packet> p){
	p = PrevalidateIncomingPkt(p);
	if (p != NULL) {
		m_sC->AddNewPacket(p);
	}
	return true;
}

bool AquaSimPhyCmn::MatchFreq(double freq) {
	double epsilon = 1e-6;	//accuracy for float comparison
	return std::fabs(freq - m_freq) < epsilon;
}

/**
 * This function pre-validate a incoming packet,
 * it checks if this reception fails because of a hardware issue
 * meanwhile, we update the energy consumption on this node.
 *
 * @param p the received packet
 * @return  NULL if this packet cannot be received even if it cannot be decoded
 * 			p would be freed and set to NULL in this case
 * 			otherwise, return p
 */
Ptr<Packet> AquaSimPhyCmn::PrevalidateIncomingPkt(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this << p);
	AquaSimPacketStamp pstamp;
	MacHeader mach;
	p->RemoveHeader(pstamp);
	p->PeekHeader(mach);
	pstamp.SetPacketStatus(AquaSimPacketStamp::RECEPTION);
	

	if (GetNetDevice()->FailureStatus()) {
		NS_LOG_WARN(
				"AquaSimPhyCmn: nodeId=" << GetNetDevice()->GetNode()->GetId() << " fails!\n");
		p = 0;
		return NULL;
	}

	if (!MatchFreq(pstamp.GetFreq())) {
		NS_LOG_WARN(
				"AquaSimPhyCmn: Cannot match freq(" << pstamp.GetFreq() << ") on node(" << GetNetDevice()->GetNode() << ")");
		p = 0;
		return NULL;
	}

	/**
	 * any packet error set here result from that a packet
	 * cannot be detected by the modem, so modem's status doesn't receive
	 */

	if ((EM() && EM()->GetEnergy() <= 0) || 
			GetNetDevice()->GetTransmissionStatus() == SLEEP
			|| pstamp.GetPr() < m_RXThresh) {

		NS_LOG_DEBUG("PHY-EM/DEV/PR problem");
		pstamp.SetPacketStatus(AquaSimPacketStamp::COLLISION);

	}else if (GetNetDevice()->GetTransmissionStatus() == SEND) {
		NS_LOG_DEBUG("PHY-TX/RX collision!");
		pstamp.SetPacketStatus(AquaSimPacketStamp::COLLISION);

	} else if(GetNetDevice()->GetTransmissionStatus() == RECV){
		Time temp = Simulator::Now()+ calcTxTime(p);
		if(temp > this->statusEventTime){
			statusEventId.Cancel();
			statusEventId = Simulator::Schedule(calcTxTime(p),
							&AquaSimNetDevice::SetTransmissionStatus, GetNetDevice(),
							NIDLE);
		}
	}
	else{
		this->statusEventTime = Simulator::Now()+ calcTxTime(p);
		GetNetDevice()->SetTransmissionStatus(RECV);
		statusEventId = Simulator::Schedule(calcTxTime(p),
				&AquaSimNetDevice::SetTransmissionStatus, GetNetDevice(),
				NIDLE);
	}

	Time txTime = calcTxTime(p);

	UpdateRxEnergy(txTime, false);

	if (mach.GetDemuxPType() == MacHeader::UWPTYPE_LOC) {
		GetNetDevice()->GetMacLoc()->SetPr(pstamp.GetPr());
	}
	p->AddHeader(pstamp); //no longer needed.
	return p;
}

/**
 * pass packet p to channel
 */
bool AquaSimPhyCmn::PktTransmit(Ptr<Packet> p, int channelId) {
	NS_LOG_FUNCTION(this << p);
	Time txTime = this->calcTxTime(p);


	if (GetNetDevice()->FailureStatus()) {
		NS_LOG_WARN(
				"AquaSimPhyCmn nodeId=" << GetNetDevice()->GetNode()->GetId() << " fails!\n");
		p = 0;
		return false;
	}

	if (GetNetDevice()->GetTransmissionStatus() == SLEEP
			|| (NULL != EM() && EM()->GetEnergy() <= 0)) {
		NS_LOG_DEBUG("Unable to reach phy layer (sleep/disable)");
		std::cout<<"Node:"<<GetNetDevice()->GetAddress()<<" die!!!!"<<"\n";
		p = 0;
		return false;
	}

	switch (GetNetDevice()->GetTransmissionStatus()) {
	case SEND:
		UpdateTxEnergy(txTime, m_ptConsume, m_pIdle);
		break;
	default:
		NS_LOG_WARN("AquaSimPhyCmn: wrong status (dropping pkt)");
		return false;
	}

	StampTxInfo(p);

	Simulator::Schedule(txTime, &AquaSimNetDevice::SetTransmissionStatus,
			GetNetDevice(), NIDLE);

	NotifyTx(p);
	m_txLogger(p, m_sC->GetNoise());
	
	return m_channel.at(channelId)->Recv(p, this);

}

/**
 * send packet to upper layer, supposed to be MAC layer,
 * but actually go to any specificed module.
 */
void AquaSimPhyCmn::SendPktUp(Ptr<Packet> p) {

	NS_LOG_FUNCTION(this);
	MacHeader mach;
	p->PeekHeader(mach);

	NotifyRx(p);
	m_rxLogger(p, m_sC->GetNoise());

	//This can be shifted to within the switch to target specific packet types.
	if (GetNetDevice()->IsAttacker()) {
		GetNetDevice()->GetAttackModel()->Recv(p);
		return;
	}

	switch (mach.GetDemuxPType()) {
	case MacHeader::UWPTYPE_OTHER:
		if (!GetMac()->RecvProcess(p))
			NS_LOG_DEBUG(this << "Mac Recv error");
		break;
	case MacHeader::UWPTYPE_LOC:
		std::cout << GetNetDevice()->GetNode() << " Phy-cmn Send up"
				<< std::endl;
		GetNetDevice()->GetMacLoc()->Recv(p);
		break;
	case MacHeader::UWPTYPE_SYNC:
		GetNetDevice()->GetMacSync()->RecvSync(p);
		break;
	case MacHeader::UWPTYPE_SYNC_BEACON:
		GetNetDevice()->GetMacSync()->RecvSyncBeacon(p);
		break;
	case MacHeader::UWPTYPE_NDN:
		GetNetDevice()->GetNamedData()->Recv(p);
		break;
	default:
		NS_LOG_DEBUG("SendPKtUp: Something went wrong.");

	}
}

/**
 * process packet with signal cache, looking for collisions and SINR
 */
void AquaSimPhyCmn::SignalCacheCallback(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this << p);

	pktRecvCounter++; //debugging...
	SendPktUp(p);

}

void AquaSimPhyCmn::PowerOn() {
	NS_LOG_FUNCTION(this);

	if (GetNetDevice()->GetTransmissionStatus() == DISABLE)
		NS_LOG_FUNCTION(
				this << " Node " << GetNetDevice()->GetNode() << " is disabled.");
	else {
		m_PoweredOn = true;
		GetNetDevice()->SetTransmissionStatus(NIDLE);
		//SetPhyStatus(PHY_IDLE);
		if (EM() != NULL) {
			//minus the energy consumed by power on
			EM()->SetEnergy(std::max(0.0, EM()->GetEnergy() - m_EnergyTurnOn));
			m_updateEnergyTime = std::max(Simulator::Now().GetSeconds(),
					m_updateEnergyTime);
		}
	}
}

void AquaSimPhyCmn::PowerOff() {
	NS_LOG_FUNCTION(this);

	if (GetNetDevice()->GetTransmissionStatus() == DISABLE)
		NS_LOG_FUNCTION(
				this << " Node " << GetNetDevice()->GetNode() << " is disabled.");
	else {
		m_PoweredOn = false;
		GetNetDevice()->SetTransmissionStatus(SLEEP);
		//SetPhyStatus(PHY_SLEEP);
		if (EM() == NULL)
			return;

		//minus the energy consumed by power off
		EM()->SetEnergy(std::max(0.0, EM()->GetEnergy() - m_EnergyTurnOff));

		if (Simulator::Now().GetSeconds() > m_updateEnergyTime) {
			EM()->DecrIdleEnergy(
					Simulator::Now().GetSeconds() - m_updateEnergyTime,
					m_pIdle);
			m_updateEnergyTime = Simulator::Now().GetSeconds();
		}
	}
}

bool AquaSimPhyCmn::IsPoweredOn() {
	return m_PoweredOn;
}

void AquaSimPhyCmn::Dump(void) const {
	NS_LOG_DEBUG(
			"AquaSimPhyCmn Dump: Channel_default(" << m_channel.at(0) << ") " << "Pt(" << m_pT << ") " <<
			//"Gt(" << m_ant->GetTxGain(0, 0, 0, m_lambda) << ") " <<
			"lambda(" << m_lambda << ") " << "L(" << m_L << ")\n");
}

void AquaSimPhyCmn::StatusShift(double txTime) {
	double endTime = Simulator::Now().GetSeconds() + txTime;
	/*  The receiver is receiving a packet when the
	 transmitter begins to transmit a data.
	 We assume the half-duplex mode, the transmitter
	 stops the receiving process and begins the sending
	 process.
	 */
	if (m_updateEnergyTime < endTime) {
		double overlapTime = m_updateEnergyTime - Simulator::Now().GetSeconds();
		double actualTxTime = endTime - m_updateEnergyTime;
		EM()->DecrEnergy(overlapTime, m_ptConsume - m_prConsume);
		EM()->DecrTxEnergy(actualTxTime, m_ptConsume);
		m_updateEnergyTime = endTime;
	} else {
		double overlapTime = txTime;
		EM()->DecrEnergy(overlapTime, m_ptConsume - m_prConsume);
	}
}

/**
 * @para ModName the name of selected modulation scheme
 *	@return     NULL if ModName cannot be found in m_modulations
 *	@return		a pointer to the corresponding AquaSimModulation obj
 */
Ptr<AquaSimModulation> AquaSimPhyCmn::Modulation(int modulationNum) {
	std::map<int, Ptr<AquaSimModulation> >::iterator pos;
	pos = modulations.find(modulationNum);
	if (modulations.end() != pos) {
		return pos->second;
	} else {
			NS_LOG_WARN("PHY-Failed to locate modulation!"
						<< "modulationNum:" << modulationNum);
			return NULL;
	}
}

void AquaSimPhyCmn::EnergyDeplete() {
	//NS_LOG_FUNCTION(this);
	//NS_LOG_DEBUG("Energy is depleted on node " << GetNetDevice()->GetNode());

	//TODO fix energy model and then allow this   SetPhyStatus(PHY_DISABLE);
}

/**
 * calculate transmission time of a packet of size pktsize
 * we consider the preamble
 */
Time AquaSimPhyCmn::calcTxTime(uint32_t pktSize, int modulationNum) {
	NS_LOG_INFO("PHY-using modulation" << modulationNum);
	//double time = double(pktSize)/667 * 8 + 0.87;
	double time = modulations[modulationNum]->TxTime(pktSize);
	return Seconds(time);
}


Time AquaSimPhyCmn::calcTxTime(Ptr<Packet> pkt){
	int pktSize = pkt->GetSize();
	AquaSimTrailer ast;
	pkt->PeekTrailer(ast);
	return calcTxTime(pktSize - ast.GetSerializedSize(), ast.GetModeId());
}


Time AquaSimPhyCmn::calcTxTime(uint32_t pktSize){
	return calcTxTime(pktSize, modulationNum);
}



double AquaSimPhyCmn::CalcPktSize(double txTime, int modulationNum) {
	//return Modulation(modName)->PktSize(txTime - Preamble());
	return 100;
}

int AquaSimPhyCmn::PktRecvCount() {
	return pktRecvCounter;
}

void AquaSimPhyCmn::DoDispose() {
	NS_LOG_FUNCTION(this);
	m_sC->Dispose();
	m_sC = 0;
	m_sinrChecker = 0;
	for (std::map<int, Ptr<AquaSimModulation> >::iterator it =
			modulations.begin(); it != modulations.end(); ++it)
		it->second = 0;
	AquaSimPhy::DoDispose();
}

void AquaSimPhyCmn::SetTransRange(double range) {
	NS_LOG_FUNCTION(this);
	m_transRange = range;
}

double AquaSimPhyCmn::GetTransRange() {
	return m_transRange;
}

int64_t AquaSimPhyCmn::AssignStreams(int64_t stream) {
	NS_LOG_FUNCTION(this << stream);
	return 0;
}
