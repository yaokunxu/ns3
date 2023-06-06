/*
 * aqua-sim-mac-broadcast-cluster.h
 *
 *  Created on: Apr 28, 2020
 *      Author: anna
 */

#ifndef AQUA_SIM_MAC_BROADCAST_CLUSTER_H_
#define AQUA_SIM_MAC_BROADCAST_CLUSTER_H_


#include "aqua-sim-mac.h"

namespace ns3 {

#define BC_BACKOFF  0.1//0.5 //default is 0.1 the maximum time period for backoff
#define BC_MAXIMUMCOUNTER 4//15 //default is 4 the maximum number of backoff
#define BC_CALLBACK_DELAY 0.0001 // the interval between two consecutive sendings

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Broadcast MAC using basic backoff mechanism
 */
class AquaSimBroadcastClusterMac : public AquaSimMac
{
public:
	AquaSimBroadcastClusterMac();
  int64_t AssignStreams (int64_t stream);
  struct node
  {
	  int node_num;
	  int rank;
	  int squence;
	  double distance;
	  double speed;
	  int size;
	  double assessment_value;
  };

  struct Position
  {
  	int x ;
  	int y ;
  	int num;

  };
  static bool comp(const node & a, const node & b)
  {
  	return a.assessment_value > b.assessment_value;
  }

  static bool dis(const node & a, const node & b)
    {
    	return a.distance > b.distance;
    }
  std::vector <node> Q;//用于存放发送rts的结点
  std::vector <Position> pos;
  double Max_speed,Min_speed;
  int Max_distance,Min_distance;
  int Max_size,Min_size;
  static TypeId GetTypeId(void);
  int m_packetHeaderSize; //# of bytes in the header
  int m_packetSize;  //to test the optimized length of packet
  std::queue<Ptr<Packet> > PktQ;//要发送的包放在队列里面
  int distance;//普通结点和簇头的距离

  // to process the incoming packet
  void CalTimerExpireOrder();
  bool G_channel(std::string content);
  void CalTimerAssessmentValue();
  void CalTimerExpireOrder1();
  void S_Channel();
  bool ClusterSendPos();
  void sendRTS();
  bool SendDataPacket();
  bool sendToMembers();
  virtual bool RecvProcess (Ptr<Packet>);
  void SendPkt(Ptr<Packet> pkt);
  void CallbackProcess ();
  void DropPacket (Ptr<Packet>);
  bool SendReplyMess(int cluster_header);
  void CalTimerExpire();
  // to process the outgoing packet
  virtual bool TxProcess (Ptr<Packet>);
protected:
  void BackoffHandler(Ptr<Packet>);
  virtual void DoDispose();
private:
  int m_backoffCounter;
  Ptr<UniformRandomVariable> m_rand;

};  // class AquaSimBroadcastClusterMac

} // namespace ns3




#endif /* AQUA_SIM_MAC_BROADCAST_CLUSTER_H_ */
