/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Jilin University
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
 * Author: ** ******* <****@mails.jlu.edu.cn>
 */

#ifndef UNDERWATER_GILDER_MOBILITY_MODEL_H
#define UNDERWATER_GILDER_MOBILITY_MODEL_H

// #include "aqua-sim-mobility-pattern.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/rectangle.h"
#include "ns3/random-variable-stream.h"
#include "ns3/mobility-model.h"
#include "constant-velocity-helper.h"
#include "mutex"

namespace ns3{
class WaveUtil;
/**
 * \ingroup aqua-sim-ng
 *
 * \brief Underwater gilder mobility model.
 * 
 * This model is used to simulate specific movement characteristics of 
 * wave gliders.
 * Wave gliders use waves to achieve passive motion on the surface of the water. 
 * 
 */
class WaveGilderMobilityModel : public MobilityModel {
public:
    /**
     * Register this type with the TypeId system.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    WaveGilderMobilityModel();
    virtual ~WaveGilderMobilityModel();

private:
    /**
     * Initialize the model and calculate new velocity, direction, and pitch
     */
    void Start (void);
    void Init (void);
    void DoWalk (Time delayLeft);

    virtual void DoDispose (void);
    virtual void DoInitialize (void);
    virtual Vector DoGetPosition (void) const;
    virtual void DoSetPosition (const Vector &position);
    virtual Vector DoGetVelocity (void) const;
    virtual int64_t DoAssignStreams (int64_t);
	// inline void PrepareNextPoint();

private:
    bool m_isVarSpeed;

    double m_speed;
    double m_direction;
    double m_pitch; //!< cur pitching angle
    WaveUtil* m_waveModel;

    Ptr<UniformRandomVariable> m_rndDirection; //!< rv to control direction
    Ptr<UniformRandomVariable> m_rndSpeed; //!< a random variable to control speed
    Ptr<ConstantRandomVariable> m_pause; //!< a random variable to control pause 
    Rectangle m_bounds; //!< the 2D bounding area
    EventId m_event; //!< event ID of next scheduled event
    ConstantVelocityHelper m_helper; //!< helper for velocity computations
    Time m_lastupdate;

};  // class WaveGilderMobilityModel
class WaveUtil {
private:
    WaveUtil();
    WaveUtil(const WaveUtil &) = delete;
    WaveUtil &operator=(const WaveUtil &) = delete;
public:
    ~WaveUtil() {}
    static WaveUtil* GetInstance() {
        if (single != nullptr)  return single;
        s_mutex.lock();
        if (single != nullptr) {
            s_mutex.unlock();
            return single;
        }
        single = new WaveUtil();
        s_mutex.unlock();
        return single;
    }
    double GetHeight(double x, double y, Time t);

private:
    void Init();
    double DirectionSpectrum(double omega = 0, double theta = 0);
    double WaveSpectrum(double omega);
    double WaveDirection(double theta);
    int wave_div_num;   // <! wave division number
    int dire_div_num;   // <! direction division number
    double beta1;
    double beta2;
    double g, U;
    std::vector<double> omegas;
    std::vector<double> thetas;
    std::vector<std::vector<double>> epsilon;
    std::vector<std::vector<double>> alpha;
    double delta_omega;
    double delta_theta;

    static WaveUtil* single;
    static std::mutex s_mutex;
};
}// namespace ns3

#endif /* WAVE_GILDER_MOBILITY_MODEL_H */
