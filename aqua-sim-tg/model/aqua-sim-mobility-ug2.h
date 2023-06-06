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

#ifndef UNDERWATER_GILDER_MOBILITY_MODEL2_H
#define UNDERWATER_GILDER_MOBILITY_MODEL2_H

// #include "aqua-sim-mobility-pattern.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/rectangle.h"
#include "ns3/random-variable-stream.h"
#include "ns3/mobility-model.h"
#include "ns3/constant-velocity-helper.h"

namespace ns3
{

    /**
     * \ingroup aqua-sim-ng
     *
     * \brief Underwater gilder mobility model.
     *
     * This model is used to simulate specific movement characteristics of
     * underwater gliders.
     * The glider performs 30-50 open-angle sawtooth movements in water
     * depths of 4,500 meters.  The glider is free to set the glide
     * depth range, Angle and speed.
     */
    class UnderwaterGilderMobilityModel2 : public MobilityModel
    {
    public:
        /**
         * Register this type with the TypeId system.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);
        UnderwaterGilderMobilityModel2();
        virtual ~UnderwaterGilderMobilityModel2();

        /*AquaSimMobilityUG();
        static TypeId GetTypeId(void);
        virtual LocationCacheElem GenNewLoc();
        virtual void Init();*/

        /**
         * Initialize the model and calculate new velocity, direction, and pitch
         */
        void Start(void);
        void Init(void);
        void DoWalk(Time delayLeft);
        /**
         * Set new direction(2D), and schedule next pause event
         */
        // void ResetDirection (void);
        /**
         * Set new open angle, and schedule next pause event
         */
        // void ResetPitch (void);
        /**
         * Set new direction, and schedule next pause event
         * \param direction (radians)
         */
        // void SetDirection (double direction);

        virtual void DoDispose(void);
        virtual void DoInitialize(void);
        virtual Vector DoGetPosition(void) const;
        virtual void DoSetPosition(const Vector &position);
        virtual Vector DoGetVelocity(void) const;
        // inline void PrepareNextPoint();
        ConstantVelocityHelper DoGetMHelper() const;
        Rectangle DoGetMBounds() const;
        double DoGetMDirection() const;
        double DoGetMSpeed() const;

    private:
        bool m_isVarSpeed;
        double m_speed;
        double m_direction;
        Ptr<UniformRandomVariable> m_rndDirection; //!< rv to control direction
        Ptr<UniformRandomVariable> m_rndSpeed;     //!< a random variable to control speed
        Ptr<ConstantRandomVariable> m_pause;       //!< a random variable to control pause
        Rectangle m_bounds;                        //!< the 2D bounding area
        EventId m_event;                           //!< event ID of next scheduled event
        ConstantVelocityHelper m_helper;           //!< helper for velocity computations

        /*
            double m_destX;  //the coordinate of next way point
            double m_destY;
            double m_destZ;
            double m_originalX;  //the coordinate of previous way point
            double m_originalY;
            double m_originalZ;

            double m_x;
            double m_y;
            double m_z;*/

        /* the ratio between dimensions and the distance between
         * previous way point and next way point
         */
        /*
        double m_ratioX;
        double m_ratioY;
        double m_ratioZ;

        double m_speed; //for speed of the node


        double m_maxSpeed, m_minSpeed;
        double m_maxThinkTime; //the max time for thinking where to go after reaching a dest
        double m_thinkTime;

        double m_distance;     //the distance to next point
        double m_startTime;   //the time when this node start to next point
        double m_thoughtTime; //the time taken by deciding where I will go

        bool   m_duplicatedNamTrace;*/
    }; // class UnderwaterGilderMobilityModel

} // namespace ns3

#endif /* UNDERWATER_GILDER_MOBILITY_MODEL_H */