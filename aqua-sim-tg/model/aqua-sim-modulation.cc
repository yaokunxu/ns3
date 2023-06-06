/*
 * aqua-sim-modulation.cc
 *
 *  Created on: Aug 25, 2021
 *  Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */


#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

#include "aqua-sim-modulation.h"

#include <cmath>

namespace ns3
{

    NS_OBJECT_ENSURE_REGISTERED(AquaSimModulation);

    AquaSimModulation::AquaSimModulation() : m_codingEff(1), m_sps(10000), ber(0)
    {
    }

    AquaSimModulation::AquaSimModulation(int blockSize, int maxPktSize){
        this->blockSize = blockSize;
        this->maxPktSize = maxPktSize;
    }

    TypeId
    AquaSimModulation::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::AquaSimModulation")
                                .SetParent<Object>()
                                .AddAttribute("CodingEff", "The coding efficiency: number of symbols per bit.",
                                              DoubleValue(1.0),
                                              MakeDoubleAccessor(&AquaSimModulation::m_codingEff),
                                              MakeDoubleChecker<double>())
                                .AddAttribute("SPS", "The number of symbols per second.",
                                              UintegerValue(10000),
                                              MakeUintegerAccessor(&AquaSimModulation::m_sps),
                                              MakeUintegerChecker<uint32_t>());
        return tid;
    }

    double
    AquaSimModulation::TxTime(int pktSize)
    {
        int blockNum = pktSize / blockSize;
        if(blockNum * blockNum != pktSize)
            blockNum++;
        
        return blockNum * (blockTime + guardTime) + guardTime;

    }


    double
    AquaSimModulation::Per(int pktSize)
    {
        return 1 - std::pow(ber, pktSize);
    }

} // namespace ns3
