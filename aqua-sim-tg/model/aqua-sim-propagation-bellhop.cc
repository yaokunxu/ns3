/*
 * aqua-sim-propagation-bellhop.cc
 *
 *  Created on: May 8, 2021
 *  Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */


#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "aqua-sim-propagation-bellhop.h"
#include "aqua-sim-header.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <math.h>
#define _USE_MATH_DEFINES

using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AquaSimBellhopPropagation");
NS_OBJECT_ENSURE_REGISTERED (AquaSimBellhopPropagation);

TypeId
AquaSimBellhopPropagation::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AquaSimBellhopPropagation")
    .SetParent<AquaSimPropagation>()
    .AddConstructor<AquaSimBellhopPropagation>()
  ;
  return tid;
}

AquaSimBellhopPropagation::AquaSimBellhopPropagation () :
    AquaSimPropagation(), basePath("bellhop/")
{
}

AquaSimBellhopPropagation::~AquaSimBellhopPropagation()
{
}



std::vector<PktRecvUnit>*
AquaSimBellhopPropagation::ReceivedCopies(Ptr<AquaSimNetDevice> s,
		Ptr<Packet> p, std::vector<Ptr<AquaSimNetDevice> > dList) {
	NS_LOG_FUNCTION(this);
	NS_ASSERT(dList.size());

	std::vector<PktRecvUnit> *res = new std::vector<PktRecvUnit>;
	//find all nodes which will receive a copy
	PktRecvUnit pru;
	double dist = 0;

	AquaSimPacketStamp pstamp;
	p->PeekHeader(pstamp);

	Ptr<Object> sObject = s->GetNode();
	Ptr<MobilityModel> senderModel = sObject->GetObject<MobilityModel>();

	int senderID = AquaSimAddress::ConvertFrom(s->GetAddress()).GetAsInt();

	double dz, dr;
	vector<vector<double>> *tlt = NULL;
	this->bellhop(senderID, senderModel->GetPosition().z, pstamp.GetFreq(),
			dz, dr, &tlt);


	unsigned i = 0;
	std::vector<Ptr<AquaSimNetDevice> >::iterator it = dList.begin();
	for (; it != dList.end(); it++, i++) {
		Ptr<Object> rObject = dList[i]->GetNode();
		Ptr<MobilityModel> recvModel = rObject->GetObject<MobilityModel>();

		pru.recver = dList[i];
		pru.pDelay = Time::FromDouble(dist / ns3::SOUND_SPEED_IN_WATER,
				Time::S);
		pru.pR = this->getRecvPower(*tlt, senderModel, recvModel, pstamp.GetPt(), dz, dr);
		res->push_back(pru);
	}
	return res;
}

double AquaSimBellhopPropagation::GETpr(Ptr<AquaSimNetDevice> s, double freq,
		double pt, Ptr<AquaSimNetDevice> dList) {

	double dist = 0;

	Ptr<Object> sObject = s->GetNode();
	Ptr<MobilityModel> senderModel = sObject->GetObject<MobilityModel>();

	Ptr<Object> rObject = dList->GetNode();
	Ptr<MobilityModel> recvModel = rObject->GetObject<MobilityModel>();

	int senderID = AquaSimAddress::ConvertFrom(s->GetAddress()).GetAsInt();

	double dz, dr;
	vector<vector<double>> *tlt = NULL;
	this->bellhop(senderID, senderModel->GetPosition().z, freq,
			dz, dr, &tlt);



	double pr = this->getRecvPower(*tlt, senderModel, recvModel, pt, dz, dr);

	return pr;

}

void AquaSimBellhopPropagation::getRangeDepth(Vector sendPos, Ptr<MobilityModel> recvModel,
		double &range, double &recverDepth){


	double dx = sendPos.x - recvModel->GetPosition().x;
	double dy = sendPos.y - recvModel->GetPosition().y;
	range = sqrt(dx * dx + dy * dy);
	recverDepth = recvModel->GetPosition().z;
}


void AquaSimBellhopPropagation::bellhop (int senderID, double senderDepth, double freq,
		double &dz, double &dr, vector<vector<double>> **tlt){

	string sender = std::to_string(senderID);
	this->updateEnvFile(sender, senderDepth, freq);
	system((basePath + "bellhop.exe " + basePath + sender).c_str());

	this->readShd(sender, dz, dr, tlt);

}


double AquaSimBellhopPropagation::getRecvPower(std::vector<std::vector<double>> &tlt, Ptr<MobilityModel> sendModel,
		  Ptr<MobilityModel> recvModel, double sendPower, double dz, double dr){

	double range, recverDepth;
	this->getRangeDepth(sendModel->GetPosition(), recvModel, range, recverDepth);

	int nrz = tlt.size();
	int nrr = tlt[0].size();
	int i = int((recverDepth / dz) + 0.5);
	if(i >= nrz)
		i = nrz - 1;
	int j = int((range / dr) + 0.5);
	if(j >= nrr)
		j = nrr - 1;

	double tl = tlt[i][j];

	NS_LOG_INFO("tl: " << tl);

	return sendPower + 135 - tl;
}


void AquaSimBellhopPropagation::readShd(std::string sender, double &dz, double &dr,
		vector<vector<double>> **tlt){
	string fileName = basePath + sender + ".shd";
	FILE *fp = fopen(fileName.c_str(), "rb");

	int recl;
	fread(&recl, sizeof(int), 1, fp);
	char title[90];
	char plotType[20];
	fread(title, sizeof(char), 80, fp);
	fseek(fp, 4 * recl, 0);
	fread(plotType, sizeof(char), 10, fp);


	int nFreq, nTheta, nsx, nsy, nsz, nrz, nrr;
	int freq0, atten;

	fseek(fp, 2 * 4 * recl, 0);
	fread(&nFreq, sizeof(int), 1, fp);
	fread(&nTheta, sizeof(int), 1, fp);
	fread(&nsx, sizeof(int), 1, fp);
	fread(&nsy, sizeof(int), 1, fp);
	fread(&nsz, sizeof(int), 1, fp);
	fread(&nrz, sizeof(int), 1, fp);
	fread(&nrr, sizeof(int), 1, fp);
	fread(&freq0, sizeof(float), 1, fp);
	fread(&atten, sizeof(float), 1, fp);


	/*vector<double> freqVec;
	fseek(fp, 3 * 4 * recl, 0);
	for (int i = 0; i < nFreq; i++) {
		double temp;
		fread(&temp, sizeof(double), 1, fp);
		freqVec.push_back(temp);
	}


	vector<float> thetaV;
	fseek(fp, 4 * 4 * recl, 0);
	for (int i = 0; i < nTheta; i++) {
		float temp;
		fread(&temp, sizeof(float), 1, fp);
		thetaV.push_back(temp);
	}

	vector<float> sx;
	fseek(fp, 5 * 4 * recl, 0);
	for (int i = 0; i < nsx; i++) {
		float temp;
		fread(&temp, sizeof(float), 1, fp);
		sx.push_back(temp);
	}

	vector<float> sy;
	fseek(fp, 6 * 4 * recl, 0);
	for (int i = 0; i < nsy; i++) {
		float temp;
		fread(&temp, sizeof(float), 1, fp);
		sy.push_back(temp);
	}

	vector<float> sz;
	fseek(fp, 7 * 4 * recl, 0);
	for (int i = 0; i < nsz; i++) {
		float temp;
		fread(&temp, sizeof(float), 1, fp);
		sz.push_back(temp);
	}*/



	vector<float> rzV;
	fseek(fp, 8 * 4 * recl, 0);
	for (int i = 0; i < nrz; i++) {
		float temp;
		fread(&temp, sizeof(float), 1, fp);
		rzV.push_back(temp);
	}


	vector<float> rrV;
	fseek(fp, 9 * 4 * recl, 0);
	for (int i = 0; i < nrr; i++) {
		float temp;
		fread(&temp, sizeof(float), 1, fp);
		rrV.push_back(temp);
	}

	dz = rzV[nrz - 1] / nrz;
	dr = rrV[nrr - 1] / nrr;

	int Nrcvrs_per_range = nrz;
	vector < vector<float> > presure(nrz, vector<float>(nrr * 2, 0));
	int recnum = 9;
	for (int i = 0; i < Nrcvrs_per_range; i++) {
		recnum += 1;
		fseek(fp, recnum * 4 * recl, 0);
		vector<float> temp(2 * nrr, 1);
		for (int j = 0; j < nrr; j++) {
			fread(&(temp[2 * j]), sizeof(float), 1, fp);
			fread(&(temp[2 * j + 1]), sizeof(float), 1, fp);
		}
		presure[i] = temp;
	}

	fclose(fp);



	int cnt = 0;
	*tlt = new vector<vector<double> >(nrz, vector<double>(nrr, 0));
	for (int i = 0; i < Nrcvrs_per_range; i++) {
		for (int j = 0; j < nrr; j++) {
			double temp = pow(presure[i][2 * j], 2)
					+ pow(presure[i][2 * j + 1], 2);
			temp = sqrt(temp);
			if (temp > 1e-37)
				cnt++;
			if (temp < 1e-37)
				temp = 1e-37;
			(**tlt)[i][j] = -20 * (log(temp) / log(10));
		}
	}



}


void AquaSimBellhopPropagation::updateEnvFile(std::string sender, double depth, double freq){

	if(depth == 0){
		depth = 100;
	}
	string fileName = basePath + sender + ".env";
	string tempFileName = basePath + sender + "temp.env";
	FILE *infile = fopen(fileName.c_str(), "r");
	ofstream outfile(tempFileName, std::ios::out);
	string line;
	char buf[1000];
	while(!feof(infile)){
		fgets(buf, 1000, infile);
		if(feof(infile))
			continue;
		line = string(buf);
		if(line.find("freq") != string::npos){
			outfile << freq << " !freq\n";
		}
		else if(line.find("sdepth") != string::npos){
			outfile  << depth << " !sdepth\n";
		}
		else{
			outfile << line;
		}
		line.clear();
	}
	fclose(infile);
	outfile.close();

	//system(("rm " + fileName).c_str());
	system(("mv " + tempFileName + " " + fileName).c_str());

}




}  // namespace ns3


