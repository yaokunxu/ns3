#include "ns3/simulator.h"
#include <algorithm>
#include <cmath>
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "aqua-sim-mobility-ug2.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("UnderwaterGilderMobilityModel2");

    NS_OBJECT_ENSURE_REGISTERED(UnderwaterGilderMobilityModel2);

    TypeId
    UnderwaterGilderMobilityModel2::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::UnderwaterGilderMobilityModel2")
                                .SetParent<MobilityModel>()
                                .SetGroupName("Mobility")
                                .AddConstructor<UnderwaterGilderMobilityModel2>()
                                .AddAttribute("VariableSpeed", "Set gilder speed is Variable. Default false.",
                                              BooleanValue(0),
                                              MakeBooleanAccessor(&UnderwaterGilderMobilityModel2::m_isVarSpeed),
                                              MakeBooleanChecker())
                                .AddAttribute("Bounds", "The 2d bounding area",
                                              RectangleValue(Rectangle(-100, 100, -100, 100)),
                                              MakeRectangleAccessor(&UnderwaterGilderMobilityModel2::m_bounds),
                                              MakeRectangleChecker())
                                // .AddAttribute ("minOpenAngle", "set the min open angle.",
                                //                 DoubleValue(0.0),
                                //                 MakeDoubleAccessor(&UnderwaterGilderMobilityModel2::min_openAngle),
                                //                 MakeDoubleChecker<double>())
                                // .AddAttribute ("maxOpenAngle", "set the max open angle.",
                                //                 DoubleValue(0.0),
                                //                 MakeDoubleAccessor(&UnderwaterGilderMobilityModel2::max_openAngle),
                                //                 MakeDoubleChecker<double>())
                                // .AddAttribute ("minDepth", "set the min depth.",
                                //                 DoubleValue(0.0),
                                //                 MakeDoubleAccessor(&UnderwaterGilderMobilityModel2::min_depth),
                                //                 MakeDoubleChecker<double>())
                                // .AddAttribute ("maxDepth", "set the max depth.",
                                //                 DoubleValue(0.0),
                                //                 MakeDoubleAccessor(&UnderwaterGilderMobilityModel2::max_depth),
                                //                 MakeDoubleChecker<double>())
                                .AddAttribute("Speed", "A random variable to control the speed (m/s).",
                                              StringValue("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
                                              MakePointerAccessor(&UnderwaterGilderMobilityModel2::m_rndSpeed),
                                              MakePointerChecker<UniformRandomVariable>())
                                .AddAttribute("Pause", "A random variable to control the pause (s).",
                                              StringValue("ns3::ConstantRandomVariable[Constant=0.1]"),
                                              MakePointerAccessor(&UnderwaterGilderMobilityModel2::m_pause),
                                              MakePointerChecker<ConstantRandomVariable>())
                                .AddAttribute("Direction",
                                              "A random variable used to assign the direction.",
                                              StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283185307]"),
                                              MakePointerAccessor(&UnderwaterGilderMobilityModel2::m_rndDirection),
                                              MakePointerChecker<UniformRandomVariable>());
        return tid;
    }

    UnderwaterGilderMobilityModel2::UnderwaterGilderMobilityModel2()
    {
        NS_LOG_FUNCTION(this);
        Simulator::ScheduleNow(&UnderwaterGilderMobilityModel2::Init, this);
    }

    UnderwaterGilderMobilityModel2::~UnderwaterGilderMobilityModel2()
    {
    }
    void
    UnderwaterGilderMobilityModel2::Init()
    {
        NS_LOG_FUNCTION(this);
        m_speed = 0.0;
        m_direction = 0.0;
        m_event = Simulator::ScheduleNow(&UnderwaterGilderMobilityModel2::Start, this);
        m_helper.Unpause();
    }
    void
    UnderwaterGilderMobilityModel2::DoDispose(void)
    {
        MobilityModel::DoDispose();
    }

    void
    UnderwaterGilderMobilityModel2::DoInitialize(void)
    {
        // ResetDirectionAndSpeed ();
        MobilityModel::DoInitialize();
    }

    Vector
    UnderwaterGilderMobilityModel2::DoGetPosition(void) const
    {
        m_helper.UpdateWithBounds(m_bounds);
        return m_helper.GetCurrentPosition();
    }
    void
    UnderwaterGilderMobilityModel2::DoSetPosition(const Vector &position)
    {
        m_helper.SetPosition(position);
        Simulator::Remove(m_event);
        m_event.Cancel();
        m_event = Simulator::ScheduleNow(&UnderwaterGilderMobilityModel2::Start, this);
    }
    Vector
    UnderwaterGilderMobilityModel2::DoGetVelocity(void) const
    {
        return m_helper.GetVelocity();
    }

    void
    UnderwaterGilderMobilityModel2::Start(void)
    {
        NS_LOG_FUNCTION(this);
        if (m_speed == 0.0)
        {
            m_speed = m_rndSpeed->GetValue();
            m_direction = m_rndDirection->GetValue();
            // Default slide down at the beginning
            double cosD = std::cos(m_direction);
            double sinD = std::sin(m_direction);
            // Set the velocity vector to give to the constant velocity helper
            m_helper.SetVelocity(Vector(m_speed * cosD, m_speed * sinD, 0));
        }
        NS_LOG_INFO("Mobility current speed is " << m_speed
                                                 << ", current direction is " << m_direction
                                                 << ".");
        m_helper.Update();
        m_helper.Unpause();

        Time delay = Seconds(m_pause->GetValue());
        DoWalk(delay);
    }

    void
    UnderwaterGilderMobilityModel2::DoWalk(Time delayLeft)
    {
        NS_LOG_FUNCTION(this << delayLeft);
        m_helper.UpdateWithBounds(m_bounds);
        Vector position = m_helper.GetCurrentPosition();
        Vector velocity = m_helper.GetVelocity();
        Vector nextPos = position;
        nextPos.x += velocity.x * delayLeft.GetSeconds();
        nextPos.y += velocity.y * delayLeft.GetSeconds();

        // Make sure that the position by the next time step is still within the boundary.
        // If out of bounds, then alter the velocity vector and average direction to keep the position in bounds

        NS_LOG_INFO(Simulator::Now().GetSeconds() << "s, next postition is " << nextPos << ".");
        // Check whether the node is out of bounds(2D)
        bool keepOriginalState = true;
        if (!m_bounds.IsInside(nextPos))
        {
            NS_LOG_DEBUG("Underwater gilder mobility has been moved to the boundary. Next position is " << nextPos);
            keepOriginalState = false;
            if (nextPos.x > m_bounds.xMax || nextPos.x < m_bounds.xMin)
            {
                velocity.x = -velocity.x;
                m_direction = M_PI - m_direction;
                if (m_direction < 0)
                    m_direction += 2 * M_PI;
            }
            if (nextPos.y > m_bounds.yMax || nextPos.y < m_bounds.yMin)
            {
                velocity.y = -velocity.y;
                m_direction = -m_direction;
                if (m_direction < 0)
                    m_direction += 2 * M_PI;
            }
            m_helper.SetVelocity(velocity);
            m_helper.Unpause();
        }
        // Check whether the node is out of depth(random change)

        double cosD = std::cos(m_direction);
        double sinD = std::sin(m_direction);
        velocity.x = m_speed * cosD;
        velocity.y = m_speed * sinD;
        m_helper.SetVelocity(velocity);
        m_helper.Unpause();

        m_event = Simulator::Schedule(delayLeft, &UnderwaterGilderMobilityModel2::Start, this);
        if (keepOriginalState == false)
        {
            // NotifyCourseChange();
        }
        NotifyCourseChange();
    }
    ConstantVelocityHelper
    UnderwaterGilderMobilityModel2::DoGetMHelper() const
    {
        return m_helper;
    }
    Rectangle UnderwaterGilderMobilityModel2::DoGetMBounds() const
    {
        return m_bounds;
    }
    double UnderwaterGilderMobilityModel2::DoGetMDirection() const
    {
        return m_direction;
    }
    double UnderwaterGilderMobilityModel2::DoGetMSpeed() const
    {
        return m_speed;
    }
}