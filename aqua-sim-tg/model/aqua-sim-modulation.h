/*
 * aqua-sim-modulation.h
 *
 *  Created on: Aug 25, 2021
 *  Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */

#ifndef AQUA_SIM_MODULATION_H
#define AQUA_SIM_MODULATION_H

#include "ns3/object.h"

namespace ns3
{

    /**
 * \ingroup aqua-sim-ng
 *
 * \brief Modulation class to assist channel/physical simulation.
 */
    class AquaSimModulation : public Object
    {

    public:
        static TypeId GetTypeId(void);

        AquaSimModulation();
        AquaSimModulation(int blockSize, int pktSize);
        virtual ~AquaSimModulation() {}

        /*
   *  Get transmission time by packet size (expected in bits).
   */
        virtual double TxTime(int pktSize);


        /*
   *  Give the packet error rate of a packet of size pktsize
   */
        virtual double Per(int pktSize);

        /*
   *  Returns number bits per second
   */
        virtual double Bps() { return m_sps / m_codingEff; }

    protected:
        /*
   *  Preamble of physical frame
   */
        double m_codingEff; //coding efficiency: number of symbols per bit
        int m_sps;          //number of symbols per second
        double ber;       //bit error rate

        int maxPktSize;
        int blockSize;

        double blockTime = 0.17;
        double guardTime = 0.16;

    }; // AquaSimModulation

} // namespace ns3

#endif /* AQUA_SIM_MODULATION_H */
