/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 JinLin University
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
 * 
 *
 * Author: HanCheng <827569146@qq.com>
 */

#include "aqua-sim-routing-predictionModel.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-propagation.h"
#include "aqua-sim-trailer.h"
#include "aqua-sim-datastructure.h"
#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <time.h>
#include "aqua-sim-mobility-ug.h"
#include "aqua-sim-routing.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimPredictionModel");

void predictionModel::generate_link_table(AquaSimAddress curnodeAdd,Vector p){
    PutinHash(curnodeAdd,p,0,0);
    std::ifstream neiborinfo("info.txt");//the struct of info.txt is : ID position.x position.y position.z anglea angleb;
	if (!neiborinfo.is_open())
	{
		std::cout << "can not open this file" << "\n";
	}
	std::string str;
	int current_node;
	Vector q;
    double a,b;
	while(getline(neiborinfo,str)){
		std::istringstream istr(str);
		istr>>current_node;
		istr>>q.x;
		istr>>q.y;
		istr>>q.z;
        istr>>a;
        istr>>b;
		if(iscloseenough(p,q)==1){
			PutinHash(curnodeAdd,q,a,b);
		}
	}
	neiborinfo.close();
}

double predictionModel::get_link_time(AquaSimAddress i, AquaSimAddress j){
    double vcx=0.0,vcy=0.0;//vol of stream
    Vector v_i;
    Vector vol_i;
    double a1,b1;
    Vector v1;
    v1.x=vol_i.x*cos(a1*pi/180.0)*sin(b1*pi/180.0)+vcx;
    v1.y=vol_i.y*cos(a1*pi/180.0)*cos(b1*pi/180.0)+vcy;
    v1.z=0.0;
    //how to getangel of node
    //how to getvol of stream
    Vector v_j;//?how to get
    Vector vol_j;
    double a2,b2;
    //how to getangel of node
    Vector v2;
    v2.x=vol_i.x*cos(a2*pi/180.0)*sin(b2*pi/180.0)+vcx;
    v2.y=vol_i.y*cos(a2*pi/180.0)*cos(b2*pi/180.0)+vcy;
    v2.z=0.0;

    //calculate a,b,c,d,e .etc
    double a=v_i.x-v_j.x;
    double b=v_i.y-v_j.y;
    double c=v_i.z-v_j.z;
    double e=v1.x*sin(a1*pi/180.0)*cos(b1*pi/180.0)-v2.x*sin(a2*pi/180.0)*cos(b2*pi/180.0);
    double f=v1.y*sin(a1*pi/180.0)*cos(b1*pi/180.0)-v2.y*sin(a2*pi/180.0)*cos(b2*pi/180.0);
    double g=v1.z*cos(a1*pi/180.0)-v2.z*cos(a2*pi/180.0);
    double m=e*e+f*f+g*g;
    double n=2*(a*e+b*f+c*g);
    double o=a*a+b*b+c*c-R*R;
    
    //calculate Texp
    double Texp1=(-n+sqrt(n*n-4*m*o))/(2*m);
    double Texp2=(-n-sqrt(n*n-4*m*o))/(2*m);
    std::cout<<Texp1<<","<<Texp2<<std::endl;
    return Texp1;
}

std::pair<Time,Time> predictionModel::get_link_duration(AquaSimAddress i, AquaSimAddress j){
      Time start=Simulator::Now();
      Time end=Simulator::Now();
      return {start,end};
}

std::vector<AquaSimAddress> predictionModel::get_neighbor_list(AquaSimAddress curnodeAdd,time_t t){

}

int predictionModel::iscloseenough(Vector p,Vector q){
    double d=sqrt((p.x-q.x)*(p.x-q.x)+(p.y-q.y)*(p.y-q.y)+(p.z-q.z)*(p.z-q.z));
   //std::cout<<"d:"<<d<<"\n";
   return d>R ? 0 : 1 ;
}

void predictionModel::PutinHash(AquaSimAddress fAddr,Vector p,double a,double b){
    local_entry entry=std::make_pair(fAddr,p);
	node_info *hashptr;
    std::map<local_entry,node_info*>::iterator it;
    //todo //表中已有该节点信息，更新,(因为位置包含在map的key值中，所以只能删除原来的再加入新的)
	for(it=m_htable.begin();it!=m_htable.end();it++){
		local_entry cur=it->first;
		node_info *nei=it->second;
		if(cur.first==fAddr){
           //std::cout<<"hasptr exit,update\n";
           nei->a=a;
		   nei->b=b;
		   cur.second=p;
		   std::cout<<"insert location:"<<cur.second.x<<" "<<cur.second.y<<" "<<cur.second.z<<std::endl;
		   return;
		}
	}
     hashptr=new node_info[1];
	   hashptr[0].a=a;
	   hashptr[0].b=b;
	   std::pair<local_entry,node_info*> newPair;
	   newPair.first=entry;newPair.second=hashptr;
        //std::cout<<"position:"<<entry.first<<"\n";
		  // std::cout<<m_htable.size()<<"......before insert.......\n";
		m_htable.insert(newPair);
}