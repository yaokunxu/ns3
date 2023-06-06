/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//
// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.
// Added Named Data components and copied for adaptibility by Robert Martin
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/aqua-sim-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "onoff-nd-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

#include "named-data-header.h"
#include "ns3/aqua-sim-header.h"
#include "ns3/aqua-sim-socket-factory.h"
#include "ns3/aqua-sim-header-mac.h"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cassert>

namespace ns3
{

	NS_LOG_COMPONENT_DEFINE("OnOffNDApplication");

	NS_OBJECT_ENSURE_REGISTERED(OnOffNDApplication);

	TypeId OnOffNDApplication::GetTypeId(void)
	{
		static TypeId tid =
			TypeId("ns3::OnOffNDApplication").SetParent<Application>().SetGroupName("Applications").AddConstructor<OnOffNDApplication>().AddAttribute("DataRate", "The data rate in on state.", DataRateValue(DataRate("500kb/s")), MakeDataRateAccessor(&OnOffNDApplication::m_cbrRate), MakeDataRateChecker()).AddAttribute("PacketSize", "The size of packets sent in on state", UintegerValue(512), MakeUintegerAccessor(&OnOffNDApplication::m_pktSize),

																																																																															  MakeUintegerChecker<uint32_t>(1))
				.AddAttribute("Remote", "The address of the destination", AddressValue(), MakeAddressAccessor(&OnOffNDApplication::m_peer), MakeAddressChecker())
				.AddAttribute("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),

							  MakePointerAccessor(&OnOffNDApplication::m_interval), MakePointerChecker<RandomVariableStream>())
				.AddAttribute("Interval", "The time between packets sent in on state", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),

							  MakePointerAccessor(&OnOffNDApplication::m_onTime), MakePointerChecker<RandomVariableStream>())
				.AddAttribute("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"), MakePointerAccessor(&OnOffNDApplication::m_offTime), MakePointerChecker<RandomVariableStream>())
				.AddAttribute("MaxBytes", "The total number of bytes to send. Once these bytes are sent, "
										  "no packet is sent again, even in on state. The value zero means "
										  "that there is no limit.",
							  UintegerValue(0), MakeUintegerAccessor(&OnOffNDApplication::m_maxBytes), MakeUintegerChecker<uint32_t>())
				.AddAttribute("Protocol", "The type of protocol to use.",
							  // TypeIdValue (AquaSimSocketFactory::GetTypeId ()),
							  TypeIdValue(UdpSocketFactory::GetTypeId()), MakeTypeIdAccessor(&OnOffNDApplication::m_tid), MakeTypeIdChecker())
				.AddTraceSource("Tx", "A new packet is created and is sent", MakeTraceSourceAccessor(&OnOffNDApplication::m_txTrace), "ns3::Packet::TracedCallback");
		return tid;
	}

	OnOffNDApplication::OnOffNDApplication() : m_socket(0), m_connected(false), m_residualBits(0), m_lastStartTime(
																									   Seconds(0)),
											   m_totBytes(0)
	{
		// std::cout << "-----------OnOffNDApplication::OnOffNDApplication -----------\n";
		k = 1;
		NS_LOG_FUNCTION(this);
	}

	OnOffNDApplication::~OnOffNDApplication()
	{
		NS_LOG_FUNCTION(this);
	}

	void OnOffNDApplication::SetMaxBytes(uint32_t maxBytes)
	{
		NS_LOG_FUNCTION(this << maxBytes);
		m_maxBytes = maxBytes;
	}

	Ptr<Socket> OnOffNDApplication::GetSocket(void) const
	{
		NS_LOG_FUNCTION(this);
		return m_socket;
	}

	void OnOffNDApplication::SetSocket(Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION(this);
		m_socket = socket;
	}

	int64_t OnOffNDApplication::AssignStreams(int64_t stream)
	{
		NS_LOG_FUNCTION(this << stream);
		m_interval->SetStream(stream + 2);
		m_onTime->SetStream(stream);
		m_offTime->SetStream(stream + 1);
		return 2;
	}

	void OnOffNDApplication::DoDispose(void)
	{
		NS_LOG_FUNCTION(this);

		m_socket = 0;
		// chain up
		Application::DoDispose();
	}

	// Application Methods
	void OnOffNDApplication::StartApplication() // Called at time specified by Start
	{
		NS_LOG_FUNCTION(this);

		// Create the socket if not already
		if (!m_socket)
		{
			m_socket = Socket::CreateSocket(GetNode(), m_tid);
			if (AquaSimSocketAddress::IsMatchingType(m_peer))
			{
				m_socket->Bind();
			}

			m_socket->Connect(m_peer);
			m_socket->SetAllowBroadcast(false);
			m_socket->SetConnectCallback(
				MakeCallback(&OnOffNDApplication::ConnectionSucceeded, this),
				MakeCallback(&OnOffNDApplication::ConnectionFailed, this));
		}
		m_cbrRateFailSafe = m_cbrRate;

		// Insure no pending event
		// CancelEvents ();
		// If we are not yet connected, there is nothing to do here
		// The ConnectionComplete upcall will start timers at that time
		// if (!m_connected) return;
		// todo   m_socket->GetNode()->GetId() == 0
		/*if (m_socket->GetNode()->GetId() == 0)
		{
			std::cout << "APP: send id:" << m_socket->GetNode()->GetId() << "\n";
			SendPacket();
		}*/
		std::cout << "APP: send id:" << m_socket->GetNode()->GetId() << "\n";
		SendPacket();

		ScheduleStartEvent_sink();
	}
	void OnOffNDApplication::HandleRead(Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION(this << socket);

		Ptr<Packet> packet;
		Address from;
		while ((packet = socket->RecvFrom(from)))
		{
			if (AquaSimSocketAddress::IsMatchingType(from))
			{

				std::cout << "At time " << Simulator::Now().GetSeconds()
						  << "s server received " << packet->GetSize()
						  << " bytes from "
						  << AquaSimSocketAddress::ConvertFrom(from).GetDestinationAddress()
						  << "\n";
			}

			packet->RemoveAllPacketTags();
			packet->RemoveAllByteTags();
		}
	}
	void OnOffNDApplication::StopApplication() // Called at time specified by Stop
	{
		NS_LOG_FUNCTION(this);

		CancelEvents();
		if (m_socket != 0)
		{
			m_socket->Close();
		}
		else
		{
			NS_LOG_WARN(
				"OnOffNDApplication found null socket to close in StopApplication");
		}
	}

	void OnOffNDApplication::CancelEvents()
	{
		NS_LOG_FUNCTION(this);

		if (m_sendEvent.IsRunning() && m_cbrRateFailSafe == m_cbrRate)
		{ // Cancel the pending send packet event
		  // Calculate residual bits since last packet sent
			Time delta(Simulator::Now() - m_lastStartTime);
			int64x64_t bits = delta.To(Time::S) * m_cbrRate.GetBitRate();
			m_residualBits += bits.GetHigh();
		}
		m_cbrRateFailSafe = m_cbrRate;
		Simulator::Cancel(m_sendEvent);
		Simulator::Cancel(m_recvEvent);
		Simulator::Cancel(m_startStopEvent);
	}

	// Event handlers
	void OnOffNDApplication::StartSending()
	{
		NS_LOG_FUNCTION(this);
		m_lastStartTime = Simulator::Now();
		SendPacket();
		ScheduleNextTx(); // Schedule the send packet event
	}

	void OnOffNDApplication::StartRecving()
	{
		NS_LOG_FUNCTION(this);
		m_lastStartTime = Simulator::Now();
		ScheduleNextTx_sink(); // Schedule the send packet event
		ScheduleStopEvent_sink();
	}

	void OnOffNDApplication::StopSending()
	{
		NS_LOG_FUNCTION(this);
		CancelEvents();

		ScheduleStartEvent();
	}
	void OnOffNDApplication::StopRecving()
	{
		NS_LOG_FUNCTION(this);
		CancelEvents();

		ScheduleStartEvent_sink();
	}
	// Private helpers
	void OnOffNDApplication::ScheduleNextTx()
	{
		NS_LOG_FUNCTION(this);
		std::cout << "APP: m_maxBytes=" << m_maxBytes << ",m_totBytes=" << m_totBytes << "\n";
		if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
		{
			uint32_t bits = m_pktSize * 8 - m_residualBits;
			NS_LOG_LOGIC("bits = " << bits);
			double interval = m_interval->GetValue();
			interval = 1;
			std::cout << "App: bits=" << bits << ",m_pktsize=" << m_pktSize << ",m_residulBits=" << m_residualBits << "\n";
			Time nextTime(
				Seconds(bits / static_cast<double>(m_cbrRate.GetBitRate()) + interval)); // Time till next packet
			std::cout << "APP:nextschedule time:" << nextTime << "\n";
			m_sendEvent = Simulator::Schedule(nextTime / 1000,
											  &OnOffNDApplication::SendPacket, this);
		}
		else
		{ // All done, cancel any pending events
			StopApplication();
		}
	}

	void OnOffNDApplication::ScheduleNextTx_sink()
	{
		NS_LOG_FUNCTION(this);
		Time nextTime(0.01);
		m_recvEvent = Simulator::Schedule(nextTime, &OnOffNDApplication::RecvPacket,
										  this);
	}

	// todo add schedule send pkt
	void OnOffNDApplication::ScheduleNextpkt()
	{
		Simulator::Schedule(Seconds(GetNode()->GetId() + rand() % 30), &OnOffNDApplication::SendPacket,
							this);
	}

	void OnOffNDApplication::ScheduleStartEvent()
	{ // Schedules the event to start sending data (switch to the "On" state)
		NS_LOG_FUNCTION(this);

		Time offInterval = Seconds(m_offTime->GetValue());
		NS_LOG_LOGIC("start at " << offInterval);
		m_startStopEvent = Simulator::Schedule(offInterval,
											   &OnOffNDApplication::StartSending, this);
	}

	void OnOffNDApplication::ScheduleStartEvent_sink()
	{ // Schedules the event to start sending data (switch to the "On" state)
		NS_LOG_FUNCTION(this);

		Time offInterval = Seconds(m_offTime->GetValue());
		NS_LOG_LOGIC("start at " << offInterval);
		m_startStopEvent = Simulator::Schedule(offInterval,
											   &OnOffNDApplication::StartRecving, this);
	}

	void OnOffNDApplication::ScheduleStopEvent()
	{ // Schedules the event to stop sending data (switch to "Off" state)
		NS_LOG_FUNCTION(this);

		Time onInterval = Seconds(m_onTime->GetValue());
		//	NS_LOG_LOGIC("stop at " << onInterval);
		m_startStopEvent = Simulator::Schedule(onInterval,
											   &OnOffNDApplication::StopSending, this);
	}
	void OnOffNDApplication::ScheduleStopEvent_sink()
	{ // Schedules the event to stop sending data (switch to "Off" state)
		NS_LOG_FUNCTION(this);

		Time onInterval = Seconds(m_onTime->GetValue());
		NS_LOG_LOGIC("stop at " << onInterval);
		m_startStopEvent = Simulator::Schedule(onInterval,
											   &OnOffNDApplication::StopRecving, this);
	}

	// send p

	void OnOffNDApplication::SendPacket()
	{
		NS_LOG_FUNCTION(this);
		std::ostringstream interest;
		interest << "Hello" << '\0';
		Ptr<Packet> packet = Create<Packet>((uint8_t *)interest.str().c_str(),
											300);
		m_txTrace(packet);
		// if(k < 2)
		// Simulator::Schedule(Seconds(5), &OnOffNDApplication::SendPacket, this);
		m_socket->Send(packet);
		m_totBytes += m_pktSize;

		std::cout << "pkt" << k++ << "At time " << Simulator::Now().GetSeconds()
				  << "s on-off application sent "
				  << packet->GetSize() << " bytes to "
				  << GetNode()->GetId()
				  << " total Tx " << m_totBytes << " bytes\n";

		m_lastStartTime = Simulator::Now();
		m_residualBits = 0;
		// if(Simulator::Now() < Seconds(1000))
		// if(Simulator::Now() < Seconds(500))
		// if(Simulator::Now() < Seconds(205))
		// if(Simulator::Now() < Seconds(50))

		// todo设置发送时间频率
		// ScheduleNextTx();
		ScheduleNextpkt();
	}

	void OnOffNDApplication::RecvPacket()
	{

		NS_LOG_FUNCTION(this);

		NS_ASSERT(m_recvEvent.IsExpired());

		NS_LOG_FUNCTION(this << m_socket);
		Ptr<Packet> packet;
		Address from;
		while ((packet = m_socket->RecvFrom(from)))
		{
			AquaSimHeader ash;
			// packet->RemoveHeader(ash);
			std::cout << "At time " << Simulator::Now().GetSeconds()
					  << "s on-off application receive " << packet->GetSize() << "\n";
			m_lastStartTime = Simulator::Now();
		}
	}

	void OnOffNDApplication::ConnectionSucceeded(Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION(this << socket);
		m_connected = true;
	}

	void OnOffNDApplication::ConnectionFailed(Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION(this << socket);
	}

} // Namespace ns3
