/*
 * aqua-sim-signal-cache.cc
 *
 *  Created on: Sep 5, 2021
 *  Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */

//#include ...
#include "aqua-sim-signal-cache.h"
#include "aqua-sim-header.h"
#include "aqua-sim-trailer.h"
#include "aqua-sim-header-mac.h"

#include <queue>

#include "aqua-sim-phy.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

//Aqua Sim Signal Cache

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimSignalCache");
NS_OBJECT_ENSURE_REGISTERED(AquaSimSignalCache);

AquaSimSignalCache::AquaSimSignalCache() 
:m_pktNum(0)
{
	NS_LOG_FUNCTION(this);

	m_head = CreateObject<IncomingPacket>(AquaSimPacketStamp::INVALID);
}

AquaSimSignalCache::~AquaSimSignalCache() {
}

TypeId AquaSimSignalCache::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::AquaSimSignalCache")
						.SetParent<Object>();
	return tid;
}

void AquaSimSignalCache::AddNewPacket(Ptr<Packet> p) {

	AquaSimPacketStamp pstamp;
	p->RemoveHeader(pstamp);
	Time transmissionDelay = m_phy->calcTxTime(p);

	uint8_t status = pstamp.GetPacketStatus();
	Ptr<IncomingPacket> inPkt = CreateObject<IncomingPacket>(p,
			(AquaSimPacketStamp::PacketStatus)status);
	inPkt->singalPower = pstamp.GetPr();


	Simulator::Schedule(transmissionDelay, &AquaSimSignalCacheRange::SubmitPkt, 
					this, inPkt);



	inPkt->next = m_head->next;
	m_head->next = inPkt;
	m_pktNum++;
	UpdatePacketStatus(pstamp);
}




bool AquaSimSignalCache::DeleteIncomingPacket(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this);
	Ptr<IncomingPacket> pre = m_head;
	Ptr<IncomingPacket> ptr = m_head->next;

	while (ptr != NULL && ptr->packet != p) {
		ptr = ptr->next;
		pre = pre->next;
	}

	if (ptr != NULL && ptr->packet == p) {
		m_pktNum--;
		pre->next = ptr->next;
		ptr = 0;
		return true;
	}
	NS_LOG_DEBUG(
			"DeleteIncomingPacket: ptr:" << ptr << "ptr(packet) == p?" << (ptr->packet != p));
	return false;
}

void AquaSimSignalCache::SubmitPkt(Ptr<IncomingPacket> inPkt) {
	NS_LOG_FUNCTION(this << inPkt << inPkt->status);

	AquaSimPacketStamp::PacketStatus status = inPkt->status;
	Ptr<Packet> p = inPkt->packet;
	DeleteIncomingPacket(p);

	if (status != AquaSimPacketStamp::RECEPTION) {
		NS_LOG_DEBUG("Packet(" << p << ") dropped");
		p = 0;
	}
	else
		m_phy->SignalCacheCallback(p);
}


Ptr<IncomingPacket> AquaSimSignalCache::Lookup(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this);
	Ptr<IncomingPacket> ptr = m_head->next;

	while (ptr != NULL && ptr->packet != PeekPointer(p)) {
		ptr = ptr->next;
	}
	return ptr;
}


void AquaSimSignalCache::SetNoiseGen(Ptr<AquaSimNoiseGen> noise) {
	NS_LOG_FUNCTION(this);
	m_noise = noise;
}

double AquaSimSignalCache::GetNoise() {
	return m_noise->Noise();
}

void AquaSimSignalCache::DoDispose() {
	NS_LOG_FUNCTION(this);

	Ptr<IncomingPacket> pos = m_head;
	while (m_head != NULL) {
		m_head = m_head->next;
		pos->packet = 0;
		pos = 0;
		pos = m_head;
	}
	m_phy = 0;
	m_noise = 0;
	Object::DoDispose();
}




















NS_OBJECT_ENSURE_REGISTERED(AquaSimSignalCacheSINR);


AquaSimSignalCacheSINR::AquaSimSignalCacheSINR()
:AquaSimSignalCache(), interference(0)
{
}



AquaSimSignalCacheSINR::~AquaSimSignalCacheSINR() {
}

TypeId AquaSimSignalCacheSINR::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::AquaSimSignalCacheSINR")
					.SetParent<AquaSimSignalCache>()
					.AddConstructor<AquaSimSignalCacheSINR>();
	return tid;
}



void AquaSimSignalCacheSINR::SubmitPkt(Ptr<IncomingPacket> inPkt) {
	NS_LOG_FUNCTION(this << inPkt << inPkt->status);
	double temp = pow(10, this->interference / 10) - pow(10, inPkt->singalPower / 10);
	this->interference = 10 * log10(temp);

	bool flag = this->decodeable(inPkt);
	if(!flag){
		inPkt->status = AquaSimPacketStamp::INVALID;
	}

	AquaSimSignalCache::SubmitPkt(inPkt);
}

bool AquaSimSignalCacheSINR::decodeable(Ptr<IncomingPacket> inPkt){
	Ptr<Packet> pkt = inPkt->packet;
 	AquaSimTrailer ast;

	double singalPower = inPkt->singalPower;
	int byteLen = pkt->GetSize() - ast.GetSerializedSize();


	double temp = pow(10, interference / 10) - pow(10, singalPower / 10)
					+ pow(10, m_noise->Noise() / 10);
	double inter = 10 * log10(temp);
	double sinr = singalPower - inter;
	double ber = sinr > 0? erfc(sqrt(sinr)) / 2 : 1;
	double per = 1 - pow(1 - ber,byteLen * 8);
	std::cout << "per:" << per << std::endl;

	double r = (rand() % 10001) / 10000.0;

	std::cout << "rand num:"<< r << std::endl;
	if(r < per){
		return false;
	}
	return true;
}




void AquaSimSignalCacheSINR::UpdatePacketStatus(AquaSimPacketStamp pstamp) {
	NS_LOG_FUNCTION(this);
	
	double temp = pow(10, (this->interference / 10)) + pow(10 ,(pstamp.GetPr()/10));
	this->interference = 10 * log10(temp);

	for (Ptr<IncomingPacket> ptr = m_head->next; ptr != NULL; ptr = ptr->next) {

		if(this->interference > ptr->interference)
			ptr->interference = this->interference;
	}
}


void AquaSimSignalCacheSINR::DoDispose() {
	NS_LOG_FUNCTION(this);

	AquaSimSignalCache::DoDispose();
}






















NS_OBJECT_ENSURE_REGISTERED(AquaSimSignalCacheRange);

AquaSimSignalCacheRange::AquaSimSignalCacheRange()
:AquaSimSignalCache()
{
}

AquaSimSignalCacheRange::~AquaSimSignalCacheRange() {
}

TypeId AquaSimSignalCacheRange::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::AquaSimSignalCacheRange")
						.SetParent<AquaSimSignalCache>()
						.AddConstructor<AquaSimSignalCacheRange>();
	return tid;
}






void AquaSimSignalCacheRange::UpdatePacketStatus(AquaSimPacketStamp pstamp) {

	NS_LOG_FUNCTION(this);
	if(m_pktNum == 1)
		return;
	Ptr<IncomingPacket> ptr = m_head->next;

	for (; ptr != NULL; ptr = ptr->next) {
		ptr->status = AquaSimPacketStamp::COLLISION;
	}

}



void AquaSimSignalCacheRange::DoDispose() {
	NS_LOG_FUNCTION(this);

	AquaSimSignalCache::DoDispose();
}


}
;
// namespace ns3
