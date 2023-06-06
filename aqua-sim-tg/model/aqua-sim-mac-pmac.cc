//
// Created by chhhh on 2021/2/20.
//
/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *
 * Date:2021.4.7
 * Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */

#include <iostream>
#include <algorithm>
#include "aqua-sim-mac-pmac.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-header-pmac.h"
#include "aqua-sim-trailer.h"
#include "aqua-sim-header-mac.h"

#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/integer.h"
#include "ns3/double.h"


namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("AquaSimPmac");
    NS_OBJECT_ENSURE_REGISTERED(AquaSimPmac);



    AquaSimPmac::AquaSimPmac() :
            AquaSimMac(), PMAC_Status(WAIT_PROBE), m_AckOn(1), nextSendPktId(0),
            m_ack(0), nextRecvPktId(0), m_maxDataPktSize(400),
            timeSlot(0), nextHopSlot(0), endNode(10), begNode(1)
    {
        Simulator::Schedule(Seconds(0.1), &AquaSimPmac::init, this);
        m_rand = CreateObject<UniformRandomVariable>();//unuse
    }


    void AquaSimPmac::init(){
        startTime = Simulator::Now();
        token = AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() % 3; // node token
    	NS_LOG_DEBUG("mytoken" << token);
        if(false){
            PMAC_Status = PASSIVE;
            timeSlot = Seconds(15);
        }
        else if(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() == 1){
            PMAC_Status = WAIT_ACK_PROBE;
            sendInitPacket(makeProbe());
        }
    }

    void AquaSimPmac::retryInitPacket(Ptr<Packet> pkt){
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        pkt->RemoveHeader(pmacHeader);
        pkt->RemoveHeader(pmacInitHeader);
        
        switch (pmacHeader.GetPktType())
        {
        case PmacHeader::PROBE:
            pmacInitHeader.sendProbe = Simulator::Now().GetInteger();
            break;
        case PmacHeader::TIME_ALIGN:
            pmacInitHeader.sendTimeAlignToStartTime = Simulator::Now().GetInteger() - startTime.GetInteger();
            break;
        default:
            break;
        }

        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);

        sendInitPacket(pkt);
    }
    

    void AquaSimPmac::sendInitPacket(Ptr<Packet> pkt){
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;


        pkt->RemoveHeader(pmacHeader);
        pkt->RemoveHeader(pmacInitHeader);
        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);

        NS_LOG_DEBUG("send init pkt " 
        << AquaSimAddress::ConvertFrom(m_device->GetAddress()) 
        << " "<< pmacHeader.GetPktType());


        Ptr<Packet> pktCopy = pkt->Copy();

        switch (m_device->GetTransmissionStatus()) {
            case SLEEP:
                PowerOn();
            case NIDLE:
                SendDown(pkt);
                break;
            default:
                break;
        }
        retryId = Simulator::Schedule(Seconds(20), &AquaSimPmac::retryInitPacket, this, pktCopy);
    }

    void AquaSimPmac::sendInitAckPacket(Ptr<Packet> pkt){
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;

        pkt->RemoveHeader(pmacHeader);
        pkt->RemoveHeader(pmacInitHeader);
        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);

        NS_LOG_DEBUG("send init ack pkt " 
        << AquaSimAddress::ConvertFrom(m_device->GetAddress()) 
        << " "<< pmacHeader.GetPktType());


        switch (m_device->GetTransmissionStatus()) {
            case SLEEP:
                PowerOn();
            case NIDLE:
                SendDown(pkt);
                break;
            default:
                break;
        }
    }


    bool AquaSimPmac::recvInitPacket(Ptr<Packet> pkt){
        PmacHeader pmacHeader;
        pkt->PeekHeader(pmacHeader);

        NS_LOG_DEBUG("recv init pkt " 
        << AquaSimAddress::ConvertFrom(m_device->GetAddress()) 
        << " "<< pmacHeader.GetPktType());

        switch (pmacHeader.GetPktType())
        {
        case PmacHeader::PROBE:
            processProbe(pkt);
            break;
        case PmacHeader::ACK_PROBE:
            processAckProbe(pkt);
            break;
        case PmacHeader::TRIGGER:
            processTrigger(pkt);
            break;
        case PmacHeader::ACK_TRIGGER:
            processAckTrigger(pkt);
            break;
        case PmacHeader::TIME_ALIGN:
            processTimeAlign(pkt);
            break;
        case PmacHeader::ACK_TIME_ALIGN:
            processAckTimeAlign(pkt);
            break;
        default:
            NS_LOG_WARN("MAC-PMAC-unknown init packet type");
        }

        return true;
    }

    void AquaSimPmac::processProbe(Ptr<Packet> pkt){
        PMAC_Status = WAIT_TRIGGER;
        
        sendInitAckPacket(makeAckProbe(pkt));

    }

	void AquaSimPmac::processAckProbe(Ptr<Packet> pkt){
        if(PMAC_Status != AquaSimPmac::WAIT_ACK_PROBE)
            return;

        retryId.Cancel();
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        pkt->RemoveHeader(pmacHeader);
        pkt->RemoveHeader(pmacInitHeader);
        pmacInitHeader.recvAckProbe = Simulator::Now().GetInteger();
        
        nextHopSlot = Time(((pmacInitHeader.recvAckProbe - pmacInitHeader.sendProbe) - 
                        (pmacInitHeader.sendAckProbe - pmacInitHeader.recvProbe)) / 2 );

        timeSlot = Time(std::max(timeSlot.GetInteger(), nextHopSlot.GetInteger()));

        NS_LOG_DEBUG("MAC-PMAC-" << AquaSimAddress::ConvertFrom(m_device->GetAddress())
                << " nextHopSlot " << nextHopSlot
                << " timeSlot " << timeSlot);


        pkt = 0;

        PMAC_Status = AquaSimPmac::WAIT_ACK_TRIGGER;
        sendInitPacket(makeTrigger());

        
    }
	void AquaSimPmac::processTrigger(Ptr<Packet> pkt){
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        pkt->RemoveHeader(pmacHeader);
        pkt->RemoveHeader(pmacInitHeader);
        pkt = 0;

        timeSlot = Time(pmacInitHeader.maxSlot);


        Ptr<Packet> ack = makeAckTrigger();
        Ptr<Packet> ackCopy = ack->Copy();
        sendInitAckPacket(ack);


        if(PMAC_Status == AquaSimPmac::WAIT_TRIGGER){
            if(AquaSimAddress::ConvertFrom(m_device->GetAddress()) == endNode){
                PMAC_Status = AquaSimPmac::WAIT_ACK_TIME_ALIGN;
                Simulator::Schedule(GetTxTime(ackCopy) + Seconds(0.5), 
                        &AquaSimPmac::retryInitPacket, this, makeTimeAlign());
            }
            else{
                PMAC_Status = AquaSimPmac::WAIT_ACK_PROBE;
                Simulator::Schedule(GetTxTime(ackCopy) + Seconds(0.5), 
                        &AquaSimPmac::retryInitPacket, this, makeProbe());
            }
        }
        ackCopy = 0;
    }

	void AquaSimPmac::processAckTrigger(Ptr<Packet> pkt){

        if(PMAC_Status != AquaSimPmac::WAIT_ACK_TRIGGER)
            return;

        retryId.Cancel();
        PMAC_Status = WAIT_TIME_ALIGN;
        pkt = 0;

    }
	void AquaSimPmac::processTimeAlign(Ptr<Packet> pkt){
        if(PMAC_Status == WAIT_ACK_TRIGGER){
            retryId.Cancel();
        }


        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        pkt->RemoveHeader(pmacHeader);
        pkt->RemoveHeader(pmacInitHeader);


        if(PMAC_Status == AquaSimPmac::WAIT_TIME_ALIGN){
            timeSlot = Time(pmacInitHeader.maxSlot);
            startTime = Time(Simulator::Now() - nextHopSlot - pmacInitHeader.sendTimeAlignToStartTime);


            NS_LOG_DEBUG("MAC-PMAC-" << AquaSimAddress::ConvertFrom(m_device->GetAddress())
                << " timeSlot " << timeSlot
                << " startTime " <<startTime);
        }

        Ptr<Packet> ack = makeAckTimeAlign();
        Ptr<Packet> ackCopy = ack->Copy();
        sendInitAckPacket(ack);

        if(PMAC_Status != AquaSimPmac::WAIT_TIME_ALIGN)
            return;

        if(AquaSimAddress::ConvertFrom(m_device->GetAddress()) == begNode){
            PMAC_Status = AquaSimPmac::PASSIVE;
            SendDataPkt();

        }
        else{
            PMAC_Status = AquaSimPmac::WAIT_ACK_TIME_ALIGN;
            Simulator::Schedule(GetTxTime(ackCopy) + Seconds(0.5), 
                    &AquaSimPmac::retryInitPacket, this, makeTimeAlign());
        }

        ackCopy = 0;
    }


	void AquaSimPmac::processAckTimeAlign(Ptr<Packet> pkt){
        if(PMAC_Status != WAIT_ACK_TIME_ALIGN)
            return;
        
        retryId.Cancel();
        pkt = 0;
        PMAC_Status = PASSIVE;
        SendDataPkt();
        
    }

    Ptr<Packet> AquaSimPmac::makeProbe(){
        Ptr <Packet> pkt = Create<Packet>(500);
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        AquaSimTrailer ast;

        pmacHeader.SetDA(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() + 1);
        pmacHeader.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
        pmacHeader.SetPktType(PmacHeader::PROBE);
        ast.SetModeId(1);
        pmacInitHeader.sendProbe = Simulator::Now().GetInteger();

        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);
        pkt->AddTrailer(ast);
        return pkt;
    }

    Ptr<Packet> AquaSimPmac::makeAckProbe(Ptr<Packet> pkt){
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        AquaSimTrailer ast;

        pkt->RemoveHeader(pmacHeader);
        pkt->RemoveHeader(pmacInitHeader);

        pmacInitHeader.recvProbe = Simulator::Now().GetInteger();
        pmacHeader.SetPktType(PmacHeader::ACK_PROBE);
        pmacHeader.SetDA(pmacHeader.GetSA());
        pmacHeader.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
        pmacInitHeader.sendAckProbe = Simulator::Now().GetInteger();

        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);
        return pkt;
    }

	Ptr<Packet> AquaSimPmac::makeTrigger(){
        Ptr <Packet> pkt = Create<Packet>();
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        AquaSimTrailer ast;

        pmacHeader.SetDA(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() + 1);
        pmacHeader.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
        pmacHeader.SetPktType(PmacHeader::TRIGGER);
        ast.SetModeId(1);
        pmacInitHeader.maxSlot = timeSlot.GetInteger();

        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);
        pkt->AddTrailer(ast);

        return pkt;
    }
	Ptr<Packet> AquaSimPmac::makeAckTrigger(){
        Ptr <Packet> pkt = Create<Packet>();
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        AquaSimTrailer ast;

        pmacHeader.SetDA(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() - 1);
        pmacHeader.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
        pmacHeader.SetPktType(PmacHeader::ACK_TRIGGER);
        ast.SetModeId(1);

        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);
        pkt->AddTrailer(ast);

        return pkt;
    }

	Ptr<Packet> AquaSimPmac::makeTimeAlign(){
        Ptr<Packet> pkt = Create<Packet>(500);
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        AquaSimTrailer ast;

        pmacHeader.SetDA(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() - 1);
        pmacHeader.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
        pmacHeader.SetPktType(PmacHeader::TIME_ALIGN);
        ast.SetModeId(1);
        if(AquaSimAddress::ConvertFrom(m_device->GetAddress()) == endNode){
            startTime = Simulator::Now();
            NS_LOG_DEBUG("MAC-PMAC-" << AquaSimAddress::ConvertFrom(m_device->GetAddress())
                << " timeSlot " << timeSlot
                << " startTime " <<startTime);
        }
        pmacInitHeader.maxSlot = timeSlot.GetInteger();
        pmacInitHeader.sendTimeAlignToStartTime = Simulator::Now().GetInteger() - startTime.GetInteger();

        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);
        pkt->AddTrailer(ast);
        return pkt;
    }

	Ptr<Packet> AquaSimPmac::makeAckTimeAlign(){
        Ptr <Packet> pkt = Create<Packet>();
        PmacHeader pmacHeader;
        PmacInitHeader pmacInitHeader;
        AquaSimTrailer ast;

        pmacHeader.SetDA(AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() + 1);
        pmacHeader.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
        pmacHeader.SetPktType(PmacHeader::ACK_TIME_ALIGN);
        ast.SetModeId(1);

        pkt->AddHeader(pmacInitHeader);
        pkt->AddHeader(pmacHeader);
        pkt->AddTrailer(ast);
        return pkt;
    }


    AquaSimPmac::~AquaSimPmac() {
    }



    TypeId
    AquaSimPmac::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::AquaSimPmac")
                .SetParent<AquaSimMac>()
                .AddConstructor<AquaSimPmac>()
                .AddAttribute("AckOn", "If acknowledgement is on",
                              IntegerValue(1),
                              MakeIntegerAccessor(&AquaSimPmac::m_AckOn),
                              MakeIntegerChecker<int>());
        return tid;
    }

    int64_t
    AquaSimPmac::AssignStreams(int64_t stream) {
        NS_LOG_FUNCTION(this << stream);
        m_rand->SetStream(stream);
        return 1;
    }

/*===========================Send and Receive===========================*/

    bool AquaSimPmac::TxProcess(Ptr <Packet> pkt) {
        NS_LOG_FUNCTION(m_device->GetAddress() << pkt << Simulator::Now().GetSeconds());
        AquaSimTrailer ast;
        PmacHeader pmacHeader;
        pkt->PeekTrailer(ast);

        pmacHeader.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
        pmacHeader.SetDA(ast.GetNextHop());

        pmacHeader.SetPktType((PmacHeader::DATA));
        pmacHeader.SetPktId(nextSendPktId);
        nextSendPktId = 1 - nextSendPktId;
        pkt->AddHeader(pmacHeader);

        PktQ_.push(pkt);
        if(PMAC_Status == PASSIVE)
        	SendDataPkt();

        return true;
    }


    Time AquaSimPmac::makeTime(){ //calculate next send time
        int num = (int)((Simulator::Now() - startTime) / timeSlot);
        int nowT = num % 3;
        int interval = (token - nowT + 3) % 3;
        if(interval == 0)
            interval = 3;
        Time t = interval * timeSlot + num * timeSlot + startTime - Simulator::Now();
        return t;
    }

    void AquaSimPmac::SendDataPkt() {
        NS_LOG_FUNCTION(this);
        if (PMAC_Status == PASSIVE)
        {
            Ptr <Packet> pkt = 0;
            if(!PktQ_.empty()){ // send data
                PMAC_Status = SEND_DATA;
                pkt = PktQ_.front()->Copy();

                PmacHeader pmacHeader;
                pkt->RemoveHeader(pmacHeader);
                if(m_ack){ // judge implicit ack
                    pmacHeader.SetPktType(PmacHeader::DATA_ACK);
                    m_ack = 0;
                }
                pkt->AddHeader(pmacHeader);
                Time t = makeTime();
                Simulator::Schedule(t, &AquaSimPmac::SendPkt, this, pkt);
            }
            else if(m_ack == 1){  //explicit ack
                PMAC_Status = SEND_DATA;
                pkt = MakeACK();
                m_ack = 0;
                Time t = makeTime();
                Simulator::Schedule(t, &AquaSimPmac::SendPkt, this, pkt);
            }
        }
    }

    void AquaSimPmac::waitToPassive(){
    	PMAC_Status = PASSIVE;
    	SendDataPkt();
    }

    void AquaSimPmac::SendPkt(Ptr <Packet> pkt) {
        NS_LOG_FUNCTION(this);
        PmacHeader pmacHeader;
        pkt->RemoveHeader(pmacHeader);
        pkt->AddHeader(pmacHeader);
        
        /*if(pmacHeader.GetPktType() == PmacHeader::ACK)
        	std::cout << "ACK";
    	std::cout << m_device->GetAddress();
    	std::cout << "SendPkt to";
        std::cout << pmacHeader.GetDA();
        std::cout << "in";
        std::cout << (int)((Simulator::Now() - startTime) / timeSlot);
        std::cout << std::endl;*/
       

        switch (m_device->GetTransmissionStatus()) {
            case SLEEP:
                PowerOn();

            case NIDLE: {
                if (pmacHeader.GetPktType() <= PmacHeader::DATA_ACK ) { //data
                    if (m_AckOn) {///m_AckOn标识是否需要ack, always is true
                        PMAC_Status = WAIT_ACK;
                    } else {
                        if (!PktQ_.empty()) {
                            PktQ_.front() = 0;
                            PktQ_.pop();
                        }
                        PMAC_Status = PASSIVE;
                    }
                }
                if(pmacHeader.GetPktType() >= PmacHeader::DATA_ACK) // ack
                {
                    m_ack = 0;
                    if(PMAC_Status == SEND_DATA)
                    	PMAC_Status = PASSIVE;
                }
                SendDown(pkt);
                Simulator::Schedule(2 * timeSlot, &AquaSimPmac::waitToPassive, this);
                break;
            }
            default:
            	break;
        }
    }

    bool AquaSimPmac::RecvProcess(Ptr <Packet> pkt) {
        NS_LOG_FUNCTION(m_device->GetAddress());

        PmacHeader pmacHeader;
        pkt->RemoveHeader(pmacHeader);


        if(pmacHeader.GetPktType() >= PmacHeader::PROBE){
            if(pmacHeader.GetDA() == AquaSimAddress::ConvertFrom(m_device->GetAddress())){
                pkt->AddHeader(pmacHeader);
                recvInitPacket(pkt);
            }
            return true;
        }



        AquaSimAddress recver = pmacHeader.GetDA();
        AquaSimAddress myAddr = AquaSimAddress::ConvertFrom(m_device->GetAddress());


        if (pmacHeader.GetPktType() >= PmacHeader::DATA_ACK && PMAC_Status == WAIT_ACK) { /// 有ack
            if((pmacHeader.GetPktType() == PmacHeader::DATA_ACK && recver != myAddr) ||
                    (pmacHeader.GetPktType() == PmacHeader::ACK && recver == myAddr)){
                if (!PktQ_.empty()) {
                    PktQ_.front() = 0;
                    PktQ_.pop();
                }
                PMAC_Status = PASSIVE;

            }
        }
        if (pmacHeader.GetPktType() <= PmacHeader::DATA_ACK) { ///有数据
            if (recver == myAddr) {
                preNode = pmacHeader.GetSA();
                if(nextRecvPktId == pmacHeader.GetPktId()) {
                    auto cpkt = pkt->Copy();
                    nextRecvPktId = 1 - nextRecvPktId;
                    SendUp(cpkt);
                }
                m_ack = 1;
                Simulator::Schedule(timeSlot/10, &AquaSimPmac::SendDataPkt, this);
            }
        }
        //std::cout << std::endl;
        pkt = 0;
        return true;
    }


    Ptr <Packet> AquaSimPmac::MakeACK() {
        NS_LOG_FUNCTION(this);
        //wouldn't it make more sense to just include aloha header SA instead of pkt for parameters?
        PmacHeader pmacHeader;
        AquaSimTrailer ast;

        Ptr <Packet> pkt = Create<Packet>();
        pmacHeader.SetDA(preNode);
        pmacHeader.SetSA(AquaSimAddress::ConvertFrom(m_device->GetAddress()));
        pmacHeader.SetPktType(PmacHeader::ACK);
        
        ast.SetModeId(1);

        pkt->AddHeader(pmacHeader);
        pkt->AddTrailer(ast);
        return pkt;
    }

    uint32_t AquaSimPmac::getHeaderSize(){
        PmacHeader pmacHeader;
        return pmacHeader.GetSerializedSize();
    }

    void AquaSimPmac::DoDispose() {
        NS_LOG_FUNCTION(this);
        while (!PktQ_.empty()) {
            PktQ_.front() = 0;
            PktQ_.pop();
        }
        m_rand = 0;
        AquaSimMac::DoDispose();
    }

} // namespace ns3
