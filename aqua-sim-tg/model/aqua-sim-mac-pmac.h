//
// Created by chhhh on 2021/2/20.
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

#ifndef AQUA_SIM_NG_MASTER_AQUA_SIM_MAC_PMAC_H
#define AQUA_SIM_NG_MASTER_AQUA_SIM_MAC_PMAC_H

#include "aqua-sim-mac.h"

#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"
#include "ns3/packet.h"

#include <queue>
#include <map>
#include <math.h>

namespace ns3
{

	class AquaSimPmac;

	class AquaSimAddress;

	/**
 * \ingroup aqua-sim-ng
 *
 * \brief Implementation of pmac protocol in underwater
 */

	class AquaSimPmac : public AquaSimMac
	{
	public:
		AquaSimPmac();

		~AquaSimPmac();

		static TypeId GetTypeId(void);

		int64_t AssignStreams(int64_t stream);

		virtual bool TxProcess(Ptr<Packet> pkt);

		virtual bool RecvProcess(Ptr<Packet> pkt);
		
		virtual uint32_t getHeaderSize();

	protected:
		void init();
		void sendInitPacket(Ptr<Packet> pkt);
		void sendInitAckPacket(Ptr<Packet> pkt);

		Ptr<Packet> makeProbe();
		Ptr<Packet> makeAckProbe(Ptr<Packet> pkt);
		Ptr<Packet> makeTrigger();
		Ptr<Packet> makeAckTrigger();
		Ptr<Packet> makeTimeAlign();
		Ptr<Packet> makeAckTimeAlign();


		void retryInitPacket(Ptr<Packet> pkt);

		void processProbe(Ptr<Packet> pkt);
		void processAckProbe(Ptr<Packet> pkt);
		void processTrigger(Ptr<Packet> pkt);
		void processAckTrigger(Ptr<Packet> pkt);
		void processTimeAlign(Ptr<Packet> pkt);
		void processAckTimeAlign(Ptr<Packet> pkt);



		Time makeTime();
		Ptr<Packet> MakeACK();
		void SendPkt(Ptr<Packet> pkt);
		void SendDataPkt();
		void waitToPassive();
		bool recvInitPacket(Ptr<Packet> pkt);
		virtual void DoDispose();

		EventId retryId;

		
		enum
		{
			PASSIVE,
			SEND_DATA,
			WAIT_ACK,
			WAIT_PROBE,
			WAIT_ACK_PROBE,
			WAIT_TRIGGER,
			WAIT_ACK_TRIGGER,
			WAIT_TIME_ALIGN,
			WAIT_ACK_TIME_ALIGN
		} PMAC_Status;

		Time startTime;
		int m_AckOn; /// 是否需要ack，默认需要
		int m_ack;
		int m_maxDataPktSize; ///最大包长
		Time timeSlot;
		Time nextHopSlot;
		int token;

		uint8_t nextSendPktId;
		uint8_t nextRecvPktId;

		AquaSimAddress preNode;
		AquaSimAddress endNode;
		AquaSimAddress begNode;

		std::queue<Ptr<Packet>> PktQ_;
		Ptr<UniformRandomVariable> m_rand;

	}; // class AquaSimAloha

} // namespace ns3

#endif //AQUA_SIM_NG_MASTER_AQUA_SIM_MAC_PMAC_H
