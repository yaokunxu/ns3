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

#include "aqua-sim-routing-dynamic.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-pt-tag.h"
//#include "ns3/ipv4-header.h"
#include "aqua-sim-trailer.h"//xj
#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/nstime.h"
#include "aqua-sim-header-mac.h"

#include "ns3/string.h"
#include "ns3/log.h"

#include <cstdio>
#include <iostream>
#include<fstream>
#include<sstream>
#include<string.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimDynamicRouting");
NS_OBJECT_ENSURE_REGISTERED(AquaSimDynamicRoutingTable);



Disconnected_Timer::Disconnected_Timer(){

	//std::cout<<"Disconnected_Timer::Disconnected_Timer()"<<std::endl;
}
Disconnected_Timer::~Disconnected_Timer(){

//	std::cout<<"Disconnected_Timer::~Disconnected_Timer()"<<std::endl;
	if(this->IsRunning()){
	this->Cancel();
	}
//	std::cout<<"this->Cancel();"<<std::endl;
}

void
Disconnected_Timer::Expire(){
//	std::cout<<"Disconnected_Timer::Expire()"<<std::endl;
	//Simulator::Schedule(Seconds(2),,this);



}



DN::DN(){
	first=	first = AquaSimAddress::GetBroadcast();
	second =255;
//	std::cout<<"DN::DN"<<std::endl;

}
DN::~DN(){
	//std::cout<<"DN::~DN"<<std::endl;

	//expire_time.Cancel();

	//std::cout<<"DN::~DN---expire_time.Cancel();"<<std::endl;
}
void
DN::Expire(){
//	std::cout<<"执行 DN::Expire()"<<std::endl;
	if(expire_time.IsRunning()){
//		std::cout<<"执行 expire_time.IsRunning()"<<std::endl;
		expire_time.Cancel();
	}
//	if((second<16)){
//		std::cout<<"执行 expire_time.SetFunction"<<std::endl;
			expire_time.SetFunction(&DN::clear,this);
//			std::cout<<"执行 expire_time.Schedule"<<std::endl;
			expire_time.Schedule(Seconds(180));
//			std::cout<<"执行 expire_time.Schedule wan"<<std::endl;
//	}


}

void
DN::clear(){
	// std::cout<<"DN::clear()"<<std::endl;
	// std::cout<<"first"<<first<<std::endl;
	// std::cout<<"second"<<second<<std::endl;
	first = AquaSimAddress::GetBroadcast();
	//std::cout<<"first"<<first<<std::endl;
	second =249;
	//std::cout<<"second"<<second<<std::endl;
	if(expire_time.IsRunning()){
		expire_time.Cancel();
	}
//	expire_time.SetFunction(&DN::Expire,this);
//	expire_time.Schedule(Seconds(0.01));
}
AquaSimDynamicRoutingTable::AquaSimDynamicRoutingTable(){
	std::cout<<"执行 AquaSimDynamicRoutingTable::AquaSimDynamicRoutingTable()"<<std::endl;
}


AquaSimDynamicRoutingTable::AquaSimDynamicRoutingTable(DN dn[100])
{
	std::cout<<"执行 AquaSimDynamicRoutingTable::AquaSimDynamicRoutingTable()(DN dn[4])"<<std::endl;
	tp=dn;
//	for(int i =0;i<11;i++){
//
//	std::cout<<&tp[i]<<"------"<<&dn[i]<<std::endl;
//	getchar();
//	}
	NS_LOG_FUNCTION(this);
}

TypeId
AquaSimDynamicRoutingTable::GetTypeId()
{
	std::cout<<"执行 AquaSimDynamicRoutingTable::GetTypeId()"<<std::endl;
  static TypeId tid = TypeId("ns3::AquaSimDynamicRoutingTable");
  return tid;
}

AquaSimAddress
AquaSimDynamicRoutingTable::NodeId()
{
	std::cout<<"执行 AquaSimDynamicRoutingTable::NodeId()"<<std::endl;
  return m_dr->RaAddr();
}

void
AquaSimDynamicRoutingTable::SetRouting(Ptr<AquaSimDynamicRouting> dynamic)
{
	std::cout<<"执行 AquaSimDynamicRoutingTable::SetRouting()"<<std::endl;
	NS_LOG_FUNCTION(this);

	m_dr = dynamic;
	std::cout<<"----> size of m_dr: "<<sizeof(m_dr)<<std::endl;
	std::cout<<"----> m_dr: "<<m_dr<<std::endl;


}

//void
//AquaSimDynamicRoutingTable::Print(AquaSimAddress id)
//{
//	std::cout<<"执行 AquaSimDynamicRoutingTable::Print()"<<std::endl;
//  NS_LOG_FUNCTION(this << id << Simulator::Now().GetSeconds());
//	for (t_table::iterator it = m_rt.begin(); it != m_rt.end(); it++) {
//		std::cout<<id << "," << (*it).first << "," <<
//				(*it).second.first << "," << it->second.second<<std::endl; //添加
//		//		NS_LOG_INFO(id << "," << (*it).first << "," <<
//		//                  (*it).second.first << "," << it->second.second);
//	}
//}

void
AquaSimDynamicRoutingTable::Print(AquaSimAddress id)
{
	std::cout<<"--> 执行 AquaSimDynamicRoutingTable::Print"<<std::endl;
  NS_LOG_FUNCTION(this << id << Simulator::Now().GetSeconds());
	for (t_table::iterator it = m_rt.begin(); it != m_rt.end(); it++) {
    //NS_LOG_INFO
		std::cout<<id << "," << (*it).first << "," <<
                  (*it).second.first << "," << it->second.second<<std::endl;
	}
}


int
AquaSimDynamicRoutingTable::IfChg ()
{
	std::cout<<"--> 执行 AquaSimDynamicRoutingTable::IfChg()"<<std::endl;
	return m_chg;
}

void
AquaSimDynamicRoutingTable::Clear()
{
	std::cout<<"--> 执行 AquaSimDynamicRoutingTable::Clear()"<<std::endl;
	m_rt.clear();
}

void
AquaSimDynamicRoutingTable::RemoveEntry(AquaSimAddress dest)
{
	std::cout<<"--> 执行 AquaSimDynamicRoutingTable::RemoveEntry()"<<std::endl;
	m_rt.erase(dest);
}

void
AquaSimDynamicRoutingTable::AddEntry(AquaSimAddress dest, DN *next)
{
	std::cout<<"--> 执行 AquaSimDynamicRoutingTable::AddEntry()"<<std::endl;
	//std::cout<<"----> m_rt[dest] = next"<<std::endl;
	next->Expire();
	m_rt[dest] = *next;
	std::cout<<"----> m_rt[dest] = next"<<std::endl;
}

AquaSimAddress
AquaSimDynamicRoutingTable::Lookup(AquaSimAddress dest)
{
	std::cout<<"--> 执行 AquaSimDynamicRoutingTable::Lookup()"<<std::endl;
	t_table::iterator it = m_rt.find(dest);
	if (it == m_rt.end()){
		std::cout<<"----> it == m_rt.end(): AquaSimAddress::GetBroadcast()"<<std::endl;
		return AquaSimAddress::GetBroadcast();
	}
	else{
		std::cout<<"----> (*it).second.first"<<std::endl;
		return (*it).second.first; //add by jun
	}
}

uint32_t
AquaSimDynamicRoutingTable::Size()
{
	std::cout<<"--> 执行 AquaSimDynamicRoutingTable::Size()"<<std::endl;
	std::cout<<"----> 返回 m_rt.size(): "<<m_rt.size()<<std::endl;
	return m_rt.size();
}

// 更新后，second+1
// 路由表 目的地址，转发地址，优先级号
void
AquaSimDynamicRoutingTable::Update(t_table* newrt, AquaSimAddress Source_N) //add by jun
{
	std::cout<<"-> 执行 AquaSimDynamicRoutingTable::Update() "<<std::endl;
	//DN tp;//"todo"
	AquaSimAddress tmp;
	m_chg=0;
	/*if(){
	 * Simulator::Schedule(Seconds(1),&AquaSimDynamicRouting_PktTimer::Expire,this);
	 *
	 * }else{
	 * Simulator::Schedule(Seconds(1),&AquaSimDynamicRouting_PktTimer::Expire,this);
	 *
	 * }*/

	// 根据广播来的信息,构建本地可达的路由条目
	//if (Lookup(Source_N) == AquaSimAddress::GetBroadcast())
 // {
		std::cout<<"Source_N:"<<Source_N.GetAsInt()<<std::endl;

		tp[Source_N.GetAsInt()-1].first= Source_N;
		tp[Source_N.GetAsInt()-1].second=1;     // 1跳地址
		//tp.Expire();//"todo"
		std::cout<<Source_N.GetAsInt()<<","<<Source_N.GetAsInt()<<","<<1<<std::endl;
		AddEntry(Source_N, &tp[Source_N.GetAsInt()-1]);  // 这个就是: 本机地址, Source_N, Source_N, 1
		m_chg=1;
		std::cout<<"----> AddEntry(Source_N, tp)"<<std::endl;

	//	std::cout<<"nei"<<std::endl;
	//}
	//std::cout<<"wai"<<std::endl;

	// Source_N在table中
	for( t_table:: iterator it=newrt->begin(); it !=newrt->end(); it++)
  {
	//	std::cout<<"t_table:: iterator"<<std::endl;
	//	std::cout<<"(*it).first111111"<<(*it).first<<std::endl;//
	//	std::cout<<"(*it).first222222"<<it->first<<std::endl;
		if( it->first == NodeId() )
			continue;
		if(((*it).second.second.GetAsInt() +1)>=6){//"todo"discard message that more than 16 hops.
	//		std::cout<<"(m_rt[it->first].first.GetAsInt()"<<m_rt[it->first].first.GetAsInt()<<std::endl;
	//		std::cout<<"(*it).second.first.GetAsInt()"<<(*it).second.first.GetAsInt()<<std::endl;
			//std::cout<<"(*it).second.first"<<(*it).second.first<<std::endl;
			//getchar();
			//if((m_rt[it->first].first.GetAsInt() == (*it).second.first.GetAsInt())||m_rt[it->first].first==0){
			if((m_rt[it->first].first.GetAsInt() == Source_N.GetAsInt())||m_rt[it->first].first==0){
//				std::cout<<"it->first"<<it->first.GetAsInt()<<std::endl;
//				std::cout<<"(m_rt[it->first].first.GetAsInt()"<<m_rt[it->first].first.GetAsInt()<<std::endl;
//				std::cout<<"(*it).second.first.GetAsInt()"<<(*it).second.first.GetAsInt()<<std::endl;
//				std::cout<< "," << it->first<<","<<m_rt[it->first].first << "," <<
//									m_rt[it->first].second << ","<<std::endl;
//				RemoveEntry((*it).first);
//				std::cout<< "," << it->first<<","<<m_rt[it->first].first << "," <<
//						m_rt[it->first].second << ","<<std::endl;
//				getchar();
				tp[(*it).first.GetAsInt()-1].first = AquaSimAddress::GetBroadcast();
				tp[(*it).first.GetAsInt()-1].second =16;
				std::cout<<(*it).first<<","<<(*it).second.first<<","<<(*it).second.second<<std::endl;
				AddEntry((*it).first, &tp[(*it).first.GetAsInt()-1]);
				continue;
			}
			continue;
		}
		// 根据广播来的信息,构建本地可达的路由条目

//add expire in there
		if (Lookup((*it).first) != AquaSimAddress::GetBroadcast() )
    {
			tmp = Lookup((*it).first);

			// 留大的
			if (m_rt[it->first].second.GetAsInt() >= (*it).second.second.GetAsInt() +1 )/* notice the relationship between m_rt[it->first]
																						* and (*it).second*/
      {
				std::cout<<"留大的"<<std::endl;
				//RemoveEntry((*it).first);
				tp[(*it).first.GetAsInt()-1].first=Source_N;
				tp[(*it).first.GetAsInt()-1].second= AquaSimAddress((*it).second.second.GetAsInt() + 1);
				//tp.Expire();//"todo"
			//	std::cout<<(*it).first<<","<<(*it).second.first<<","<<(*it).second.second<<std::endl;
				AddEntry((*it).first, &tp[(*it).first.GetAsInt()-1]);
				m_chg=1;
				continue;
      }else{
    	  std::cout<<"another"<<std::endl;
			if((m_rt[it->first].first.GetAsInt() == Source_N.GetAsInt())){
				tp[(*it).first.GetAsInt()-1].first=Source_N;
								tp[(*it).first.GetAsInt()-1].second= AquaSimAddress((*it).second.second.GetAsInt()+1);
								//tp.Expire();//"todo"
								std::cout<<(*it).first<<","<<(*it).second.first<<","<<(*it).second.second<<std::endl;
								AddEntry((*it).first, &tp[(*it).first.GetAsInt()-1]);
								m_chg=1;
			}
			continue;
      }
    }

		// 没找到
//		else{
//			if((m_rt[it->first].first.GetAsInt() == Source_N.GetAsInt())){
//				tp[(*it).first.GetAsInt()-1].first=Source_N;
//								tp[(*it).first.GetAsInt()-1].second= AquaSimAddress((*it).second.second.GetAsInt()+1);
//								//tp.Expire();//"todo"
//								AddEntry((*it).first, &tp[(*it).first.GetAsInt()-1]);
//								m_chg=1;
//			}
			else{
			tp[(*it).first.GetAsInt()-1].first= Source_N;
			//std::cout<<"tp.first"<<Source_N<<std::endl;
			//std::cout<<"(*it).second.second"<<(*it).second.second.GetAsInt()<<std::endl;
			tp[(*it).first.GetAsInt()-1].second=AquaSimAddress((*it).second.second.GetAsInt() + 1);
			//std::cout<<"(*it).second.first"<<(*it).second.first.GetAsInt()<<std::endl;
			//std::cout<<"tp.second"<<tp[(*it).first.GetAsInt()-1].second<<std::endl;
			//std::cout<<"(*it).first"<<(*it).first<<std::endl;
			//std::cout<<(*it).first<<","<<(*it).second.first<<","<<(*it).second.second<<std::endl;
			//tp.Expire();
			AddEntry((*it).first, &tp[(*it).first.GetAsInt()-1]);
			 //chg=change
			m_chg=1;
			}
		}
//	}
//	for(int i = 0;i<4; i++){
//		AddEntry(i+1, &tp[i]);
//			}
	for( t_table:: iterator it=newrt->begin(); it !=newrt->end(); it++){
	//	std::cout<<"for( t_table:: iterator it=newrt->begin(); it !=newrt->end(); it++)111"<<std::endl;
		if( it->first == NodeId() )
			continue;
		if((m_rt[it->first].first==tp[(*it).first.GetAsInt()-1].first)&&(m_rt[it->first].second==tp[(*it).first.GetAsInt()-1].second))
		{
			continue;
			}else{
			AddEntry((*it).first, &tp[(*it).first.GetAsInt()-1]);
		}
//		AddEntry((*it).first, &tp[(*it).first.GetAsInt()-1]);(m_rt[it->first].first==tp[(*it).first.GetAsInt()-1].first)&&

	}
	std::cout<<"end"<<std::endl;
//	tp.first= 4;
//	tp.second=1;
//	AddEntry(4, tp);
}

/**** AquaSimDynamicRouting_PktTimer ****/
AquaSimDynamicRouting_PktTimer::AquaSimDynamicRouting_PktTimer(AquaSimDynamicRouting* routing, double updateInterval)// updateInterval=50s
 : Timer()
{
	std::cout<<"执行 AquaSimDynamicRouting_PktTimer::AquaSimDynamicRouting_PktTimer()"<<std::endl;
  NS_LOG_FUNCTION(this);

  m_routing = routing;
  m_updateInterval = 50;  // =50
  std::cout<<"----> size of m_routing: "<<sizeof(m_routing)<<std::endl;
  std::cout<<"----> size of m_routing->m_rTable: "<<sizeof(m_routing->m_rTable)<<std::endl;
  std::cout<<"----> m_routing: "<<m_routing<<std::endl;
  std::cout<<"----> m_updateInterval: "<<m_updateInterval<<std::endl;

}

AquaSimDynamicRouting_PktTimer::~AquaSimDynamicRouting_PktTimer()
{
	std::cout<<"--> 执行 AquaSimDynamicRouting_PktTimer::~AquaSimDynamicRouting_PktTimer()"<<std::endl;
	delete m_routing;
}

// 路猜测是动态路由失效时间,相当于间隔多少时间之后,再次执行Expire()
void
AquaSimDynamicRouting_PktTimer::Expire()
{
	std::cout<<"执行 AquaSimDynamicRouting_PktTimer::Expire()"<<std::endl;
	std::cout<<"----> m_routing执行SendDRoutingPkt() "<<std::endl;
	m_routing->SendDRoutingPkt();
	std::cout<<"----> 完成 m_routing->SendDRoutingPkt() "<<std::endl;
//  m_routing->ResetDRoutingPktTimer();
  double delay = (GetUpdateInterval() + m_routing->BroadcastJitter(10));  //GetUpdateInterval()=50,delay=52.453
  std::cout<<"----> delay= "<<delay<<std::endl;
  std::cout<<"----> 在delay后回调AquaSimDynamicRouting_PktTimer::Expire() "<<delay<<std::endl;
  Simulator::Schedule(Seconds(delay),&AquaSimDynamicRouting_PktTimer::Expire,this);

}


/**** AquaSimDynamicRouting ****/

NS_OBJECT_ENSURE_REGISTERED(AquaSimDynamicRouting);

AquaSimDynamicRouting::AquaSimDynamicRouting() : m_pktTimer(this, 50)
{
	std::cout<<"执行 AquaSimDynamicRouting::AquaSimDynamicRouting()"<<std::endl;
  NS_LOG_FUNCTION(this);
  //n=m_n;
  m_coun=0;
  //m_seqNum=0;
 m_rTable = new AquaSimDynamicRoutingTable(tp);
  m_rTable->SetRouting(this);
  std::cout<<"----> size of m_rTable: "<<sizeof(m_rTable)<<std::endl;

  m_pktTimer.SetFunction(&AquaSimDynamicRouting_PktTimer::Expire,&m_pktTimer);
  // 原有配置
//  m_pktTimer.Schedule(Seconds(0.0000001+10*m_rand->GetValue()));
  /*
   * 更改
   */
  m_pktTimer.Schedule(Seconds(1));

  m_rand = CreateObject<UniformRandomVariable> ();
  std::cout<<"---------> m_rand: "<<m_rand<<std::endl;
}

TypeId
AquaSimDynamicRouting::GetTypeId(void)
{
	std::cout<<"执行 AquaSimDynamicRouting::GetTypeId(void)"<<std::endl;
  static TypeId tid = TypeId("ns3::AquaSimDynamicRouting")
      .SetParent<AquaSimRouting>()
      .AddConstructor<AquaSimDynamicRouting>()
      .AddAttribute("AccessibleVar", "Accessible Variable.",
        IntegerValue(0),
        MakeIntegerAccessor (&AquaSimDynamicRouting::m_accessibleVar),
        MakeIntegerChecker<int> ())
	 .AddAttribute ("N", "Enable routing VBF setting. Default 1 is true.",
	    IntegerValue(11),
	    MakeIntegerAccessor(&AquaSimDynamicRouting::m_n),
	    MakeIntegerChecker<int>())
    ;
  return tid;
}

int64_t
AquaSimDynamicRouting::AssignStreams (int64_t stream)
{
	std::cout<<"执行 AquaSimDynamicRouting::AssignStreams (int64_t stream)"<<std::endl;
  NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}

/*int AquaSimDynamicRouting::command(int argc, const char*const* argv) {
  if (argc == 2) {
	if (strcasecmp(argv[1], "start") == 0) {
	  m_pktTimer.resched(0.0000001+broadcast_jitter(10));
	  return TCL_OK;
	}
	else if (strcasecmp(argv[1], "print_rtable") == 0) {
	  if (logtarget_ != 0) {
		sprintf(logtarget_->pt_->buffer(), "P %f _%d_ Routing Table",
				Simulator::Now(),
				RaAddr());
		logtarget_->pt_->dump();
	  }
	  else {
		fprintf(stdout, "%f _%d_ If you want to print this routing table "
						"you must create a trace file in your tcl script",
						Simulator::Now(),
						RaAddr());
	  }
	  return TCL_OK;
	}
  }
  else if (argc == 3) {
	// Obtains corresponding dmux to carry packets to upper layers
	if (strcmp(argv[1], "port-dmux") == 0) {
	  dmux_ = (PortClassifier*)TclObject::lookup(argv[2]);
	  if (dmux_ == 0) {
		fprintf(stderr, "%s: %s lookup of %s failed\n",
				__FILE__,
				argv[1],
				argv[2]);
		return TCL_ERROR;
	  }
	  return TCL_OK;
	}
	// Obtains corresponding tracer
	else if (strcmp(argv[1], "log-target") == 0 ||
	  strcmp(argv[1], "tracetarget") == 0) {
	  logtarget_ = (Trace*)TclObject::lookup(argv[2]);
	if (logtarget_ == 0)
	  return TCL_ERROR;
	return TCL_OK;
	  }
  }

}
*/
bool AquaSimDynamicRouting::TxProcess(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber) {
	/**
	 * 路由接受函数的判断逻辑如下:
	 * 1.加载路由表
	 * 2.判断是否为本节点数据	是:接收并向上传递
	 * 3.找到下一跳，判断是否死循环	是:不处理该包
	 * 4.更新头部信息
	 * 5.发包
	 */
	// dest如果为???,说明包从下方来，不做添加头部处理
	DRoutingHeader drh;
	drh.SetNumForwards((uint16_t)0);
	drh.SetPktLen(40);
	drh.SetPktSrc(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
	drh.SetPktDst(AquaSimAddress::ConvertFrom(dest));
	p->AddHeader(drh);
	
	std::cout<<"进入 AquaSimDynamicRouting::Recv() "<<std::endl;
	return Recv(p, dest, protocolNumber);
}

bool
AquaSimDynamicRouting::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
	std::cout<<"执行 AquaSimDynamicRouting::Recv() "<<std::endl;
	std::cout<<"--> 本机地址: "<<RaAddr()<<std::endl;
	std::cout<<"--> AquaSimDynamicRouting::Recv packet: "<<std::endl<<p->ToString()<<std::endl;
	// AquaSimHeader ash;
	// Ipv4Header iph;//xj
	DRoutingHeader drh;
	AquaSimPtTag ptag;//?????xj
	//AlohaHeader aloh;
	AquaSimTrailer ast;

//   if (p->GetSize() <= 32)  //p->GetSize()==35,后来都是52,因为auqasimheader是22个字节,然后负载是13个字节,里面数据是hello world!
//   {
//     p->AddHeader(iph);
//     p->AddHeader(drh);
//     p->AddHeader(ash);
//   }//xj 感觉是判断是从方向的？ 好奇怪啊 那不是以及有ash 先注释了
  //getchar();
	p->RemoveTrailer(ast);//xj 先注释了看有没有必要
//	 getchar();
	std::cout<<"p after remove ash: "<<p->ToString()<<std::endl;
	p->PeekHeader(drh);
	//p->RemoveHeader(drh);
	p->PeekPacketTag(ptag);//xj 是个属性判断的值
//	 getchar();// Payload (size=13)
	std::cout<<"p->PeekPacketTag(ptag) :"<<p->ToString()<<std::endl;

//  std::cout<<p->print(std::cout)<<std::endl;
	//struct hdr_cmn* ch = HDR_CMN(p);
	//struct hdr_ip* ih = HDR_IP(p);
	//ch->uw_flag() = true;

//	if ((ash.GetDAddr()== RaAddr()) && (ptag.GetPacketType() != AquaSimPtTag::PT_UW_DROUTING)){
//		std::cout<<"----> 执行了这个if"<<std::endl;
//		p->RemoveHeader(aloh);
//		p->RemoveHeader(drh);
//		p->RemoveHeader(iph);
//		p->AddHeader(aloh);
//		p->AddHeader(ash);
//		std::cout<<"p: "<<p->ToString()<<std::endl;
//		SendUp(p);
//		return true;
//	}

	if (drh.GetPktSrc() == RaAddr()) {     // 如果本机地址是源地址//从哪来
		// If there exists a loop, must drop the packet
		std::cout<<"----> 本机地址是源地址 "<<std::endl;
		if (drh.GetNumForwards() > 0) {   // 如果转发数大于0
			NS_LOG_INFO("Recv: there exists a loop, dropping packet=" << p);
			//drop(p, DROP_RTR_ROUTE_LOOP);
			p=0;
			return false;
		}
		// else if this is a packet I am originating, must add IP header
		// 转发数等于0，说明我是源，需要加如IP头
		// else if (ash.GetNumForwards() == 0){
		// 	std::cout<<"----> 判断 ash.GetNumForwards() == 0 "<<std::endl;
		// 	ash.SetSize(ash.GetSize() + IP_HDR_LEN);
		// }//xj 可能没用

	}
	// else if(ash.GetNextHop() != AquaSimAddress::GetBroadcast() && ash.GetNextHop() != RaAddr() )
	// {//xj  来的包的下一跳从来来的 mac给记录了吗  还是直接mac层处理。

	// 	NS_LOG_INFO("Recv: duplicate, dropping packet=" << p);
	// 	//drop(p, DROP_MAC_DUPLICATE);
	// 	p=0;
	// 	return false;
	// }//xj 由mac处理

	// 转发数加1，添加回header
	
	

	// If it is a protoname packet, must process it
	if (ptag.GetPacketType() == AquaSimPtTag::PT_UW_DROUTING)//"todo"15
	{
		std::cout<<"----> 包类型是PT_UW_DROUTING"<<std::endl;
		RecvDRoutingPkt(p);
		return true;
	}
	// Otherwise, must forward the packet (unless TTL has reached zero)
	
			p->RemoveHeader(drh);
			if(drh.GetNumForwards()>0){
		uint8_t numForward = drh.GetNumForwards() + 1;
	drh.SetNumForwards(numForward);
	//p->AddHeader(ash);//xj 可能没用
	std::cout<<"----> NumForward+1后: "<<p->ToString()<<std::endl;

	std::cout<<"----> ptag.GetPacketType(): "<<ptag.GetPacketType()<<std::endl;//"todo"AquaSimPtTag::PT_UW_DROUTING is 15
//	std::cout<<"执行 ptag.GetPacketType() "<<ptag.GetPacketType()::endl;
	}
		//p->RemoveHeader(ash);
		std::cout<<"----> p->RemoveHeader(ash)后: "<<p->ToString()<<std::endl;

		/*
		 * 第一条之后,如果mac层不去掉包头,这里就要去掉mac层的包
		 * */
		// if (ash.GetNumForwards()!=1){              // add by Tong Zhang
		// 	//p->RemoveHeader(aloh);"todo"Maybe because MAC has been modified, no need to remove the aloh header
		// 	p->RemoveHeader(drh);
		// 	//p->RemoveHeader(iph);

		// }//xj

//		std::cout<<"p 看看aloh: "<<std::endl<<p->ToString()<<std::endl;
		// TTL TimeToLive 生存时间值
		uint8_t ttl = 255-drh.GetNumForwards();  //ttl = 255
		std::cout<<"----> ttl: "<<uint32_t(ttl)<<std::endl;
		if (ttl == 0) {
			NS_LOG_INFO("Recv: RTR TTL == 0, dropping packet=" << p);
			//drop(p, DROP_RTR_TTL);
			p=0;
			return false;
		}
		int temp =drh.GetNumForwards()+1;
		drh.SetNumForwards(temp);
		
		
	//iph.SetTtl(ttl);

	std::cout<<"----> 添加三个包头之前: "<<p->ToString()<<std::endl;
	//p->AddHeader(iph);
	ast.SetSAddr(drh.GetPktSrc());
	ast.SetDAddr(drh.GetPktDst());
    p->AddHeader(drh);
	p->AddTrailer(ast);
    //p->AddHeader(ash);

//    std::cout<<"p 看看加回来之后ash是不是UP: "<<std::endl<<p->ToString()<<std::endl;
//    std::cout<<"----> 经过NumForwards+1和ttl-1之后"<<p->ToString()<<std::endl;;
//    std::cout<<"p after removeHeader: "<<p->ToString()<<std::endl;

    // 发送数据包
    std::cout<<"----> 准备forwarddata: "<<p->ToString()<<std::endl;

    ForwardData(p);
	

  return true;
}


void
AquaSimDynamicRouting::RecvDRoutingPkt(Ptr<Packet> p)
{
	std::cout<<"执行 AquaSimDynamicRouting::RecvDRoutingPkt()"<<std::endl;
	std::cout<<"--> 本机地址: "<<RaAddr()<<std::endl;
	std::cout<<"----> RecvDRoutingPkt中的p: "<<p->ToString()<<std::endl;
	DRoutingHeader drh;
	//AquaSimHeader ash;
	//Ipv4Header iph;
	//AlohaHeader aloh;   // 添加
	//p->RemoveHeader(ash);//xj
	// 在去掉ash头后,还有AlohaHeader//"todo"maybe not
	//p->RemoveHeader(aloh);  // 添加  "todo"Maybe because MAC has been modified, no need to remove the aloh header
	 p->RemoveHeader(drh);//xj
	// p->RemoveHeader(iph);//xj
	std::cout<<"----> drh内容:"<<std::endl;
	drh.Print(std::cout);
//	p->AddHeader(aloh);
//	p->AddHeader(ash);

	// All routing messages are sent from and to port RT_PORT,
	// so we check it.
	//assert(ih->sport() == RT_PORT);
	//assert(ih->dport() == RT_PORT);
	// take out the packet, rtable

	DN temp_DN;   // 结构是 first, second
	t_table temp_rt;  // typedef std::map<AquaSimAddress, DN> t_table;
	AquaSimAddress temp1;  // memset (m_address, 0, 2);
	uint32_t size = p->GetSize();   //size=57
	std::cout<<"size"<<size<<std::endl;
	uint8_t *data = new uint8_t[size]; // data 9
	p->CopyData(data,size);

	for(uint8_t i=0;i<size;i++){
		std::cout<<"+++"<<+data[i]<<std::endl;
	}
	// 这里有问题,因为GetEntryNum
	std::cout<<"----> drh.GetEntryNum(): "<<drh.GetEntryNum()<<std::endl;//"todo""Packet length"

//	for(uint i=0; i < drh.GetEntryNum(); i++) {
//
//		temp1 = *((AquaSimAddress*)data);
//		data += sizeof(AquaSimAddress);
//
//		temp_DN.first = *((AquaSimAddress*)data);
//		//temp2 = *((AquaSimAddress*)data);
//		data += sizeof(AquaSimAddress);
//
//		//temp_DN[temp2]== *((int*)data);
//		temp_DN.second = *((int*)data);
//		data += sizeof(int);
//		temp_rt[temp1]=temp_DN;
//	}

	for(uint i=0; i < drh.GetEntryNum(); i++) {

			temp1 = *((uint8_t*)data);
			data += sizeof(AquaSimAddress)/16;
			std::cout<<"sizeof(AquaSimAddress)"<<sizeof(AquaSimAddress)<<std::endl;
			std::cout<<"temp1"<<temp1.GetAsInt()<<std::endl;
			temp_DN.first = *((uint8_t*)data);
			//temp2 = *((AquaSimAddress*)data);
			data += sizeof(AquaSimAddress)/16;
			std::cout<<"temp_DN.first"<<temp_DN.first.GetAsInt()<<std::endl;
			//temp_DN[temp2]== *((int*)data);
			temp_DN.second = *((uint8_t*)data);
			std::cout<<"temp_DN.second"<<temp_DN.second.GetAsInt()<<std::endl;
			data += sizeof(AquaSimAddress)/16;//  ?????
			//temp_DN.Expire();//"todo" test
			//data += sizeof(AquaSimAddress)/16;
			temp_rt[temp1]=temp_DN;
		}


		m_rTable->Update(&temp_rt, drh.GetPktSrc());
		std::cout<<"update"<<std::endl;



	m_rTable->Print(RaAddr());  // 添加
	std::cout<<"m_rTable.Print(RaAddr());"<<std::endl;

	if (m_rTable->IfChg() ==1) {
		std::cout<<"----> m_rTable.IfChg() ==1: m_pktTimer.SetUpdateInterval(30.0)"<<std::endl;
		m_pktTimer.SetUpdateInterval(50.0);//"todo"
	}

	if (m_rTable->IfChg() ==0)//"todo" maybe retransmission times
	{
		std::cout<<"----> m_rTable.IfChg() ==0: m_coun++"<<std::endl;
		m_coun++;
	}

	if (m_coun ==2) {
		std::cout<<"----> m_coun ==2: m_pktTimer.SetUpdateInterval(100.0)"<<std::endl;
		m_pktTimer.SetUpdateInterval(50.0);
		m_coun=0;
	}

	// Release resources
  p=0;
}

double
AquaSimDynamicRouting::BroadcastJitter(double range)
{
	std::cout<<"-->执行 AquaSimDynamicRouting::BroadcastJitter()"<<std::endl;
	std::cout<<"----> range*m_rand->GetValue(): "<<range*m_rand->GetValue()<<std::endl;

	return range*m_rand->GetValue();
}


void
AquaSimDynamicRouting::SendDRoutingPkt()
{
	std::cout<<"执行 AquaSimDynamicRouting::SendDRoutingPkt()"<<std::endl;
	std::cout<<"--> 本机地址: "<<RaAddr()<<std::endl;
	NS_LOG_FUNCTION(this);

	/*
	 * 创建一个动态路由包
	 */
	Ptr<Packet> p = Create<Packet>();
	//AquaSimHeader ash;
	DRoutingHeader drh;
	AquaSimTrailer ast;
	//Ipv4Header iph;//xj
	AquaSimPtTag ptag;

	//add by jun
	//struct hdr_uw_drouting_pkt* ph = HDR_UW_DROUTING_PKT(p);

	drh.SetPktSrc(RaAddr());    // 设置源为本机地址,第一次为01
//	std::cout<<"----> SetPktSrc: "<<RaAddr()<<std::endl;

	drh.SetPktDst(AquaSimAddress::GetBroadcast());
	ast.SetDAddr(drh.GetPktDst());
	ast.SetSAddr(drh.GetPktSrc());
	ast.SetNextHop(AquaSimAddress::GetBroadcast());
	drh.SetNumForwards((uint16_t)0);

	drh.SetPktLen(7);           // 设置包长度为7//?? xj
//    std::cout<<"----> SetPktLen(7)"<<std::endl;

	drh.SetPktSeqNum(m_seqNum++);
	//drh.SetPktSeqNum(0);
	std::cout<<"----> SetPktSeqNum: "<< (uint16_t)m_seqNum<<std::endl;

//	std::cout<<"----> SetEntryNum() "<<std::endl;
	drh.SetEntryNum(m_rTable->Size());

//	std::cout<<"----> SetPktLen() "<<std::endl;
	drh.SetPktLen(sizeof(drh.GetPktLen())+sizeof(drh.GetPktSrc()) +sizeof(drh.GetPktDst())+
                 sizeof(drh.GetNumForwards())+ sizeof(drh.GetPktSeqNum())+sizeof(drh.GetEntryNum()) );//xj
	std::cout<<"----> 设置drh内容之后: "<<std::endl;
	drh.Print(std::cout);

	/*
	 * m_rTable.size = 0 这里可能有问题
	 * */

	uint32_t size = (m_rTable->Size())*(3*sizeof(AquaSimAddress))/16;  // 经测试 sizeof(AquaSimAddress) is 16
	std::cout<<"----> (m_rTable.Size())*(3*sizeof(AquaSimAddress)) size: "<<size<<std::endl;
	uint8_t* payload = new uint8_t[size];

	/*
	 * 通过迭代器,加载路由信息到payload中
	 */
	int iter=0;
	for(t_table::iterator it=m_rTable->m_rt.begin(); it!=m_rTable->m_rt.end(); it++)// ?
	{
		// payload格式: first, second.first, second.second
		std::cout<<"----> 迭代加载路由表进payload: "<<std::endl;
		std::cout<<"----> for loop "<<std::endl;
		payload[iter]=it->first.GetAsInt();
		//std::cout<<"----> payload[iter]: "<<payload[iter]<<std::endl;

		payload[iter+1]=it->second.first.GetAsInt();
		//std::cout<<"----> payload[iter+1]: "<<payload[iter+1]<<std::endl;

		payload[iter+2]=it->second.second.GetAsInt();
		//std::cout<<"----> payload[iter+2]: "<<payload[iter+2]<<std::endl;
		iter+=3;
//		payload[iter+3]=it->second.expire_time;//"todo " test
//		payload[iter+3]=0;
//		iter+=4;
	}
	/*
	 * 创建tempacket,包括payload和size
	 */
	Ptr<Packet> tempPacket = Create<Packet>(payload,size);
	std::cout<<"----> tempPacket: "<<tempPacket->ToString()<<std::endl;
	p->AddAtEnd(tempPacket);
	std::cout<<"----> p->AddAtEnd: "<<p->ToString()<<std::endl;
	std::cout<<"----> 加载temPacket后的p: "<<p->ToString()<<std::endl;

	ptag.SetPacketType(AquaSimPtTag::PT_UW_DROUTING);

	/*
	 * 设置ash信息
	 */
	//ash.SetSAddr(RaAddr());
	//ash.SetDAddr(AquaSimAddress::GetBroadcast());
	//ih->sport() = RT_PORT;
	//ih->dport() = RT_PORT;
	//iph.SetTtl(1);    // 将IPV4的TTL设置为1,进行广播,只保留1跳

	// ash.SetDirection(AquaSimHeader::DOWN);
	// ash.SetSize(IP_HDR_LEN + drh.GetPktLen()+size);
	// ash.SetErrorFlag(false);
	// ash.SetNextHop(AquaSimAddress::GetBroadcast());
	//ash.addr_type() = NS_AF_INET;
	//ash.uw_flag() = true;

 // p->AddHeader(iph);
  p->AddHeader(drh);
  //p->AddHeader(ash);
  p->AddPacketTag(ptag);// xj这是啥
  p->AddTrailer(ast);
  std::cout<<"----> SendDRoutingPkt最终发送的p: "<<std::endl<<"----> aa"<<p->ToString()<<"bb"<<std::endl;
  Time jitter = Seconds(m_rand->GetValue()*0.5);  // 0.3557663s
  std::cout<<"----> Time jitter: "<<jitter<<std::endl;
  std::cout<<"----> SendDRoutingPkt()调用SendDown()"<<jitter<<std::endl;
  //回调函数,在jitter=0.35s后执行
  Simulator::Schedule(jitter,&AquaSimRouting::SendDown,this,p,ast.GetNextHop(),jitter);
}

void
AquaSimDynamicRouting::ResetDRoutingPktTimer()
{
	std::cout<<"执行 AquaSimDynamicRouting::ResetDRoutingPktTimer()"<<std::endl;
  m_pktTimer.Schedule( Seconds(50 + BroadcastJitter(10)) );
}

void
AquaSimDynamicRouting::ForwardData(Ptr<Packet> p)
{
	std::cout<<"执行 AquaSimDynamicRouting::ForwardData()"<<std::endl;
	std::cout<<"--> 本机地址: "<<RaAddr()<<std::endl;
	//AquaSimHeader ash;
	//Ipv4Header iph;
	DRoutingHeader drh;
	AquaSimTrailer ast;
	//AlohaHeader aloh;
	p->PeekHeader(drh);
	p->RemoveTrailer(ast);
	//struct hdr_ip* ih = HDR_IP(p);


	//double t = NOW;
//  if (ash.GetDirection() == AquaSimHeader::UP && ash.GetDAddr() == RaAddr()){
//	  SendUp(p);
//  }

	// if (ash.GetDirection() == AquaSimHeader::UP &&
	//     (ash.GetDAddr() == AquaSimAddress::GetBroadcast() || ash.GetDAddr() == RaAddr()))
	if(drh.GetPktDst()==RaAddr())
	{
		//ash.SetSize(ash.GetSize() - IP_HDR_LEN);//xj
		NS_LOG_INFO("ForwardData: dmux->recv not implemented yet for packet=" << p);
		std::cout<<"ForwardData: dmux->recv not implemented yet for packet="<<p<<std::endl;
//		dmux_->recv(p, (Handler*)NULL); //should be sending to dmux
		//SendUp should handle dmux...
//		p->RemoveHeader(aloh);
//		p->RemoveHeader(drh);
//		p->RemoveHeader(iph);
//		p->AddHeader(aloh);
//		p->AddHeader(ash);
//		SendUp(p);
//		return;
		std::cout<<"收到了吗？"<<std::endl;
		if(!SendUp(p))
			NS_LOG_WARN("ForwardData: Something went wrong when passing packet up.");
//			std::cout<<"ForwardData: Something went wrong when passing packet up."<<std::endl;
		return;
	}

	else
	{
		//ash.SetDirection(AquaSimHeader::DOWN);
		//ash.addr_type() = NS_AF_INET;
		if (ast.GetDAddr() == AquaSimAddress::GetBroadcast()){
				std::cout<<"aaaaaaaaaaaaa"<<std::endl;
			ast.SetNextHop(AquaSimAddress::GetBroadcast());
		}
			
		else
    {
			/*查找下一跳
			 * 有:设置下一跳为目的地址*/
			AquaSimAddress next_hop = m_rTable->Lookup(ast.GetDAddr());
			std::cout<<"----> m_rTable.Lookup next_hop: "<<next_hop<<std::endl;  // next_hop=0255"todo"01,04 no record

			if (next_hop == AquaSimAddress::GetBroadcast())
			{
				std::cout<<"--> next_hop is AquaSimAddress::GetBroadcast()"<<std::endl;
				NS_LOG_DEBUG("ForwardData: Node " << RaAddr() <<
						" can not forward a packet destined to " << ast.GetDAddr() <<
						" at time " << Simulator::Now().GetSeconds());
				std::cout<<"ForwardData: Node " << RaAddr() <<
						" can not forward a packet destined to " << ast.GetDAddr() <<
						" at time " << Simulator::Now().GetSeconds()<<std::endl;
				std::cout<<"--> NS_LOG_DEBUG"<<std::endl;
				std::cout<<"bbbbbbb"<<std::endl;
			ast.SetNextHop(AquaSimAddress::GetBroadcast());
				//drop(p, DROP_RTR_NO_ROUTE);
				p=0;
				return;
			}
			else{
//				p->RemoveHeader(drh);
//				p->RemoveHeader(iph);


				std::cout<<"----> SetNextHopqian: "<<p->ToString()<<std::endl;
				ast.SetNextHop(next_hop);
				std::cout<<"----> SetNextHop: "<<next_hop<<std::endl;
//				if(ash.GetNumForwards()!=0){
//									Ipv4Header iph1;
//									DRoutingHeader drh1;
//									Ipv4Header iph2;
//									DRoutingHeader drh2;
//									p->RemoveHeader(drh1);
//									p->RemoveHeader(iph1);
//									p->RemoveHeader(drh2);
//									p->RemoveHeader(iph2);
//									p->AddHeader(iph1);
//									p->AddHeader(drh1);
//								}
				p->AddTrailer(ast);


				std::cout<<"----> SetNextHop后: "<<p->ToString()<<std::endl;
			}
		}
    Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ast.GetNextHop(),Seconds(0));
	}
}

void AquaSimDynamicRouting::DoDispose()
{
	std::cout<<"执行 AquaSimDynamicRouting::DoDispose()"<<std::endl;
  m_rand=0;
  AquaSimRouting::DoDispose();
}
