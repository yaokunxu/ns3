/*
 * aqua-sim-propagation-bellhop.h
 *
 *  Created on: May 8, 2021
 *  Author: chenhao <chenhao2118@mails.jlu.edu.cn>
 */

#ifndef SRC_AQUA_SIM_NG_MODEL_AQUA_SIM_PROPAGATION_BELLHOP_H_
#define SRC_AQUA_SIM_NG_MODEL_AQUA_SIM_PROPAGATION_BELLHOP_H_


#include "aqua-sim-propagation.h"
#include "ns3/event-id.h"
#include <string>
#include <vector>


namespace ns3 {

    class Packet;

    /**
     * \ingroup aqua-sim-ng
     *
     * \brief Simple propagation model. This propagation model calculates attenuation using rayleigh model and allows all nodes in the network to receive a copy.
     *   Depenedent on transmission factors (range/power/noise/etc.) other parts of simulator will determine if copy is decodable.
     */
    class AquaSimBellhopPropagation : public AquaSimPropagation {
    public:
        static TypeId GetTypeId(void);

        AquaSimBellhopPropagation(void);

        ~AquaSimBellhopPropagation(void);

        virtual std::vector <PktRecvUnit> *ReceivedCopies(Ptr <AquaSimNetDevice> s,
                                                          Ptr <Packet> p,
                                                          std::vector <Ptr<AquaSimNetDevice>> dList);


        virtual double GETpr(Ptr <AquaSimNetDevice> s,
                             double freq, double pt,
                             Ptr <AquaSimNetDevice> dList);

    protected:


        void bellhop(int senderID, double senderDepth, double freq,
                     double &dz, double &dr, std::vector <std::vector<double>> **tlt);

        void getRangeDepth(Vector sendPos, Ptr <MobilityModel> recvModel, double &range, double &recverDepth);

        double getRecvPower(std::vector <std::vector<double>> &tlt, Ptr <MobilityModel> sendModel,
                            Ptr <MobilityModel> recvModel, double sendPower, double dz, double dr);

        void updateEnvFile(std::string sender, double depth, double freq);

        void readShd(std::string sender, double &dz, double &dr, std::vector <std::vector<double>> **tlt);

    private:
        std::string basePath;

    };  //class AquaSimSimplePropagation

}  // namespace ns3


#endif /* SRC_AQUA_SIM_NG_MODEL_AQUA_SIM_PROPAGATION_BELLHOP_H_ */
