/*
 * aqua-sim-mac-phy-fama.h
 *
 *  Created on: Mar 24, 2020
 *      Author: anna
 */

#ifndef AQUA_SIM_MAC_PHY_FAMA_H_
#define AQUA_SIM_MAC_PHY_FAMA_H_

#include "ns3/random-variable-stream.h"
#include "aqua-sim-mac.h"
#include "ns3/timer.h"
#include "aqua-sim-rmac-buffer.h"

#include <queue>
#include <vector>

#define CALLBACK_DELAY 0.001

namespace ns3{

class Time;
class Packet;
class AquaSimAddress;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief FAMA implementation
 */
class AquaSimPhyFama: public AquaSimMac {
public:
	AquaSimPhyFama();
  ~AquaSimPhyFama();
  static TypeId GetTypeId(void);
  int64_t AssignStreams (int64_t stream);

  virtual bool TxProcess(Ptr<Packet> pkt);
  virtual bool RecvProcess(Ptr<Packet> pkt);

protected:

  enum {
    PASSIVE,
    BACKOFF,
    WAIT_CTS,
    WAIT_DATA_FINISH,
    WAIT_DATA,
    REMOTE   /*I don't know what it means. but
		     node can only receive packet in this status*/
  }FamaStatus;
  double m_NDPeriod;
  int  m_maxBurst;	//the maximum number of packet burst. default is 1
  Time m_dataPktInterval;  //0.0001??

  Time m_estimateError;		//Error for timeout estimation
  int m_dataPktSize;



  double m_transmitDistance;
    //distCST_ from ns2. this should be NOT be manual and instead be calc within channel.
  Time m_maxPropDelay;
  Time m_RTSTxTime;
  Time m_CTSTxTime;

  Time m_maxDataTxTime;


  std::queue<Ptr<Packet> > PktQ;
  std::queue<Ptr<Packet> > PktQ_resend;


  Timer m_waitCTSTimer;
  Timer m_backoffTimer;
  Timer m_remoteTimer;
  Time m_remoteExpireTime;
  Timer m_waitACKTimer;
  TransmissionBuffer m_txBuffer;

  //packet_t UpperLayerPktType;



  Ptr<Packet> MakeRTS(AquaSimAddress Recver);
  Ptr<Packet> MakeCTS(AquaSimAddress RTS_Sender);
  Ptr<Packet> MakeACKDATA(AquaSimAddress DATA_Sender);

//  void ProcessND(AquaSimAddress sa);
//  void ProcessRTS(AquaSimAddress sa);
 void ProcessData(AquaSimAddress sa);

  void SendRTS(Time DeltaTime);
  void SendPkt(Ptr<Packet> pkt);
  void SendDataPkt();

//  void ProcessDataSendTimer(Ptr<Packet> pkt);
//  void ProcessDataBackoffTimer();
 // void ProcessRemoteTimer();
//  void NDTimerExpire();//periodically send out Neighbor discovery packet for 4 times.
//  void ReSendRTSTimerExpire();

//  void ReSendRTS();

  //void BackoffTimerExpire();
  void ReSendRTS();
  bool CarrierDected();
  //void DoBackoff();
  //oid DoRemote(Time DeltaTime);

  //virtual void DoDispose();

private:
  Ptr<UniformRandomVariable> m_rand;
};

}  // namespace ns3




#endif /* AQUA_SIM_MAC_PHY_FAMA_H_ */
