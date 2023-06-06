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

#include "aqua-sim-mac-contrast.h"

#include "aqua-sim-header.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-address.h"


#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/simulator.h"

#include <math.h>
#include "iterator"
#include "vector"
#include "ns3/aqua-sim-pt-tag.h"

#include<ctime>
#include <iostream>
#include<vector>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimContrast");
NS_OBJECT_ENSURE_REGISTERED(AquaSimContrast);


/* ======================================================================
Broadcast MAC for  underwater sensor
====================================================================== */

AquaSimContrast::AquaSimContrast()
{
  m_backoffCounter=0;
  m_rand = CreateObject<UniformRandomVariable> ();

  Simulator::Schedule(Seconds(1), &AquaSimContrast::CalTimerExpireOrder1, this);
}
void
AquaSimContrast::CalTimerExpireOrder1()
{
	if(m_device!=NULL)
		{
		  int number=AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt();
		  if(number==1)
				 // ||number==2||number==3||number==4)
		  {
			  ClusterSendPos();
			  Simulator::Schedule(Seconds(15), &AquaSimContrast::CalTimerExpireOrder, this);//15s后由簇头结点发送通知包，通知其成员可以发送rts
		  }
		  else
		  {
			  Simulator::Schedule(Seconds(3), &AquaSimContrast::CalTimerExpire, this);//3s后触发计算簇头的函数
		  }
		}

}
int Sumc(double **P,int row,int start,int end)
{
	int sum=0;
	for(int i=start;i<=end;i++)
	{
		sum+=P[row][i];
	}
	return sum;
}
bool AquaSimContrast::G_channel(std::string content)
{
	  AquaSimHeader ash;
	  MacHeader mach;
	  AquaSimPtTag ptag;
	  //AquaSimAddress::GetBroadcast()
	  //ash.SetDAddr(AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(member)));
	  ash.SetDAddr(AquaSimAddress::GetBroadcast());
	  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDA(AquaSimAddress::GetBroadcast());//i do not know the next hop in this mac
	  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
	  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_G_Channel);

//		std::string content;
//		content=std::to_string(start)+"#"+std::to_string(end)+"#";
      std::cout<<m_device->GetAddress()<<"    "<<content<<"  "<<std::endl;
		std::ostringstream interest;
		interest <<content<< '\0';
		Ptr<Packet> packet = Create<Packet>((uint8_t*)interest.str().c_str(), interest.str().length());



	  packet->AddPacketTag(ptag);

	  packet->AddHeader(ash);
	  packet->AddHeader(mach);



	   double trans=packet->GetSize()/667+0.8+distance/1500;
	  	 // std::cout<<"before packet!"<<std::endl;
	  	  SendPkt(packet);
	  	 // std::cout<<"after packet!"<<std::endl;
	  	  std::vector <node> Dis=Q;
	  	  sort(Dis.begin(), Dis.end(), dis);
	  	  double delay=Dis[0].distance/1500+Dis[0].size/667+0.8+1;
	  	  Simulator::Schedule(Seconds(delay+trans), &AquaSimContrast::CalTimerExpireOrder, this);
	  return true;
}
void
AquaSimContrast::S_Channel()
{
int cal_squence=0;
int x1=0;//本次通信的结点个数
int x=0;//总结点数
x=Q.size();
double wsum=0;
double xl=0;
for(unsigned int i=0;i<Q.size();i++)
{
	if(cal_squence+Q[i].squence<512)
	{
		cal_squence+=Q[i].squence;
		x1++;
	}
	else
		break;
}
std::cout<<"X1:  "<<x1<<std::endl;
std::cout<<"X:  "<<x<<std::endl;

double SNR[15][512];
double ** W=new double *[x];
for(int i=0;i<x;i++)
{
	W[i]=new double [512];
}

srand(time(NULL));
for(int i=0;i<x;i++)
{
	for(int j=0;j<512;j++)
	{
		//double n=(rand()%3),m=(rand()%10);
	//	double r=rand()%100;
		//double a=n+m/10.0;

		SNR[i][j]=rand()%10;
		W[i][j]=(SNR[i][j]+1)*5;
	//	ber[i][j]=r/1000.0;
	}
}



double * Visit=new double [512];//记录子载波是否已分配
for(int i=0;i<512;i++){
	Visit[i]=0;
}

int count=0;
std::string content="";
std::string total="";
srand((unsigned)time(NULL));
for(int i=0;i<x1;i++)
{
	int num=512/Q[i].squence;

	int ran=rand()%num+1;
//	std::cout<<"ran:"<<ran<<"\n";
	int c=(ran-1)*Q[i].squence;
	int d=ran*Q[i].squence-1;
	double sum=0;

   double w=Sumc(W,i,c,d)/(d-c+1);
   wsum+=w;


	for(int j=c;j<=d;j++)
	{
		sum=sum+Visit[j];
	}
	if(sum!=0) count++;
	else
	{
		for(int j=c;j<d;j++)
		{
			Visit[j]=1;
		}

		content=content+std::to_string(Q[i].node_num)+"#"+std::to_string(c)+"*"+std::to_string(d)+"#";
	   xl+=Q[i].size/w;
	}

}
total=std::to_string(x1-count)+"#"+content;
G_channel(total);
std::cout<<"count:  "<<count<<std::endl;
std::cout<<"xl:"<<xl<<"\n";
std::cout<<"wsum:"<<wsum<<"\n";

std::ofstream ofile;               //定义输出文件
ofile.open("result1.txt",std::ios::app);     //作为输出文件打开
ofile<<"x1 number "<<x1<<std::endl;   //标题写入文件
ofile<<"count number "<<count<<std::endl;
ofile<<"xl: "<<xl<<std::endl;
ofile<<"wsum: "<<wsum<<std::endl;
ofile.close();


std::vector <node>().swap(Q);


}
TypeId
AquaSimContrast::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimContrast")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimContrast>()
      .AddAttribute("PacketHeaderSize", "Size of packet header",
        IntegerValue(0),
        MakeIntegerAccessor (&AquaSimContrast::m_packetHeaderSize),
        MakeIntegerChecker<int> ())
      .AddAttribute("PacketSize", "Size of packet",
	IntegerValue(0),
	MakeIntegerAccessor (&AquaSimContrast::m_packetSize),
	MakeIntegerChecker<int> ())
    ;
  return tid;
}



void
AquaSimContrast::SendPkt(Ptr<Packet> pkt)
{
	  MacHeader mach;
	  AquaSimHeader ash;
	  pkt->RemoveHeader(mach);
	  pkt->RemoveHeader(ash);
	  switch( m_device->GetTransmissionStatus() )
	  {
	  case SLEEP:
	      PowerOn();
	      break;
	  case NIDLE:

	      ash.SetDirection(AquaSimHeader::DOWN);
	      //ash->addr_type()=NS_AF_ILINK;
	      //add the sync hdr
	  	   pkt->AddHeader(ash);
	      //Phy()->SetPhyStatus(PHY_SEND);
	  	   pkt->AddHeader(mach);
	  //    std::cout<<"send result "<<SendDown(pkt)<<std::endl;
	  	  SendDown(pkt);
	      m_backoffCounter=0;
	      return;
	  case RECV:
	    {
	      double backoff=m_rand->GetValue()*BC_BACKOFF;
	      NS_LOG_DEBUG("BACKOFF time:" << backoff << " on node:" << m_device->GetAddress() << "\n");
	      //pkt->AddHeader(mach);
	      pkt->AddHeader(ash);
	      Simulator::Schedule(Seconds(backoff),&AquaSimContrast::BackoffHandler,this,pkt);
	      return;
	    }

	  case SEND:
	    {
	      double backoff=m_rand->GetValue()*BC_BACKOFF;
	      NS_LOG_DEBUG("BACKOFF time:" << backoff << " on node:" << m_device->GetAddress() << "\n");
	      //pkt->AddHeader(mach);
	      pkt->AddHeader(ash);
	      Simulator::Schedule(Seconds(backoff),&AquaSimContrast::BackoffHandler,this,pkt);
	    }
	    return;
	      /*pkt=0;*/
	  default:
	      /*
	      * all cases have been processed above, so simply return
	      */
	    break;
	  }
}



bool AquaSimContrast::SendDataPacket()
{
	  AquaSimHeader ash;
	  MacHeader mach;
	  AquaSimPtTag ptag;
	  ash.SetDAddr(AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(m_device->GetClusterHead())));
	  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDA(AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(m_device->GetClusterHead())));//i do not know the next hop in this mac
	  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
	  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_DATA);

	  if(!PktQ.empty()){

	 	  Ptr<Packet> pkt=PktQ.front();

	 	//  pkt->Print(std::cout);
	 	  AquaSimHeader ash1;
	 	  AquaSimPtTag ptag1;
	 	  pkt->RemoveHeader(ash1);
	      pkt->RemovePacketTag(ptag1);
	 	  pkt->AddPacketTag(ptag);
	 	  pkt->AddHeader(ash);
	 	  pkt->AddHeader(mach);
	 	  SendPkt(pkt);
	 	  }
	 	  PktQ.pop();
	return true;
}



int64_t
AquaSimContrast::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

/*
this program is used to handle the received packet,
it should be virtual function, different class may have
different versions.
*/
bool
AquaSimContrast::RecvProcess (Ptr<Packet> pkt)
{

  NS_LOG_FUNCTION(this);
//  std::cout << "\nBMac @RecvProcess check:\n";
//  pkt->Print(std::cout);
// std::cout << "\n";

  AquaSimHeader ash;
  MacHeader mach;
  AquaSimPtTag ptag;
//  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_CLUSTER);
 pkt->RemoveHeader(mach);
 pkt->RemoveHeader(ash);
 pkt->RemovePacketTag(ptag);
  if(ptag.GetPacketType()==AquaSimPtTag::PT_MCMAC_CLUSTER)
  {

	  AquaSimAddress dst = mach.GetDA();
	  AquaSimAddress destination = ash.GetDAddr();
		if (ash.GetErrorFlag())
		{
			NS_LOG_DEBUG("BroadcastMac:RecvProcess: received corrupt packet.");
			pkt=0;
			return false;
		}
     int number=AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt();
   //  std::cout<<"number:"<<number<<"\n";
	  if(number==1)
			  //||number==2||number==3||number==4)//收到来自簇成员的回复包，将簇成员放入队列中
	  {
			 if(destination == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
			{
				 std::cout<<"**********"<<std::endl;
				 int member=ash.GetSAddr().GetAsInt();
				 m_device->PushMember(member);
				 std::cout<<AquaSimAddress::ConvertFrom(m_device->GetAddress())<<"   menber    "<<"  "<<member<<"       "<<ash.GetSAddr()<<std::endl;
				 std::cout<<"**********"<<std::endl;
				return true;
			}
	  }
	  else//普通结点收到来自簇头关于自己位置的广播包，普通结点将其放入自己存储的队列中
	  {
		//  std::cout<<"test\n";
		  if (dst == AquaSimAddress::GetBroadcast())
		  {
				uint8_t* m_Buffer;
				m_Buffer=new uint8_t[65536];
				pkt->CopyData(m_Buffer,pkt->GetSize());
				std::string first="";
				std::string two="";
				for(int i=0;i<3;i++)
				{
					first=first+char(*m_Buffer);
					m_Buffer++;
				}
				for(int i=0;i<3;i++)
				{
					two=two+char(*m_Buffer);
					m_Buffer++;
				}

				int position_x=atoi(first.c_str());
				int position_y=atoi(two.c_str());

				Position a;
				a.x=position_x;
				a.y=position_y;
				a.num=AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt();
				pos.push_back(a);
			//	std::cout<<"add position\n";
				//delete []m_Buffer;
			  return true;
		  }
	  }



		//get a packet from modem, remove the sync hdr from txtime first
		//cmh->txtime() -= getSyncHdrLen();





	//	printf("underwaterAquaSimBroadcastMac: this is neither broadcast nor my packet, just drop it\n");
		pkt=0;
		return true;
  }
  else if(ptag.GetPacketType()==AquaSimPtTag::PT_MCMAC_Broadcast)
  {
	  int SA=AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt();
	  if(SA==m_device->GetClusterHead())
	  {
		  		  if(PktQ.size()>0)
		  		  {
		  			  sendRTS();//判断是不是自己的簇头，在发送RTS
		  		  }
	  }

  }
  else if(ptag.GetPacketType()==AquaSimPtTag::PT_MCMAC_RTS)
  {
	  int DA=AquaSimAddress::ConvertFrom(mach.GetDA()).GetAsInt();
	  if(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()==DA)
	  {
		  			uint8_t* m_Buffer;
		  			m_Buffer=new uint8_t[65536];
		  			pkt->CopyData(m_Buffer,pkt->GetSize());
		  			std::cout<<"@@@@@@@"<<std::endl;
		  			for(int i=0;i<=30;i++)
		  			{
		  				std::cout<<m_Buffer[i];
		  			}
		  			std::cout<<std::endl;
		  			//int start=0;
		  			int end=0;
		  			node a;
		  			a.node_num=AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt();
		  			for(int i=0;i<4;i++)
		  			{
		  				std::string content="";
		  				//end=start;
		  				while(m_Buffer[end]!='#')
		  				{
		  					content+=m_Buffer[end];
		  					end++;
		  				}
		  				end++;
		  				if(i==0)
		  				{
		  					a.size=atoi(content.c_str());
		  					if(a.size>Max_size) Max_size=a.size;
		  					else if(a.size<Min_size) Min_size=a.size;
		  				}
		  				else if(i==1)
		  				{
		  					a.speed=atof(content.c_str());
		  					if(a.speed>Max_speed) Max_speed=a.speed;
		  					else if(a.speed<Min_speed) Min_speed=a.speed;

		  				}
		  				else if(i==2)
		  				{
		  					a.distance=atof(content.c_str());
		  					if(a.distance>Max_distance) Max_distance=a.distance;
		  					else if(a.distance<Min_distance) Min_distance=a.distance;
		  				}

		  				else
		  				{
		  					a.rank=atof(content.c_str());
		  				}
	                 }
		  						std::cout<<"I get RTS "<<m_device->GetAddress()<<"  "<<a.size<<" "<<a.speed<<" "<<a.distance<<" "<<a.rank<<std::endl;
		  						Q.push_back(a);
	  }
  }
  else if(ptag.GetPacketType()==AquaSimPtTag::PT_MCMAC_G_Channel)
   {
 	  if(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()==1)
 			  //||AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()==2||AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()==3||AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()==4)
 	  {
 		  return false;
 	  }
 	  int SA=AquaSimAddress::ConvertFrom(ash.GetSAddr()).GetAsInt();
 	  //std::cout<<"PT_MCMAC_G_Channel"<<" "<<DA<<"   "<<AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()<<std::endl;
 		uint8_t* m_Buffer;
 		m_Buffer=new uint8_t[65536];
 		pkt->CopyData(m_Buffer,pkt->GetSize());
 		//std::cout<<"I am "<<AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()<<" get packet from "<<AquaSimAddress::ConvertFrom(ash.GetSAddr()).GetAsInt()<<" ash DA "<<AquaSimAddress::ConvertFrom(ash.GetDAddr()).GetAsInt()<<std::endl;
 //		std::cout<<"mac SA "<<AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt()<<" mac DA "<<AquaSimAddress::ConvertFrom(mach.GetDA()).GetAsInt()<<std::endl;
 		for(unsigned int i=0;i<pkt->GetSize();i++)
 		{
 			std::cout<<*(m_Buffer+i);
 		}
 		std::cout<<"\n";
 	  if(m_device->GetClusterHead()==SA){

 	 //  std::cout<<"number getchannel:"<<AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()<<"\n";

 		int end=0;
 		int number=0;
 		std::string content1="";

 		while(m_Buffer[end]!='#')
 		{
 		content1+=m_Buffer[end];
 		end++;
 		}
		number=atoi(content1.c_str());
 	//	std::cout<<"total number:"<<number<<"\n";
 		end++;
 		for(int k=0;k<number;k++)
 		{
 			std::string content2="";
 			std::string content3="";
 			std::string content4="";
 			std::string content5="";
 			while(m_Buffer[end]!='#')
 			{
 			content2+=m_Buffer[end];
 			end++;
 		    }
 			end++;
 		//	std::cout<<"content2:"<<content2<<"\n";
 			while(m_Buffer[end]!='#'){
 			  content5+=m_Buffer[end];
 			  end++;

 			}
 			end++;
 		//	std::cout<<"content5:"<<content5<<"\n";
 			int tem=atoi(content2.c_str());
 			if(tem==AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt())
 			{
 				SendDataPacket();
 			}
 //				end++;
 //				while(m_Buffer[end]!='*')
 //				{
 //					content3+=m_Buffer[end];
 //					end++;
 //				}
 //				int first=atoi(content3.c_str());
 //				std::cout<<"first: "<<first<<std::endl;
 //				end++;
 //				while(m_Buffer[end]!='#')
 //				{
 //					content4+=m_Buffer[end];
 //					end++;
 //				}
 //				int last=atoi(content4.c_str());
 //				std::cout<<"last: "<<last<<std::endl;
 //				end++;
 //				SendDataPacket();
 //
 //
 //			}
 //			while(m_Buffer[end]!='#')
 //			{
 //			end++;
 //			}
 //			end++;

 		}
 //		node a;
 //		a.node_num=AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt();
 //        std::string content1="";
 //        std::string content2="";
 //			while(m_Buffer[end]!='#')
 //			{
 //				content1+=m_Buffer[end];
 //				end++;
 //			}
 //			end++;
 //			std::cout<<"start: "<<content1<<std::endl;
 //			while(m_Buffer[end]!='#')
 //			{
 //				content2+=m_Buffer[end];
 //				end++;
 //			}
 //			end++;
 //			std::cout<<"end: "<<content2<<std::endl;
 //
 	  }

   }
  else if( ptag.GetPacketType()==AquaSimPtTag::PT_MCMAC_DATA)
  {
	  int DA=AquaSimAddress::ConvertFrom(ash.GetDAddr()).GetAsInt();
	  int SA=AquaSimAddress::ConvertFrom(ash.GetSAddr()).GetAsInt();
	  if(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()==DA)
	  {
        std::cout<<m_device->GetAddress()<<" Get the data packet! from node "<<SA<<" packetsize:"<<pkt->GetSize()<<std::endl;
        std::ofstream ofile;               //定义输出文件
        ofile.open("result1.txt",std::ios::app);     //作为输出文件打开
        ofile<<m_device->GetAddress()<<" Get the data packet! from node "<<SA<<" packetsize "<<pkt->GetSize()<<std::endl;   //标题写入文件
        ofile.close();
	  }
  }


  return true;

}
//向簇头发送RTS
void
AquaSimContrast::sendRTS()
{
	  AquaSimHeader ash;
	  MacHeader mach;
	  AquaSimPtTag ptag;
	  ash.SetDAddr(AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(m_device->GetClusterHead())));
	  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDA(AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(m_device->GetClusterHead())));//i do not know the next hop in this mac
	  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
	  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_RTS);
	  Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
	  Ptr<Packet> p=PktQ.front();
	 unsigned int packetsize=p->GetSize();
      Vector speed=model->GetVelocity();
      double speed_resultant=sqrt(speed.x*speed.x+speed.y*speed.y);
	    int rank= rand()%5+1;

		std::string content;
		content=std::to_string(packetsize)+"#"+std::to_string(speed_resultant)+"#"+std::to_string(distance)+"#"+std::to_string(rank)+"#";
        std::cout<<m_device->GetAddress()<<"    "<<content<<std::endl;
		std::ostringstream interest;
		interest <<content<< '\0';
		Ptr<Packet> packet = Create<Packet>((uint8_t*)interest.str().c_str(), interest.str().length());



	  packet->AddPacketTag(ptag);
	  packet->AddHeader(ash);
	  packet->AddHeader(mach);
	  SendPkt(packet);

}


void
AquaSimContrast::DropPacket(Ptr<Packet> pkt)
{
  //this is not necessary... only kept for current legacy issues
  pkt=0;
  return;
}

//簇头结点向普通结点发送自己的位置
bool
AquaSimContrast::ClusterSendPos()
{
    std::cout<<"AquaSimBroadcastClusterMac::ClusterSendPos()"<<std::endl;
	 Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
	 int x=model->GetPosition().x;
	 int y=model->GetPosition().y;
	  MacHeader mach;
	  AquaSimHeader ash;
	  AquaSimPtTag ptag;
	  mach.SetDA(AquaSimAddress::GetBroadcast());
	  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
	  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));//no use,randomly set it
	  ash.SetDAddr(AquaSimAddress::GetBroadcast());
	  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_CLUSTER);

     int n_zero=3;

     std::string content_x = std::string(n_zero - std::to_string(x).length(), '0') + std::to_string(x);
     std::string content_y = std::string(n_zero - std::to_string(y).length(), '0') + std::to_string(y);

	std::string content=content_x+content_y;
	std::cout<<m_device->GetAddress()<<"    "<<content<<std::endl;
	std::ostringstream interest;
	interest <<content<< '\0';


	Ptr<Packet> packet = Create<Packet>((uint8_t*)interest.str().c_str(), interest.str().length());


ash.SetTxTime(Seconds(1));//change--------------
packet->AddPacketTag(ptag);
packet->AddHeader(ash);
packet->AddHeader(mach);
packet->Print(std::cout);
SendPkt(packet);
return true;
}






/*
this program is used to handle the transmitted packet,
it should be virtual function, different class may have
different versions.
*/
bool
AquaSimContrast::TxProcess(Ptr<Packet> pkt)
{
	 PktQ.push(pkt);

	return true;

}


//计算评估值的函数
void AquaSimContrast::CalTimerAssessmentValue()
{
	std::cout<<"TTTTTTTT"<<std::endl;


		if(Q.size()!=0){
		std::vector<double> v;
		std::vector<double> d;
		std::vector<int>    s;

		for(unsigned int i=0;i<Q.size();i++){
			v.push_back(Q[i].speed);
			d.push_back(Q[i].distance);
			s.push_back(Q[i].size);
		}

		double vmax=*max_element(v.begin(), v.end());
		double vmin=*min_element(v.begin(), v.end());

		double dmax=*max_element(d.begin(), d.end());
		double dmin=*min_element(d.begin(), d.end());

		double smax=*max_element(s.begin(), s.end());
		double smin=*min_element(s.begin(), s.end());

		std::cout<<"v:"<<vmax-vmin<<"\n";
		std::cout<<"d:"<<dmax-dmin<<"\n";
		std::cout<<"s:"<<smax-smin<<"\n";

	    for(unsigned int i=0;i<Q.size();i++)
	    {
	    	Q[i].assessment_value=pow(((Q[i].speed-vmin)/(vmax-vmin)*0.5+(Q[i].distance-dmin)/(dmax-dmin)*(1.0/3)+(Q[i].size-smin)/(smax-smin)*(1.0/6)),Q[i].rank);
	    }
	    sort(Q.begin(), Q.end(), comp);
	    //计算等分点，扩频
	    std::ofstream ofile;               //定义输出文件
	    ofile.open("result1.txt",std::ios::app);     //作为输出文件打开
	    ofile<<"Q number "<<Q.size()<<std::endl;   //标题写入文件
	    ofile.close();



	    std::cout<<"Q number "<<Q.size()<<"\n";
	    if(Q.size()<=6)

	    {
	    //	std::cout<<"Q<6"<<"\n";
	    	Q[0].squence=128;
	    	for(unsigned int i=1;i<=2&&i<Q.size();i++)
	    		Q[i].squence=64;
	    	for(unsigned int i=3;i<Q.size();i++)
	    		Q[i].squence=32;
	    }
	    else
	    {
	    	unsigned int equinoctial=Q.size()/6;
	    	for(unsigned int i=0;i<equinoctial&&i<Q.size();i++)
	    	{
	    		Q[i].squence=128;
	    	}
	    	for(unsigned int i=equinoctial;i<equinoctial*3&&i<Q.size();i++)
	    	{
	    		Q[i].squence=64;
	    	}

	    	for(unsigned int i=equinoctial*3;i<Q.size();i++)
	    	{
	    		Q[i].squence=32;
	    	}
	    }
	//   for(unsigned int i=0;i<Q.size();i++)
	//    {
	//    	std::cout<<Q[i].assessment_value<<" "<<Q[i].squence<<" "<<Q[i].rank<<" "<<Q[i].node_num<<std::endl;
	//    }




	   S_Channel();
 }

		else{
				std::cout<<"no communication requests\n";
				 Simulator::Schedule(Seconds(15), &AquaSimContrast::CalTimerExpireOrder, this);

			}
}


//簇头向其成员发送广播包，通知其可以发送rts
void AquaSimContrast::CalTimerExpireOrder()
{
	std::cout<<"*******broadcast time******"<<ns3::Simulator::Now().GetSeconds()<<"\n";
	sendToMembers();
	Simulator::Schedule(Seconds(15), &AquaSimContrast::CalTimerAssessmentValue, this);//簇头收到rts包后计算评估值
	//Simulator::Schedule(Seconds(28), &AquaSimContrast::S_Channel, this);//选择信道
	//S_Channel();

}


//通知成员
bool AquaSimContrast::sendToMembers()
{
	std::string content="!!!!";
	std::ostringstream interest;
	interest <<content<< '\0';
	// std::cout << "-----std::ostringstream interest-------"<<(uint8_t*) interest.str().c_str()<<"---"<<interest.str().length()<<"\n";
	Ptr<Packet> packet = Create<Packet>((uint8_t*)interest.str().c_str(), interest.str().length());
	  AquaSimHeader ash;
	  MacHeader mach;
	  AquaSimPtTag ptag;
	  ash.SetDAddr(AquaSimAddress::GetBroadcast());
	  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDA(AquaSimAddress::GetBroadcast());//i do not know the next hop in this mac
	  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
	  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_Broadcast);


	  if( m_packetSize != 0 )
	    ash.SetSize(m_packetSize);
	  else
	    ash.SetSize(m_packetHeaderSize + ash.GetSize());

	  ash.SetTxTime(Seconds(1));//Write at random

	  packet->AddPacketTag(ptag);

      packet->AddHeader(ash);
      packet->AddHeader(mach);
      SendPkt(packet);

	  return true;
}

//定时被触发计算距离，确定簇头结点
void AquaSimContrast::CalTimerExpire()
{
	int tem_distance=21474836;
	Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
	 int x=model->GetPosition().x;
	 int y=model->GetPosition().y;
	// Position * nodes=m_device->GetNodePosition();



	 for(unsigned int i=0;i<pos.size();i++)
	 {
		 int tem=sqrt(pow(x-pos[i].x,2)+pow(y-pos[i].y,2));
		 std::cout<<tem<<"\n";
		 if(tem<tem_distance)
		 {
			 tem_distance=tem;
			 distance=tem_distance;
			 m_device->SetClusterHead(pos[i].num);
		 }
	 }
	 std::cout<<"++++++++++++++++"<<std::endl;
	 std::cout<<AquaSimAddress::ConvertFrom(m_device->GetAddress())<<"          "<<"cluster_header "<<m_device->GetClusterHead()<<std::endl;
	 std::cout<<"++++++++++++++++"<<std::endl;
	 SendReplyMess(m_device->GetClusterHead());
}
//普通结点确定簇头后，发送给簇头的恢复信息
bool AquaSimContrast::SendReplyMess(int cluster_header)
{
	std::string content="Reply";
	std::ostringstream interest;
	interest <<content<< '\0';
	// std::cout << "-----std::ostringstream interest-------"<<(uint8_t*) interest.str().c_str()<<"---"<<interest.str().length()<<"\n";
	Ptr<Packet> packet = Create<Packet>((uint8_t*)interest.str().c_str(), interest.str().length());
	  AquaSimHeader ash;
	  MacHeader mach;
	  AquaSimPtTag ptag;
	  ash.SetDAddr(AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(m_device->GetClusterHead())));
	  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDA(AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(m_device->GetClusterHead())));//i do not know the next hop in this mac
	  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
	  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_CLUSTER);


	  if( m_packetSize != 0 )
	    ash.SetSize(m_packetSize);
	  else
	    ash.SetSize(m_packetHeaderSize + ash.GetSize());

	  ash.SetTxTime(Seconds(1));//Write at random
	   packet->AddPacketTag(ptag);
      packet->AddHeader(ash);
      packet->AddHeader(mach);
      SendPkt(packet);

	  return true;



}
void
AquaSimContrast::BackoffHandler(Ptr<Packet> pkt)
{
  m_backoffCounter++;
  if (m_backoffCounter<BC_MAXIMUMCOUNTER)
    TxProcess(pkt);
  else
    {
      NS_LOG_INFO("BackoffHandler: too many backoffs");
      m_backoffCounter=0;
      DropPacket(pkt);
    }
}

void AquaSimContrast::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimMac::DoDispose();
}
