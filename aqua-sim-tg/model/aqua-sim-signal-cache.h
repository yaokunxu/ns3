/*
 * aqua-sim-signal-cache.h
 *
 *  Created on: Sep 5, 2021
 *  Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */

#ifndef AQUA_SIM_SIGNAL_CACHE_H
#define AQUA_SIM_SIGNAL_CACHE_H

#include <queue>

#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/object.h"

#include "aqua-sim-phy.h"
#include "aqua-sim-noise-generator.h"
#include "aqua-sim-header.h"

//Aqua Sim Signal Cache

namespace ns3 {

struct IncomingPacket: Object {
	Ptr<Packet> packet;
	AquaSimPacketStamp::PacketStatus status;
	Ptr<IncomingPacket> next;
	double interference;
	double singalPower;

	IncomingPacket(AquaSimPacketStamp::PacketStatus s = AquaSimPacketStamp::INVALID) :
			packet(NULL), status(s), next(NULL), interference(0) {
	}
	IncomingPacket(Ptr<Packet> p,
			AquaSimPacketStamp::PacketStatus s = AquaSimPacketStamp::INVALID) :
			packet(p), status(s), next(NULL), interference(0) {
	}
};

class AquaSimSignalCache;








/**
 * \brief Signal Cache class which simulates the way that modems handle signals without considering multi-path effect
 */




class AquaSimSignalCache: public Object {
public:
	AquaSimSignalCache(void);
	virtual ~AquaSimSignalCache(void);
	static TypeId GetTypeId(void);

	virtual void SubmitPkt(Ptr<IncomingPacket> inPkt);

	void AddNewPacket(Ptr<Packet>);	
	bool DeleteIncomingPacket(Ptr<Packet>);
	Ptr<IncomingPacket> Lookup(Ptr<Packet>);


	void SetNoiseGen(Ptr<AquaSimNoiseGen> noise);
	double GetNoise();


protected:
	virtual void UpdatePacketStatus(AquaSimPacketStamp pstamp) = 0;
	void DoDispose();

public:
	int m_pktNum;	// number of active incoming packets

protected:
	Ptr<IncomingPacket> m_head;
	Ptr<AquaSimPhy> m_phy;
	Ptr<AquaSimNoiseGen> m_noise;

private:
	void AttachPhy(Ptr<AquaSimPhy> phy) {
		m_phy = phy;
	}
	friend class AquaSimPhy;
};








class AquaSimSignalCacheSINR: public AquaSimSignalCache {
public:
	AquaSimSignalCacheSINR(void);
	virtual ~AquaSimSignalCacheSINR(void);
	static TypeId GetTypeId(void);

	virtual void SubmitPkt(Ptr<IncomingPacket> inPkt);

protected:
	virtual void UpdatePacketStatus(AquaSimPacketStamp pstamp);

	void DoDispose();
	bool decodeable(Ptr<IncomingPacket> inPkt);


	double interference;


};
//class AquaSimSignalCacheSINR









class AquaSimSignalCacheRange: public AquaSimSignalCache {
public:

	AquaSimSignalCacheRange();
	virtual ~AquaSimSignalCacheRange();
	static TypeId GetTypeId(void);

protected:
	virtual void UpdatePacketStatus(AquaSimPacketStamp pstamp);
	void DoDispose();


};
//class AquaSimSignalCacheRange


}//namespace ns3

#endif /* AQUA_SIM_SIGNAL_CACHE_H */
