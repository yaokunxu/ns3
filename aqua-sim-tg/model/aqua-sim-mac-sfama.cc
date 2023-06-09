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

#include "aqua-sim-mac-sfama.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/integer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimSFama");
NS_OBJECT_ENSURE_REGISTERED(AquaSimSFama);

/*expire functions and handle functions*/
void AquaSimSFama_Wait_Send_Timer::expire() {
	m_mac->WaitSendTimerProcess(m_pkt);
	m_pkt = NULL; /*reset pkt_*/
}

void AquaSimSFama_Wait_Reply_Timer::expire() {
	m_mac->WaitReplyTimerProcess();
}

void AquaSimSFama_Backoff_Timer::expire() {
	m_mac->BackoffTimerProcess();
}

void AquaSimSFama_DataSend_Timer::expire() {
	m_mac->DataSendTimerProcess();
}

AquaSimSFama::AquaSimSFama() :
		m_status(IDLE_WAIT), m_guardTime(0.05), m_slotLen(0), m_isInRound(
				false), m_isInBackoff(false), m_maxBackoffSlots(20), m_maxBurst(
				5), m_dataSendingInterval(0.0000001), m_waitSendTimer(this), m_waitReplyTimer(
				this), m_backoffTimer(this), m_datasendTimer(this) {
	NS_LOG_FUNCTION(this);
	m_rand = CreateObject<UniformRandomVariable>();
	m_slotNumHandler = 0;
	m_lastRxDataSlotsNum = 1;
	Simulator::Schedule(Seconds(0.05) /*callback delay*/,
			&AquaSimSFama::InitSlotLen, this);
}

AquaSimSFama::~AquaSimSFama() {
}

TypeId AquaSimSFama::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::AquaSimSFama").SetParent<AquaSimMac>().AddConstructor<
					AquaSimSFama>().AddAttribute("GuardTime",
					"The guard time in double. Default is 0.005",
					DoubleValue(5),
					MakeDoubleAccessor(&AquaSimSFama::m_guardTime),
					MakeDoubleChecker<double>()).AddAttribute("MaxBackoffSlots",
					"The maximum number of backoff slots. default is 4",
					IntegerValue(4),
					MakeIntegerAccessor(&AquaSimSFama::m_maxBackoffSlots),
					MakeIntegerChecker<int>()).AddAttribute("MaxBurst",
					"The maximum number of packets in the train. Default is 1",
					IntegerValue(1),
					MakeIntegerAccessor(&AquaSimSFama::m_maxBurst),
					MakeIntegerChecker<int>());
	return tid;
}

int64_t AquaSimSFama::AssignStreams(int64_t stream) {
	NS_LOG_FUNCTION(this << stream);
	m_rand->SetStream(stream);
	return 1;
}

bool AquaSimSFama::RecvProcess(Ptr<Packet> p) {
	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	p->RemoveHeader(mach);
	p->RemoveHeader(ash);
	p->RemoveHeader(SFAMAh);
	ash.SetDirection(AquaSimHeader::UP);

	p->AddHeader(mach);
	p->AddHeader(SFAMAh);
	p->AddHeader(ash);

	NS_LOG_DEBUG(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "in Time:" << Simulator::Now().GetSeconds()
			<< "recv"<< ",node " << mach.GetSA() << " send to " << mach.GetDA());

	switch (SFAMAh.GetPType()) {
	case SFamaHeader::SFAMA_RTS:
		NS_LOG_DEBUG("RTS");
		ProcessRTS(p);
		break;
	case SFamaHeader::SFAMA_CTS:
		NS_LOG_DEBUG("CTS");
		ProcessCTS(p);
		break;
	case SFamaHeader::SFAMA_DATA:
		NS_LOG_DEBUG("DATA");
		ProcessDATA(p);
		break;
	case SFamaHeader::SFAMA_ACK:
		NS_LOG_DEBUG("ACK");
		ProcessACK(p);
		break;
	default:
		/*unknown packet type. error happens*/
		NS_LOG_WARN("RecvProcess: unknown packet type.");
		break;
	}
	p = 0;
	return true;

}

bool AquaSimSFama::TxProcess(Ptr<Packet> p) {
	//callback to higher level, should be implemented differently
	//Scheduler::instance().schedule(&callback_handler, &callback_event, 0.0001 /*callback delay*/);
	FillDATA(p);
	m_CachedPktQ.push(p);
	if (m_CachedPktQ.size() == 1 && GetStatus() == IDLE_WAIT) {
		PrepareSendingDATA();
	}
	return true;
}
/**
 * Initialize Slot Length = max propagation delay + Transmission of CTS control packet + guard time
 *
 */

void AquaSimSFama::InitSlotLen() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	SFamaHeader SFAMA;
	double slotLen;
	//ns3::Time txTime = GetTxTime(SFAMA.GetSize(SFamaHeader::SFAMA_CTS));
	ns3::Time txTime = GetTxTime(30);
	double txTimeSec = txTime.ToDouble(Time::S);
	double prTimeSec = 4000.0 / 1500.0; //Device()->GetPhy()->GetTransRange()/ 1500.0;// Device()->GetPropSpeed();
	slotLen = m_guardTime + txTimeSec + prTimeSec;
	m_maxPrTimeSec = prTimeSec;

	m_slotLen = slotLen;

	m_rtsCtsAckNumSlotWait = 1;
	m_rtsCtsAckNumSlotWait = std::ceil(m_rtsCtsAckNumSlotWait); /// get the up int
}

/**
 * Calculate residual time to next Coming Slot
 */
double AquaSimSFama::GetTime2ComingSlot(double t) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << t);
	double numElapseSlot = t / m_slotLen;
	double res = m_slotLen * (std::ceil(numElapseSlot)) - t;
	if (res < 0)
		res = 0;

	return res;
}

/**
 * Make RTS control packet
 */
Ptr<Packet> AquaSimSFama::MakeRTS(AquaSimAddress recver, int slot_num) {
	NS_LOG_FUNCTION(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << recver.GetAsInt() << slot_num);

	Ptr<Packet> rts_pkt = Create<Packet>();
	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	AquaSimPtTag ptag;

	//ash.SetSize(SFAMAh.GetSize(SFamaHeader::SFAMA_RTS));
	//ash.SetTxTime(GetTxTime(ash.GetSize()) );
	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_SFAMA);

	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	mach.SetDA(recver);

	SFAMAh.SetPType(SFamaHeader::SFAMA_RTS);
	//SFAMAh->SA = index_;
	//SFAMAh->DA = recver;
	SFAMAh.SetSlotNum(slot_num);

	//rts_pkt->next_ = NULL;

	rts_pkt->AddHeader(mach);
	rts_pkt->AddHeader(SFAMAh);
	rts_pkt->AddHeader(ash);
	rts_pkt->AddPacketTag(ptag);
	return rts_pkt;
}

/**
 * Make CTS control packet
 */
Ptr<Packet> AquaSimSFama::MakeCTS(AquaSimAddress rts_sender, int slot_num) {
	NS_LOG_FUNCTION(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << rts_sender.GetAsInt() << slot_num);

	Ptr<Packet> cts_pkt = Create<Packet>();
	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	AquaSimPtTag ptag;

	ash.SetSize(SFAMAh.GetSize(SFamaHeader::SFAMA_CTS));
	ash.SetTxTime(GetTxTime(ash.GetSize()));
	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_SFAMA);

	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	mach.SetDA(rts_sender);

	SFAMAh.SetPType(SFamaHeader::SFAMA_CTS);
	//SFAMAh->SA = index_;
	//SFAMAh->DA = rts_sender;
	SFAMAh.SetSlotNum(slot_num);

	//cts_pkt->next_ = NULL;

	cts_pkt->AddHeader(mach);
	cts_pkt->AddHeader(SFAMAh);
	cts_pkt->AddHeader(ash);
	cts_pkt->AddPacketTag(ptag);
	return cts_pkt;
}

/**
 * Add header for data packet
 */
Ptr<Packet> AquaSimSFama::FillDATA(Ptr<Packet> data_pkt) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());

	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	data_pkt->RemoveHeader(ash);

	ash.SetSize(ash.GetSize() + SFAMAh.GetSize(SFamaHeader::SFAMA_DATA));
	auto bytes = ash.GetSize();
	auto txtime = GetTxTime(bytes);
	ash.SetTxTime(txtime);
	//NS_LOG_DEBUG(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; New Data Pkt to transmit = " << bytes << " bytes ; TxTime = " << txtime.ToDouble(Time::S));
	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);

	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	mach.SetDA(ash.GetNextHop());

	SFAMAh.SetPType(SFamaHeader::SFAMA_DATA);
	//SFAMAh->SA = index_;
	//SFAMAh->DA = ash.GetNextHop();

	data_pkt->AddHeader(mach);
	data_pkt->AddHeader(SFAMAh);
	data_pkt->AddHeader(ash);
	return data_pkt;
}

/**
 * Make ACK control packet
 */
Ptr<Packet> AquaSimSFama::MakeACK(AquaSimAddress data_sender) {
	NS_LOG_FUNCTION(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << data_sender.GetAsInt());

	Ptr<Packet> ack_pkt = Create<Packet>();
	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	AquaSimPtTag ptag;

	ash.SetSize(SFAMAh.GetSize(SFamaHeader::SFAMA_ACK));
	ash.SetTxTime(GetTxTime(ash.GetSize()));
	ash.SetErrorFlag(false);
	ash.SetDirection(AquaSimHeader::DOWN);
	ptag.SetPacketType(AquaSimPtTag::PT_SFAMA);

	mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	mach.SetDA(data_sender);

	SFAMAh.SetPType(SFamaHeader::SFAMA_ACK);
	//SFAMAh->SA = index_;
	//SFAMAh->DA = data_sender;

	ack_pkt->AddHeader(mach);
	ack_pkt->AddHeader(SFAMAh);
	ack_pkt->AddHeader(ash);
	ack_pkt->AddPacketTag(ptag);
	return ack_pkt;
}

/*process all kinds of packets*/

void AquaSimSFama::ProcessRTS(Ptr<Packet> rts_pkt) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());

	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	rts_pkt->RemoveHeader(ash);
	rts_pkt->RemoveHeader(SFAMAh);
	rts_pkt->PeekHeader(mach);
	rts_pkt->AddHeader(SFAMAh);
	rts_pkt->AddHeader(ash);

	double time2comingslot = GetTime2ComingSlot(
			Simulator::Now().ToDouble(Time::S));
	if (mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress())) {
		if ((GetStatus() == IDLE_WAIT || GetStatus() == WAIT_SEND_RTS
				|| GetStatus() == BACKOFF_FAIR)) {
			StopTimers();
			SetStatus(WAIT_SEND_CTS);
			//reply a cts
			m_waitSendTimer.m_pkt = MakeCTS(mach.GetSA(), SFAMAh.GetSlotNum());
			m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,
					&m_waitSendTimer);
			m_waitSendTimer.Schedule(Seconds(time2comingslot));
		} else {
			NS_LOG_WARN(
					AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; RTS received but unable to process (" << GetStatus() <<")");
		}
	} else {
		m_lastRxDataSlotsNum = SFAMAh.GetSlotNum();
		//do backoff
		double backoff_time =
				time2comingslot
						+ m_slotLen
								* (1 /*for cts*/+ SFAMAh.GetSlotNum() /*for data*/
										+ 1 /*for ack*/);

		StopTimers();
		NS_LOG_DEBUG(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; RTS detected; Backoff time: " << backoff_time << " s (slots: " << (backoff_time / m_slotLen) << "; Data slots: "<< SFAMAh.GetSlotNum()<<")");
		//m_isInBackoff = true;
		SetStatus(BACKOFF);

		m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,
				&m_backoffTimer);
		m_backoffTimer.Schedule(Seconds(backoff_time));
	}
}

void AquaSimSFama::ProcessCTS(Ptr<Packet> cts_pkt) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());

	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	cts_pkt->RemoveHeader(ash);
	cts_pkt->RemoveHeader(SFAMAh);
	cts_pkt->PeekHeader(mach);
	cts_pkt->AddHeader(SFAMAh);
	cts_pkt->AddHeader(ash);

	double time2comingslot = GetTime2ComingSlot(
			Simulator::Now().ToDouble(Time::S));

	if (mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress())
			&& GetStatus() == WAIT_RECV_CTS) {


		//send DATA
		StopTimers();
		SetStatus(WAIT_SEND_DATA);

		//send the packet
		m_waitSendTimer.m_pkt = NULL;
		m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,
				&m_waitSendTimer);
		m_waitSendTimer.Schedule(Seconds(time2comingslot));
	} else {
		m_lastRxDataSlotsNum = SFAMAh.GetSlotNum();
		//do backoff
		double backoff_time = time2comingslot
				+ m_slotLen * (SFAMAh.GetSlotNum() /*for data*/+ 1 /*for ack*/);

		StopTimers();
		NS_LOG_DEBUG(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; CTS detected; backoff time: " << backoff_time << " s (slots: " << (backoff_time / m_slotLen) << "; Data slots: "<< SFAMAh.GetSlotNum()<<")");
		//m_isInBackoff = true;
		SetStatus(BACKOFF);

		m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,
				&m_backoffTimer);
		m_backoffTimer.Schedule(Seconds(backoff_time));
	}

}

void AquaSimSFama::ProcessDATA(Ptr<Packet> data_pkt) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());

	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	data_pkt->RemoveHeader(ash);
	data_pkt->RemoveHeader(SFAMAh);
	data_pkt->PeekHeader(mach);
	data_pkt->AddHeader(SFAMAh);
	data_pkt->AddHeader(ash);

	if (mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress())
			&& GetStatus() == WAIT_RECV_DATA) {
		//send ACK
		StopTimers();
		SetStatus(WAIT_SEND_ACK);

		m_waitSendTimer.m_pkt = MakeACK(mach.GetSA());
		m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,
				&m_waitSendTimer);
		m_waitSendTimer.Schedule(
				Seconds(
						GetTime2ComingSlot(
								Simulator::Now().ToDouble(Time::S))));

		/*send packet to upper layer*/
		data_pkt->RemoveHeader(ash);
		data_pkt->RemoveHeader(SFAMAh);
		data_pkt->RemoveHeader(mach);
		ash.SetSize(ash.GetSize() - SFAMAh.GetSize(SFamaHeader::SFAMA_DATA));
		data_pkt->AddHeader(ash);
		SendUp(data_pkt->Copy()); /*the original one will be released*/
	} else {
		//do backoff
		double backoff_time = m_slotLen * (1)
				+ GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));

		StopTimers();
		NS_LOG_DEBUG(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; DATA detected; backoff time: " << backoff_time << " s (slots: " << (backoff_time / m_slotLen) << ")");
		//m_isInBackoff = true;
		SetStatus(BACKOFF);

		m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,
				&m_backoffTimer);
		m_backoffTimer.Schedule(Seconds(backoff_time));
	}
}

void AquaSimSFama::ProcessACK(Ptr<Packet> ack_pkt) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	ack_pkt->RemoveHeader(ash);
	ack_pkt->RemoveHeader(SFAMAh);
	ack_pkt->PeekHeader(mach);
	ack_pkt->AddHeader(SFAMAh);
	ack_pkt->AddHeader(ash);
	NS_LOG_DEBUG(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; ProcessACK: Status is " << GetStatus());

	if (mach.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress())
			&& GetStatus() == WAIT_RECV_ACK) {
		StopTimers();
		SetStatus(IDLE_WAIT);

		//release data packets have been sent successfully
		ReleaseSentPkts();

		//start to prepare for sending next DATA packet
		PrepareSendingDATA();
	}
	/*
	 * consider the multi-hop case, cannot stop timers here
	 else {
	 StopTimers();
	 SetStatus(IDLE_WAIT);

	 //release data packets have been sent successfully
	 PrepareSendingDATA();
	 }
	 */
}

void AquaSimSFama::StopTimers() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	m_waitSendTimer.Cancel();
	m_waitReplyTimer.Cancel();
	m_backoffTimer.Cancel();
}

void AquaSimSFama::ReleaseSentPkts() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	Ptr<Packet> tmp = Create<Packet>();

	while (!m_sendingPktQ.empty()) {
		tmp = m_sendingPktQ.front();
		m_sendingPktQ.pop();
		tmp = 0;
	}
}

void AquaSimSFama::PrepareSendingDATA() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	std::queue<Ptr<Packet> > tmpQ_;
	Ptr<Packet> tmp_pkt;
	AquaSimAddress recver_addr;
	int pkt_num = 1;

	if (m_sendingPktQ.empty() && m_CachedPktQ.empty()) {
		return;
	}

	if (!m_sendingPktQ.empty() && GetStatus() == IDLE_WAIT) {
		AquaSimHeader ash;
		SFamaHeader SFAMAh;
		MacHeader mach;
		m_sendingPktQ.front()->RemoveHeader(ash);
		m_sendingPktQ.front()->RemoveHeader(SFAMAh);
		m_sendingPktQ.front()->PeekHeader(mach);
		m_sendingPktQ.front()->AddHeader(SFAMAh);
		m_sendingPktQ.front()->AddHeader(ash);
		recver_addr = mach.GetDA();
	} else if (!m_CachedPktQ.empty() && GetStatus() == IDLE_WAIT) {
		AquaSimHeader ash_tmp;
		SFamaHeader SFAMAh_tmp;
		MacHeader mach_tmp;

		tmp_pkt = m_CachedPktQ.front();
		tmp_pkt->RemoveHeader(ash_tmp);
		tmp_pkt->RemoveHeader(SFAMAh_tmp);
		tmp_pkt->PeekHeader(mach_tmp);
		tmp_pkt->AddHeader(SFAMAh_tmp);
		tmp_pkt->AddHeader(ash_tmp);
		recver_addr = mach_tmp.GetDA();
		m_CachedPktQ.pop();
		m_sendingPktQ.push(tmp_pkt);
		pkt_num = 1;

		/*get at most m_maxBurst DATA packets with same receiver*/
		while ((pkt_num < m_maxBurst) && (!m_CachedPktQ.empty())) {
			tmp_pkt = m_CachedPktQ.front();
			m_CachedPktQ.pop();

			if (recver_addr == mach_tmp.GetDA()) {
				m_sendingPktQ.push(tmp_pkt);
				pkt_num++;
			} else {
				tmpQ_.push(tmp_pkt);
			}

		}

		//make sure the rest packets are stored in the original order
		while (!m_CachedPktQ.empty()) {
			tmpQ_.push(m_CachedPktQ.front());
			m_CachedPktQ.pop();
		}

		while (!tmpQ_.empty()) {
			m_CachedPktQ.push(tmpQ_.front());
			tmpQ_.pop();
		}
	}

	double requiredTimeToSendData = GetPktTrainTxTime();
	auto reqSlots = static_cast<int>(ceil(requiredTimeToSendData / m_slotLen));
	NS_LOG_DEBUG(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; Req slots = " << reqSlots);
	ScheduleRTS(recver_addr, reqSlots);
}

double AquaSimSFama::GetPktTrainTxTime() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	double txtime = 0.0;

	int q_size = m_sendingPktQ.size();
	int bytes = 0;
	AquaSimHeader ash;
	for (int i = 0; i < q_size; i++) {
		m_sendingPktQ.front()->PeekHeader(ash);
		txtime += ash.GetTxTime().ToDouble(Time::S);
		bytes += ash.GetSize();
		m_sendingPktQ.push(m_sendingPktQ.front());
		m_sendingPktQ.pop();
	}
	txtime += (q_size - 1) * m_dataSendingInterval + m_maxPrTimeSec;

	return txtime;
}

void AquaSimSFama::ScheduleRTS(AquaSimAddress recver, int slot_num) {
	NS_LOG_FUNCTION(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << recver.GetAsInt() << "Required slots to send data packet: " << slot_num);
	int backoff_slots;
	if (!m_isInBackoff)
		backoff_slots = 0;
	else
		backoff_slots = RandBackoffSlots();////////////???????????????????
	m_isInBackoff = false;
	double backoff_time = backoff_slots * m_slotLen
			+ GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));
	SetStatus(WAIT_SEND_RTS);
	m_waitSendTimer.m_pkt = MakeRTS(recver, slot_num);

	double sendSlot = (backoff_time + Simulator::Now().ToDouble(Time::S))
			/ m_slotLen;
	NS_LOG_DEBUG(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; Time to the RTS transmission: " << backoff_time << " (Backoff Slots: "<< backoff_slots <<") ; RTS slot: " << sendSlot);
	m_waitSendTimer.SetFunction(&AquaSimSFama_Wait_Send_Timer::expire,
			&m_waitSendTimer);
	m_waitSendTimer.Schedule(Seconds(backoff_time));
}

int AquaSimSFama::RandBackoffSlots() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	return (int) m_rand->GetValue(0.0, (double) m_maxBackoffSlots);
}

void AquaSimSFama::SendPkt(Ptr<Packet> pkt) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	if (AquaSimSFAMA_DEBUG) {
		pkt->Print(std::cout);
	}

	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	pkt->RemoveHeader(ash);
	pkt->RemoveHeader(SFAMAh);
	pkt->RemoveHeader(mach);

	ash.SetDirection(AquaSimHeader::DOWN);

	Time txtime = ash.GetTxTime();

	//status_handler.is_ack() = false;
	if (SFAMAh.GetPType() == SFamaHeader::SFAMA_CTS) {
		m_slotNumHandler = SFAMAh.GetSlotNum();
	}
	/*else if ( SFAMAh->packet_type == hdr_SFAMA::SFAMA_ACK ) {
	 status_handler.is_ack() = true;
	 }*/

	switch (m_device->GetTransmissionStatus()) {
	case SLEEP:
		PowerOn();
	case RECV:
		NS_LOG_INFO("RECV-SEND Collision!!!!!");
		pkt = 0;
		//do backoff??
		break;
	case NIDLE:
		//m_device->SetTransmissionStatus(SEND);
		ash.SetTimeStamp(Simulator::Now());
		NS_LOG_DEBUG("SendDownControl");

		pkt->AddHeader(SFAMAh);
		pkt->AddHeader(ash);
		pkt->AddHeader(mach);

		SendDown(pkt);
		Simulator::Schedule(txtime, &AquaSimSFama::SlotInitHandler, this);/////////////?????????????????
		break;
	default:
		//status is SEND, send too fast
		NS_LOG_INFO(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; Node:" << m_device->GetNode() << " send data too fast");
		pkt = 0;
		break;
	}

	return;
}

void AquaSimSFama::StatusProcess(int slotnum) {
	NS_LOG_FUNCTION(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << slotnum);
	//m_device->SetTransmissionStatus(NIDLE);
	//int myadd = AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt();
	switch (GetStatus()) {
	case WAIT_SEND_RTS:
		slotnum = m_rtsCtsAckNumSlotWait;
		NS_LOG_DEBUG(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; Wait CTS: " << slotnum << " slots");
		SetStatus(WAIT_RECV_CTS);
		break;
	case WAIT_SEND_CTS:
		//slotnum += 1;
		NS_LOG_DEBUG(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; Wait DATA: " << slotnum << " slots");
		SetStatus(WAIT_RECV_DATA);
		break;
	case WAIT_SEND_DATA:
		slotnum = m_rtsCtsAckNumSlotWait;//////////////////////??????????????????????????
		NS_LOG_DEBUG(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; Wait ACK: " << slotnum << " slots");
		SetStatus(WAIT_RECV_ACK);
		break;
	case WAIT_SEND_ACK:
		WaitReplyTimerProcess(true); //go to next round
		return;
	default:
		break;

	}

// 	Time time2comingslot = getTime2ComingSlot(Simulator::Now());
// 	if ( time2comingslot <  0.1 ) {
// 	  slotnum++;
// 	}

	//if( ! status_handler.is_ack() ) {
	if (m_waitReplyTimer.IsRunning()) {
		NS_LOG_WARN(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; m_waitReplyTimer running");
		return;
	}
	m_waitReplyTimer.SetFunction(&AquaSimSFama_Wait_Reply_Timer::expire,
			&m_waitReplyTimer);
	m_waitReplyTimer.Schedule(
			Seconds(
					m_slotLen * slotnum
							+ GetTime2ComingSlot(
									Simulator::Now().ToDouble(Time::S))));
	//}
	/*else {
	 status_handler.is_ack() = false;
	 }*/

	return;
}

void AquaSimSFama::BackoffTimerProcess() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	SetStatus(IDLE_WAIT);
	PrepareSendingDATA();
}

void AquaSimSFama::WaitSendTimerProcess(Ptr<Packet> pkt) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	if ( NULL == pkt) {
		NS_LOG_DEBUG(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; pkt == NULL");
		m_datasendTimer.SetFunction(&AquaSimSFama_DataSend_Timer::expire,
				&m_datasendTimer);
		m_datasendTimer.Schedule(Seconds(0.00001));
	} else {
		SendPkt(pkt);
	}
}

void AquaSimSFama::WaitReplyTimerProcess(bool directcall) {
	NS_LOG_FUNCTION(
			AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << directcall);
	/*do backoff*/
	double backoff_time = RandBackoffSlots() * m_slotLen
			+ GetTime2ComingSlot(Simulator::Now().ToDouble(Time::S));
	if (m_backoffTimer.IsRunning()) //TODO: fix it
	{
		NS_LOG_WARN(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; m_backoffTimer running");
		m_backoffTimer.Cancel();
	}
	if (directcall) {
		SetStatus(BACKOFF_FAIR);
		m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,
				&m_backoffTimer);
		m_backoffTimer.Schedule(
				Seconds(
						GetTime2ComingSlot(
								Simulator::Now().ToDouble(Time::S))));
	} else {
		SetStatus(BACKOFF);
		m_backoffTimer.SetFunction(&AquaSimSFama_Backoff_Timer::expire,
				&m_backoffTimer);
		m_backoffTimer.Schedule(Seconds(backoff_time));
	}
}

void AquaSimSFama::DataSendTimerProcess(){
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	if (!m_sendingPktQ.empty()) {
		Ptr<Packet> pkt = m_sendingPktQ.front();
		m_sendingPktQ.pop();
		m_BackupSendingPktQ.push(pkt);


		AquaSimHeader ash;
		pkt->PeekHeader(ash);
		Time txtime = ash.GetTxTime();

		SendDataPkt(pkt->Copy());
		m_datasendTimer.SetFunction(&AquaSimSFama_DataSend_Timer::expire,
				&m_datasendTimer);
		m_datasendTimer.Schedule(Seconds(m_dataSendingInterval) + txtime);


	} else {
		while (!m_BackupSendingPktQ.empty()) {
			//push all packets into m_sendingPktQ. After getting ack, release them

			m_sendingPktQ.push(m_BackupSendingPktQ.front());
			m_BackupSendingPktQ.pop();
		}
		Simulator::Schedule(Seconds(0.0000001), &AquaSimSFama::SlotInitHandler,
				this);

	}
}

void AquaSimSFama::SendDataPkt(Ptr<Packet> pkt) {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	if (AquaSimSFAMA_DEBUG) {
		pkt->Print(std::cout);
	}

	AquaSimHeader ash;
	SFamaHeader SFAMAh;
	MacHeader mach;
	pkt->RemoveHeader(ash);

	ash.SetDirection(AquaSimHeader::DOWN);

	switch (m_device->GetTransmissionStatus()) {
	case SLEEP:
		PowerOn();
	case RECV:
		NS_LOG_INFO("RECV-SEND Collision!!!!!");
		pkt = 0;
		//do backoff??
		break;
	case NIDLE:
		NS_LOG_DEBUG("SendDownData");
		//m_device->SetTransmissionStatus(SEND);
		ash.SetTimeStamp(Simulator::Now());
		pkt->RemoveHeader(SFAMAh);
		pkt->RemoveHeader(mach);

		pkt->AddHeader(SFAMAh);
		pkt->AddHeader(ash);
		pkt->AddHeader(mach);
		SendDown(pkt);
		break;
	default:
		//status is SEND, send too fast
		NS_LOG_INFO(
				AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << "; Node:" << m_device->GetNode() << " send data too fast");
		pkt = 0;
		break;
	}

	return;
}

void AquaSimSFama::SetStatus(enum AquaSimSFama_Status status) {
	std::string v = "";
	switch (status) {
	case IDLE_WAIT:
		v = "IDLE_WAIT";
		break;
	case WAIT_SEND_RTS:
		v = "WAIT_SEND_RTS";
		break;
	case WAIT_SEND_CTS:
		v = "WAIT_SEND_CTS";
		break;
	case WAIT_RECV_CTS:
		v = "WAIT_RECV_CTS";
		break;
	case WAIT_SEND_DATA:
		v = "WAIT_SEND_DATA";
		break;
	case WAIT_RECV_DATA:
		v = "WAIT_RECV_DATA";
		break;
	case WAIT_SEND_ACK:
		v = "WAIT_SEND_ACK";
		break;
	case WAIT_RECV_ACK:
		v = "WAIT_RECV_ACK";
		break;
	case BACKOFF:
		v = "BACKOFF";
		break;
	case BACKOFF_FAIR:
		v = "BACKOFF_FAIR";
		break;
	}

	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt() << v);
	/*if( status == BACKOFF ) {
	 status = status; //????
	 }*/
	m_status = status;
}

enum AquaSimSFama_Status AquaSimSFama::GetStatus() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	return m_status;
}

void AquaSimSFama::SlotInitHandler() {
	NS_LOG_FUNCTION(AquaSimAddress::ConvertFrom(GetAddress()).GetAsInt());
	StatusProcess(m_slotNumHandler);
}

void AquaSimSFama::DoDispose() {
	while (!m_sendingPktQ.empty()) {
		m_sendingPktQ.front() = 0;
		m_sendingPktQ.pop();
	}
	while (!m_CachedPktQ.empty()) {
		m_CachedPktQ.front() = 0;
		m_CachedPktQ.pop();
	}
	while (!m_BackupSendingPktQ.empty()) {
		m_BackupSendingPktQ.front() = 0;
		m_BackupSendingPktQ.pop();
	}
	m_rand = 0;
	AquaSimMac::DoDispose();
}

#ifdef AquaSimSFama_DEBUG
void
AquaSimSFama::PrintAllQ()
{
  NS_LOG_INFO("Time " << Simulator::Now().GetSeconds() << " node " <<
                m_device->GetNode() << ". m_CachedPktQ:");
  NS_LOG_INFO(m_CachedPktQ);
  NS_LOG_INFO("m_sendingPktQ:");
  NS_LOG_INFO(m_sendingPktQ);
}

void
AquaSimSFama::PrintQ(std::queue< Ptr<Packet> >& my_q)
{
	std::queue<Ptr<Packet> > tmp_q;
	AquaSimHeader ash;
	while(!my_q.empty()) {
		my_q.front()->PeekHeader(ash);
		printf("%d\t", ash.GetUId());
		tmp_q.push(my_q.front());
		my_q.pop();
		}

	while(!tmp_q.empty()) {
	  my_q.push(tmp_q.front());
	  tmp_q.pop();
	}
}

#endif

} //namespace ns3
