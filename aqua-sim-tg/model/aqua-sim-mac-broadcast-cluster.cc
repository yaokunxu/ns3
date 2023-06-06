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

#include "aqua-sim-mac-broadcast-cluster.h"
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

NS_LOG_COMPONENT_DEFINE("AquaSimBroadcastClusterMac");
NS_OBJECT_ENSURE_REGISTERED(AquaSimBroadcastClusterMac);


/* ======================================================================
Broadcast MAC for  underwater sensor
====================================================================== */

AquaSimBroadcastClusterMac::AquaSimBroadcastClusterMac()
{
  m_backoffCounter=0;
  m_rand = CreateObject<UniformRandomVariable> ();

  Simulator::Schedule(Seconds(1), &AquaSimBroadcastClusterMac::CalTimerExpireOrder1, this);
}
//簇头节点发起分簇
void
AquaSimBroadcastClusterMac::CalTimerExpireOrder1()
{
	if(m_device!=NULL)
		{
		  int number=AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt();
		//  std::cout<<"number:"<<number<<"\n";
		  if(number==1)
				//  ||number==2||number==3||number==4
		  {
			  ClusterSendPos();
			  Simulator::Schedule(Seconds(15), &AquaSimBroadcastClusterMac::CalTimerExpireOrder, this);//15s后由簇头结点发送通知包，通知其成员可以发送rts
		  }
		  else
		  {//成员节点计算谁是簇头
			  Simulator::Schedule(Seconds(3), &AquaSimBroadcastClusterMac::CalTimerExpire, this);//3s后触发计算簇头的函数
		  }
		}

}
bool cmp(int x,int y)
{
	return x > y;
}
void Swap(std::vector<int> &a,int A,int B)
{
	int c=a[A];
	a[A]=a[B];
	a[B]=c;
}
int Sum(double **P,int row,int start,int end)
{
	int sum=0;
	for(int i=start;i<=end;i++)
	{
		sum+=P[row][i];
	}
	return sum;
}
double find_max(double *p,int &pos,int length)
{
	double a=p[0];
	for(int i=0;i<length;i++)
	{
		if(p[i]>a)
		{
			a=p[i];
			pos=i;
		}
	}
	return a;
}


double find_min(double *p,int &pos,int length)
{
	double a=p[0];
	for(int i=0;i<length;i++)
	{
		if(p[i]<a)
		{
			a=p[i];
			pos=i;
		}
	}
	return a;
}

//bool AquaSimBroadcastClusterMac::G_channel(int start,int end,int member)
//{
//	  AquaSimHeader ash;
//	  MacHeader mach;
//	  AquaSimPtTag ptag;
//	  //AquaSimAddress::GetBroadcast()
//	  //ash.SetDAddr(AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(member)));
//	  ash.SetDAddr(AquaSimAddress::GetBroadcast());
//	  ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
//	  mach.SetDA(AquaSimAddress::GetBroadcast());//i do not know the next hop in this mac
//	  mach.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
//	  mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
//	  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_G_Channel);
//
//		std::string content;
//		content=std::to_string(start)+"#"+std::to_string(end)+"#";
//        std::cout<<m_device->GetAddress()<<"    "<<content<<"  "<<member<<std::endl;
//		std::ostringstream interest;
//		interest <<content<< '\0';
//		Ptr<Packet> packet = Create<Packet>((uint8_t*)interest.str().c_str(), interest.str().length());
//
//
//
//	  packet->AddPacketTag(ptag);
//	  packet->AddHeader(mach);
//	  packet->AddHeader(ash);
//	  std::cout<<"before packet!"<<std::endl;
//	  SendPkt(packet);
//	  std::cout<<"after packet!"<<std::endl;
//	  return true;
//}

//簇头回复ACK
bool AquaSimBroadcastClusterMac::G_channel(std::string content)
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
	  Simulator::Schedule(Seconds(delay+trans), &AquaSimBroadcastClusterMac::CalTimerExpireOrder, this);
	  return true;
}

//蚁群选择信道
void
AquaSimBroadcastClusterMac::S_Channel()
{
int cal_squence=0;
int x1=0;//本次通信的结点个数
int x=0;//总结点数
int ant=15;//设置蚂蚁数
int NC_MAX=20;//设置最大迭代次数
int Alpha=2.5;  //Alpha 表征信息素重要程度的参数
int Beta=6;  //Beta 表征启发式因子重要程度的参数
double Rho=0.4; //Rho 信息素蒸发系数
double Q_value=500;
double SNR[15][512];
//double Tau[15][512];
int Tabu[ant][15];
int Randpos[15][15];
int R_best[NC_MAX][15];

x=Q.size();
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
//创建Eta,W,SNR数组
double ** Eta=new double *[x];
for(int i=0;i<x;i++)
{
	Eta[i]=new double [512];
}
double ** W=new double *[x];
for(int i=0;i<x;i++)
{
	W[i]=new double [512];
}

//
////double ** SNR=new double *[x];
////for(int i=0;i<x;i++)
////{
////	SNR[i]=new double [512];
////}
////生成随机数
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
	//	std::cout<<W[i][j]<<" \n";
	//	ber[i][j]=r/1000.0;
	}
}
////根据rank值设置Eta,这里根据的不是结点的编号，而是根据结点在Q中的顺序
for(int i=0;i<x1;i++)
{
	if(Q[i].rank>1)
	{
		for(int k=0;k<512;k++)
		{
			Eta[i][k]=W[i][k];
		}
	}
	else
	{
		for(int k=0;k<512;k++)
		{
			Eta[i][k]=SNR[i][k];
		}
	}
}
//设置相应数组及分配空间
double ** Tau=new double *[x];//信息素矩阵
for(int i=0;i<x;i++)
{
	Tau[i]=new double [512];
}
double ** Visit=new double *[ant];//记录子载波是否已分配
for(int i=0;i<ant;i++)
{
	Visit[i]=new double [512];
}

double * XL_best=new double [NC_MAX];
double * XL_ave=new double [NC_MAX];
//double * B_best=new double [NC_MAX];
//double * L_best=new double [NC_MAX];
double * L_ave=new double [NC_MAX];
std::vector<int> initial_order;
int NC=1;//设置迭代次数
int s128e=0;
int s64e=0;
int s32e=0;
for(int i=0;i<x1;i++)

{
	std::cout<<Q[i].squence<<"\n";
	if(Q[i].squence==128)
	{
		s128e++;
	}

	else if(Q[i].squence==64)
	{
		s64e++;
	}

	else
	{
		s32e++;
	}
	initial_order.push_back(i);
}
//std::cout<<s128e<<"  "<<s64e<<"   "<<s32e<<std::endl;





while(NC<NC_MAX)
{
//	//第二步：将m只蚂蚁放到n个城市上  m只蚂蚁访问顺序
//	//std::cout<<"NC  "<<NC<<std::endl;
	std::vector<int > v1(initial_order);
	srand(unsigned(time(NULL)));
	for(int i=0;i<ant;i++)
	{

//	std::vector<int>::iterator it=v1.begin();
	random_shuffle(v1.begin(), v1.begin()+s128e);
	random_shuffle(v1.begin()+s128e, v1.begin()+s128e+s64e);
	random_shuffle(v1.begin()+s128e+s64e, v1.begin()+s128e+s64e+s32e);
	for(int j=0;j<x1;j++)
	{
		Randpos[i][j]=v1[j];
	}
	}
////	std::cout<<"Randpos"<<std::endl;
//
	//重置子载波
	for(int i=0;i<ant;i++)
	{
		for(int j=0;j<512;j++)
		{
			Visit[i][j]=0;
		}
	}
//	//第三步：m只蚂蚁按概率函数选择信道，完成各自的周游

	for(int i=0;i<ant;i++)
	{
//		std::cout<<"ant: "<<i<<std::endl;
		for(int j=0;j<x1;j++)
		{
			double Psum=0;
			int code=Randpos[i][j];
			int to_visit;
			int J=512/Q[code].squence;
			double *P=new double[J];
			double *P_tem=new double[J];
			for(int d=0;d<J;d++)
			{
				int a=d*Q[code].squence;
				int b=(d+1)*Q[code].squence-1;
            int flag=Sum(Visit,i,a,b);
            if(flag>0)
            {
            	P[d]=0;
            	Psum=Psum+P[d];
            }
            else
            {
            	double tau=Tau[code][(d+1)*Q[code].squence-1];
            	if(tau==0)
            		tau=1;
            	double eta;
            	if(Q[code].rank>1)
            	{
            		 eta=1.0/(Sum(Eta,code,a,b)/Q[code].squence);
            	}
            	else
            	{
            		 eta=(Sum(Eta,code,a,b)/Q[code].squence)/Q[code].rank;
            	}
            	P[d]=pow(tau,Alpha)*pow(eta,Beta);
            	Psum=Psum+P[d];

            }
			}
			srand(unsigned(time(NULL)));
//			std::cout<<"Psum  "<<Psum<<std::endl;
			 for(int a=0;a<J;a++)
			 {
				 P_tem[a]=P[a]/Psum;
			 }
             int ran=rand();
             int R=log(NC)/log(10);
             if(ran<R)
             {
            	 int value=0;
            	 int pos=-1;
            	 for(int m=0;m<J;m++)
            	 {
            		 if(P[m]>value)
            		 {
            			 value=P[m];
            			 pos=m;
            		 }
            	 }
            	 to_visit=pos;
//            	 std::cout<<"to_visit  "<<to_visit<<std::endl;
//            	 std::cout<<"J:"<<J<<std::endl;
             }

             else
             {
//            	 std::cout<<"ran>=R"<<std::endl;
            	 int value=0;
            	 int pos=-1;
            	 for(int m=0;m<J;m++)
            	 {
            		 if(P_tem[m]>value)
            		 {
            			 value=P_tem[m];
            			 pos=m;
            		 }
            	 }
            	 to_visit=pos;//to_visit选择的是具体去到哪个信道
            //	 std::cout<<"to_visit  "<<to_visit<<std::endl;
             }
             for(int k=to_visit*Q[code].squence;k<(to_visit+1)*Q[code].squence-1;k++)
             {
            	 Visit[i][k]=1;
             }
             Tabu[i][code]=to_visit;

		}
	}
//
	 if(NC>=2)
	 {
		 for(int u=0;u<x;u++)
		 {
			 Tabu[1][u]=R_best[NC-1][u];
		 }
	 }
	 double * L=new double[ant];
	 double * XL=new double[ant];
	 for(int i=0;i<ant;i++){
		 L[i]=0;
		 XL[i]=0;
	 }
//	 double * Biterr=new double[ant];
	 for(int u=0;u<ant;u++)//每只蚂蚁的选择
	 {
		 for(int r=0;r<x1;r++)//每只蚂蚁对每个节点的选择计算耗能和
		 {
			   int num=Tabu[u][r];
				int a=num*Q[r].squence;
				int b=(num+1)*Q[r].squence-1;
			   double w=Sum(W,r,a,b)/Q[r].squence;
			   std::cout<<"w:"<<w<<" ";
			    L[u]+=w;
			    XL[u]+=Q[r].size/w;
			 // b=sum(ber(code,p:q))/len(code)   这里的ber代表什么
	       //     Biterr[u]=Biterr[u]+Sum(ber,r,a,b)/Q[r].squence;


		 }
		//  std::cout<<"L[u]:"<<L[u]<<" u:"<<u<<"\n";
	 }
	 int pos=0;
	 XL_best[NC]=find_max(XL,pos,ant);

	// std::cout<<pos<<std::endl;
     for(int w=0;w<x;w++)
     {
    	// std::cout<<w<<"\n";
    	 R_best[NC][w]=Tabu[pos][w];
     }

	// B_best[NC]=Biterr[pos];
	 int sum_tem=0;
	 int sum_w=0;
	 for(int w=0;w<ant;w++)
	 {
		 sum_tem+=XL[w];
		 sum_w+=L[w];
	 }
	 XL_ave[NC]=sum_tem/ant;
	 L_ave[NC]=sum_w/ant;
	 NC++;
//     //第五步：更新信息素
     double ** Delta_Tau=new double *[x];//信息素矩阵
     for(int i=0;i<x;i++)
     {
    	 Delta_Tau[i]=new double [512];
     }
     for(int u=0;u<ant;u++)
     {
    	 for(int h=0;h<x1;h++)
    	 {
    		 int num=Tabu[u][h];
			 int a=num*Q[h].squence;
			 int b=(num+1)*Q[h].squence-1;
			 double w=Sum(W,h,a,b)/Q[h].squence;
			 double c=Q[h].size/w;
			 for(int f=a;f<=b;f++)
			 {
				 Delta_Tau[h][f]=Delta_Tau[h][f]+Q_value*c;
			 }

    	 }
     }
     for(int e=0;e<x;e++)
     {
    	 for(int r=0;r<512;r++)
    	 {
    		 //                   (15,512)
    		 Tau[e][r]=(1-Rho)*Tau[e][r]+Delta_Tau[e][r];//（x,512）


    	 }
     }
//
//     //第六步：禁忌表清零
     for(int e=0;e<ant;e++)
     {
    	 for(int f=0;f<x;f++)
    		 Tabu[e][f]=0;
     }

//
}

int pos=0;
double max_value=find_max(XL_ave,pos,ant);
//double max_w=find_min(L_ave,pos,ant);
std::cout<<"XL value: "<<max_value<<std::endl;
std::cout<<"L value: "<<L_ave[pos]<<std::endl;
//std::cout<<"pos:"<<pos<<"\n";
//double max_w=L_ave[pos];
//std::ofstream ofile;               //定义输出文件
//ofile.open("result.txt",std::ios::app);     //作为输出文件打开
//ofile<<"x1 number "<<x1<<std::endl;   //标题写入文件
//ofile<<"xl: "<<max_value<<std::endl;
//ofile<<"wsum: "<<max_w<<std::endl;
//ofile.close();



//std::cout<<"The best route:  ";
std::string content="";
content=std::to_string(x1)+"#";
for(int i=0;i<x;i++)
{

	//std::cout<<R_best[pos][i]<<" I am  "<<m_device->GetAddress()<<"   to   "<<Q[i].node_num<<std::endl;
	content=content+std::to_string(Q[i].node_num)+"#"+std::to_string(R_best[pos][i]*Q[i].squence)+"*"+std::to_string((R_best[pos][i]+1)*Q[i].squence-1)+"#";
}
////G_channel(R_best[pos][i]*Q[i].squence,(R_best[pos][i]+1)*Q[i].squence-1,Q[i].node_num);
////   sort(Q.begin(), Q.end(), dis);
////   double delay=Q[0].distance/1500+Q[0].size/667+0.8+1;
////   Simulator::Schedule(Seconds(2*delay), &AquaSimBroadcastClusterMac::CalTimerExpireOrder, this);
   G_channel(content);

for (int i = 0;i < x;i++)
{
	delete[]Eta[i];
}
delete[]Eta;


for (int i = 0;i < x;i++)
{
	delete[]W[i];
}
delete[]W;

delete [] XL_best;
delete [] XL_ave;

delete [] L_ave;
std::vector <node>().swap(Q);

}
TypeId
AquaSimBroadcastClusterMac::GetTypeId()
{
  static TypeId tid = TypeId("ns3::AquaSimBroadcastClusterMac")
      .SetParent<AquaSimMac>()
      .AddConstructor<AquaSimBroadcastClusterMac>()
      .AddAttribute("PacketHeaderSize", "Size of packet header",
        IntegerValue(0),
        MakeIntegerAccessor (&AquaSimBroadcastClusterMac::m_packetHeaderSize),
        MakeIntegerChecker<int> ())
      .AddAttribute("PacketSize", "Size of packet",
	IntegerValue(0),
	MakeIntegerAccessor (&AquaSimBroadcastClusterMac::m_packetSize),
	MakeIntegerChecker<int> ())
    ;
  return tid;
}



void
AquaSimBroadcastClusterMac::SendPkt(Ptr<Packet> pkt)
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
	  	   pkt->AddHeader(mach);
	      //Phy()->SetPhyStatus(PHY_SEND);
         SendDown(pkt);
	//      std::cout<<"send result "<<<<std::endl;
	      m_backoffCounter=0;
	      return;
	  case RECV:
	    {
	      double backoff=m_rand->GetValue()*BC_BACKOFF;
	      NS_LOG_DEBUG("BACKOFF time:" << backoff << " on node:" << m_device->GetAddress() << "\n");
	      //pkt->AddHeader(mach);
	      pkt->AddHeader(ash);
	      Simulator::Schedule(Seconds(backoff),&AquaSimBroadcastClusterMac::BackoffHandler,this,pkt);
	      return;
	    }

	  case SEND:
	    {
	      double backoff=m_rand->GetValue()*BC_BACKOFF;
	      NS_LOG_DEBUG("BACKOFF time:" << backoff << " on node:" << m_device->GetAddress() << "\n");
	      //pkt->AddHeader(mach);
	      pkt->AddHeader(ash);
	      Simulator::Schedule(Seconds(backoff),&AquaSimBroadcastClusterMac::BackoffHandler,this,pkt);
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



bool AquaSimBroadcastClusterMac::SendDataPacket()
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
AquaSimBroadcastClusterMac::AssignStreams (int64_t stream)
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
AquaSimBroadcastClusterMac::RecvProcess (Ptr<Packet> pkt)
{

  NS_LOG_FUNCTION(this);
//  std::cout << "\nBMac @RecvProcess check:\n";
//  pkt->Print(std::cout);
//  std::cout << "\n";

  AquaSimHeader ash;
  MacHeader mach;
  AquaSimPtTag ptag;
//  ptag.SetPacketType(AquaSimPtTag::PT_MCMAC_CLUSTER);
  pkt->RemoveHeader(mach);
  pkt->RemoveHeader(ash);

  pkt->RemovePacketTag(ptag);
  if(ptag.GetPacketType()==AquaSimPtTag::PT_MCMAC_CLUSTER)
  {
	 // std::cout<<"PT_MCMAC_CLUSTER"<<"\n";
	  AquaSimAddress dst = mach.GetDA();
	  AquaSimAddress destination = ash.GetDAddr();
		if (ash.GetErrorFlag())
		{
			NS_LOG_DEBUG("BroadcastMac:RecvProcess: received corrupt packet.");
			pkt=0;
			return false;
		}
      int number=AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt();
	  if(number==1)
			  //||number==2||number==3||number==4)//收到来自簇成员的回复包，将簇成员放入队列中
	  {
			 if(destination == AquaSimAddress::ConvertFrom(m_device->GetAddress()))
			{
				// std::cout<<"**********"<<std::endl;
				 int member=ash.GetSAddr().GetAsInt();
				 m_device->PushMember(member);
				// std::cout<<AquaSimAddress::ConvertFrom(m_device->GetAddress())<<"   menber    "<<"  "<<member<<"       "<<ash.GetSAddr()<<std::endl;
				// std::cout<<"**********"<<std::endl;
				// m_device->SetValuesCompare(m_device->GetMember().size());//???SetValuesCompare()这个函数用来干什么？？
//				if(find(m_device->GetMember().begin(), m_device->GetMember().end(), member)==m_device->GetMember().end())
//				{
//					std::cout<<"Push it!"<<std::endl;
//
//
//				}
				return true;
			}
	  }
	  else//普通结点收到来自簇头关于自己位置的广播包，普通结点将其放入自己存储的队列中
	  {
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

			//	m_device->SetNodePosition(AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt(),position_x,position_y);
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
	 // std::cout<<"PT_MCMAC_Broadcast"<<"\n";
	  int SA=AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt();
	  if(SA==m_device->GetClusterHead())//判断是不是自己的簇头，再发送RTS

	  {
		//  std::cout<<"Recv broadcast "<<AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()<<"\n";
		  if(!PktQ.empty())
		  		  {
		  			  sendRTS();
		  		  }
	  }

  }
  else if(ptag.GetPacketType()==AquaSimPtTag::PT_MCMAC_RTS)  //收到RTS
  {
	  int DA=AquaSimAddress::ConvertFrom(mach.GetDA()).GetAsInt();
	  if(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()==DA)
	  {

		  			uint8_t* m_Buffer;
		  			m_Buffer=new uint8_t[65536];
		  			pkt->CopyData(m_Buffer,pkt->GetSize());
//		  			std::cout<<"@@@@@@@"<<std::endl;
//		  			for(int i=0;i<=30;i++)
//		  			{
//		  				std::cout<<m_Buffer[i];
//		  			}
//		  			std::cout<<std::endl;
		  			//int start=0;
		  			int end=0;
		  			node a;
		  			a.node_num=AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt();
		  		//	std::cout<<"node num:"<<a.node_num<<"\n";

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
		  					//	std::cout<<"I get RTS "<<m_device->GetAddress()<<"  "<<a.size<<" "<<a.speed<<" "<<a.distance<<" "<<a.rank<<std::endl;
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

		  std::cout<<"number getchannel:"<<AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()<<"\n";
//			for(int i=0;i<60;i++)
//			{
//				std::cout<<*(m_Buffer+i);
//			}
//			std::cout<<"I am "<<AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()<<" get packet from "<<AquaSimAddress::ConvertFrom(ash.GetSAddr()).GetAsInt()<<" ash DA "<<AquaSimAddress::ConvertFrom(ash.GetDAddr()).GetAsInt()<<std::endl;
//		  std::cout<<"!!!!!!!"<<std::endl;
//
//		  std::cout<<"!!!!!!!"<<std::endl;
//		  std::cout<<"!!!!!!!"<<std::endl;
//		  std::cout<<"!!!!!!!"<<std::endl;
		  //std::cout<<"PT_MCMAC_G_Channel"<<" DA： "<<"   "<<AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt()<<"    SA   "<<AquaSimAddress::ConvertFrom(mach.GetSA()).GetAsInt()<<std::endl;

//		int start=0;
		int end=0;
		int number=0;
		std::string content1="";

		while(m_Buffer[end]!='#')
		{
		content1+=m_Buffer[end];
		end++;
		}
		number=atoi(content1.c_str());
		std::cout<<"total number:"<<number<<"\n";
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



	  }


//	 std::ofstream ofile;               //定义输出文件
//	 ofile.open("result.txt",std::ios::app);     //作为输出文件打开
//    ofile<<m_device->GetAddress()<<" Get the data packet! from node "<<SA<<" packetsize "<<pkt->GetSize()<<std::endl;   //标题写入文件
//    ofile.close();
  }


  return true;

}
//向簇头发送RTS
void
AquaSimBroadcastClusterMac::sendRTS()
{
	// std::cout<<"send RTS\n";
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
	    int rank= rand()%4+1;

		std::string content;
		content=std::to_string(packetsize)+"#"+std::to_string(speed_resultant)+"#"+std::to_string(distance)+"#"+std::to_string(rank)+"#";
      std::cout<<m_device->GetAddress()<<"    "<<content<<std::endl;
		std::ostringstream interest;
		interest <<content<< '\0';
	  Ptr<Packet> packet = Create<Packet>((uint8_t*)interest.str().c_str(), interest.str().length());
	  packet->AddPacketTag(ptag);
	  packet->AddHeader(ash);
	  packet->AddHeader(mach);
	//  packet->Print(std::cout);
	  SendPkt(packet);

}


void
AquaSimBroadcastClusterMac::DropPacket(Ptr<Packet> pkt)
{
  //this is not necessary... only kept for current legacy issues
  pkt=0;
  return;
}

//簇头结点向普通结点发送自己的位置
bool
AquaSimBroadcastClusterMac::ClusterSendPos()
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
	//  ash.SetDirection(AquaSimHeader::DOWN);
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

//std::cout<<"send pos cluster:\n";
//packet->Print(std::cout);
//std::cout<<"\n";

SendPkt(packet);
return true;
}






/*
this program is used to handle the transmitted packet,
it should be virtual function, different class may have
different versions.
*/
bool
AquaSimBroadcastClusterMac::TxProcess(Ptr<Packet> pkt)
{
	 PktQ.push(pkt);

	return true;

}


//计算评估值的函数
void AquaSimBroadcastClusterMac::CalTimerAssessmentValue()
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
//    std::ofstream ofile;               //定义输出文件
//    ofile.open("result.txt",std::ios::app);     //作为输出文件打开
//    ofile<<"Q number "<<Q.size()<<std::endl;   //标题写入文件
//    ofile.close();



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
		 Simulator::Schedule(Seconds(15), &AquaSimBroadcastClusterMac::CalTimerExpireOrder, this);

	}

}


//簇头向其成员发送广播包，通知其可以发送rts
void AquaSimBroadcastClusterMac::CalTimerExpireOrder()

{
	std::cout<<"*******broadcast time******"<<ns3::Simulator::Now().GetSeconds()<<"\n";
	sendToMembers();
	Simulator::Schedule(Seconds(15), &AquaSimBroadcastClusterMac::CalTimerAssessmentValue, this);//簇头收到rts包后计算评估值
//	Simulator::Schedule(Seconds(50), &AquaSimBroadcastClusterMac::S_Channel, this);//选择信道
	//S_Channel();



}


//通知成员
bool AquaSimBroadcastClusterMac::sendToMembers()
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
void AquaSimBroadcastClusterMac::CalTimerExpire()
{
	int tem_distance=21474836;
	Ptr<MobilityModel> model = m_device->GetNode()->GetObject<MobilityModel>();
	 int x=model->GetPosition().x;
	 int y=model->GetPosition().y;
	// Position  nodes=m_device->GetNodePosition();

	 for(unsigned int i=0;i<pos.size();i++)
	 {
		 int tem=sqrt(pow(x-pos[i].x,2)+pow(y-pos[i].y,2));
		// std::cout<<tem<<"  ";
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
bool AquaSimBroadcastClusterMac::SendReplyMess(int cluster_header)
{
	//std::cout<<"!!!!!!!!!!!!!!!SendReplyMess!!!!!!!!!!!!!\n";
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
AquaSimBroadcastClusterMac::BackoffHandler(Ptr<Packet> pkt)
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

void AquaSimBroadcastClusterMac::DoDispose()
{
  NS_LOG_FUNCTION(this);
  AquaSimMac::DoDispose();
}
