#include "ns3/simulator.h"
#include <algorithm>
#include <cmath>
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "aqua-sim-mobility-ug3.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("UnderwaterGilderMobilityModel3");

    NS_OBJECT_ENSURE_REGISTERED(UnderwaterGilderMobilityModel3);

    TypeId
    UnderwaterGilderMobilityModel3::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::UnderwaterGilderMobilityModel3")
                                .SetParent<MobilityModel>()
                                .SetGroupName("Mobility")
                                .AddConstructor<UnderwaterGilderMobilityModel3>()
                                .AddAttribute("VariableSpeed", "Set gilder speed is Variable. Default false.",
                                              BooleanValue(0),
                                              MakeBooleanAccessor(&UnderwaterGilderMobilityModel3::m_isVarSpeed),
                                              MakeBooleanChecker())
                                .AddAttribute("Bounds", "The 2d bounding area",
                                              RectangleValue(Rectangle(-100, 100, -100, 100)),
                                              MakeRectangleAccessor(&UnderwaterGilderMobilityModel3::m_bounds),
                                              MakeRectangleChecker())
                                .AddAttribute("OpenAngle", "UnderwaterGilder's open angle(degree)",
                                              StringValue("ns3::UniformRandomVariable[Min=30.0|Max=50.0]"),
                                              MakePointerAccessor(&UnderwaterGilderMobilityModel3::m_openAngle),
                                              MakePointerChecker<UniformRandomVariable>())
                                .AddAttribute("Depth", "UnderwaterGilder's depth range",
                                              StringValue("ns3::UniformRandomVariable[Min=100.0|Max=1000.0]"),
                                              MakePointerAccessor(&UnderwaterGilderMobilityModel3::m_depthRange),
                                              MakePointerChecker<UniformRandomVariable>())
                                // .AddAttribute ("minOpenAngle", "set the min open angle.",
                                //                 DoubleValue(0.0),
                                //                 MakeDoubleAccessor(&UnderwaterGilderMobilityModel3::min_openAngle),
                                //                 MakeDoubleChecker<double>())
                                // .AddAttribute ("maxOpenAngle", "set the max open angle.",
                                //                 DoubleValue(0.0),
                                //                 MakeDoubleAccessor(&UnderwaterGilderMobilityModel3::max_openAngle),
                                //                 MakeDoubleChecker<double>())
                                // .AddAttribute ("minDepth", "set the min depth.",
                                //                 DoubleValue(0.0),
                                //                 MakeDoubleAccessor(&UnderwaterGilderMobilityModel3::min_depth),
                                //                 MakeDoubleChecker<double>())
                                // .AddAttribute ("maxDepth", "set the max depth.",
                                //                 DoubleValue(0.0),
                                //                 MakeDoubleAccessor(&UnderwaterGilderMobilityModel3::max_depth),
                                //                 MakeDoubleChecker<double>())
                                .AddAttribute("Speed", "A random variable to control the speed (m/s).",
                                              StringValue("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
                                              MakePointerAccessor(&UnderwaterGilderMobilityModel3::m_rndSpeed),
                                              MakePointerChecker<UniformRandomVariable>())
                                .AddAttribute("Pause", "A random variable to control the pause (s).",
                                              StringValue("ns3::ConstantRandomVariable[Constant=0.1]"),
                                              MakePointerAccessor(&UnderwaterGilderMobilityModel3::m_pause),
                                              MakePointerChecker<ConstantRandomVariable>())
                                .AddAttribute("Direction",
                                              "A random variable used to assign the direction.",
                                              StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283185307]"),
                                              MakePointerAccessor(&UnderwaterGilderMobilityModel3::m_rndDirection),
                                              MakePointerChecker<UniformRandomVariable>());
        return tid;
    }

    UnderwaterGilderMobilityModel3::UnderwaterGilderMobilityModel3()
    {
        NS_LOG_FUNCTION(this);
        Simulator::ScheduleNow(&UnderwaterGilderMobilityModel3::Init, this);
    }

    UnderwaterGilderMobilityModel3::~UnderwaterGilderMobilityModel3()
    {
    }
    void
    UnderwaterGilderMobilityModel3::Init()
    {
        NS_LOG_FUNCTION(this);
        m_speed = 0.0;
        m_direction = 0.0;
        m_pitch = 0.0;
        max_depth = m_depthRange->GetMax();
        min_depth = m_depthRange->GetMin();
        max_openAngle = m_openAngle->GetMax() / 180.0 * M_PI;
        min_openAngle = m_openAngle->GetMin() / 180.0 * M_PI;
        m_event = Simulator::ScheduleNow(&UnderwaterGilderMobilityModel3::Start, this);
        m_helper.Unpause();
    }
    void
    UnderwaterGilderMobilityModel3::DoDispose(void)
    {
        MobilityModel::DoDispose();
    }

    void
    UnderwaterGilderMobilityModel3::DoInitialize(void)
    {
        // ResetDirectionAndSpeed ();
        MobilityModel::DoInitialize();
    }

    Vector
    UnderwaterGilderMobilityModel3::DoGetPosition(void) const
    {
        m_helper.UpdateWithBounds(m_bounds);
        return m_helper.GetCurrentPosition();
    }
    void
    UnderwaterGilderMobilityModel3::DoSetPosition(const Vector &position)
    {
        m_helper.SetPosition(position);
        Simulator::Remove(m_event);
        m_event.Cancel();
        m_event = Simulator::ScheduleNow(&UnderwaterGilderMobilityModel3::Start, this);
    }
    Vector
    UnderwaterGilderMobilityModel3::DoGetVelocity(void) const
    {
        return m_helper.GetVelocity();
    }
    int64_t
    UnderwaterGilderMobilityModel3::DoAssignStreams(int64_t stream)
    {
        m_openAngle->SetStream(stream);
        m_depthRange->SetStream(stream + 1);
        m_rndDirection->SetStream(stream + 2);
        m_rndSpeed->SetStream(stream + 3);
        m_pause->SetStream(stream + 4);
        return 5;
    }

    void
    UnderwaterGilderMobilityModel3::Start(void)
    {
        NS_LOG_FUNCTION(this);
        if (m_speed == 0.0)
        {
            m_speed = m_rndSpeed->GetValue();
            m_direction = m_rndDirection->GetValue();
            double openAngle = m_openAngle->GetValue() / 180.0 * M_PI;
            // Default slide down at the beginning
            m_pitch = openAngle / 2 - M_PI_2;
            double cosD = std::cos(m_direction);
            double cosP = std::cos(m_pitch);
            double sinD = std::sin(m_direction);
            double sinP = std::sin(m_pitch);
            // Set the velocity vector to give to the constant velocity helper
            m_helper.SetVelocity(Vector(m_speed * cosD * cosP, m_speed * sinD * cosP, -m_speed * sinP));
        }
        NS_LOG_INFO("Mobility current speed is " << m_speed
                                                 << ", current direction is " << m_direction
                                                 << ", current pitch is " << m_pitch * 180.0 / M_PI << ".");
        m_helper.Update();
        m_helper.Unpause();

        Time delay = Seconds(m_pause->GetValue());
        DoWalk(delay);
    }

    void
    UnderwaterGilderMobilityModel3::DoWalk(Time delayLeft)
    {
        NS_LOG_FUNCTION(this << delayLeft);
        m_helper.UpdateWithBounds(m_bounds);
        Vector position = m_helper.GetCurrentPosition();
        Vector velocity = m_helper.GetVelocity();
        Vector nextPos = position;
        nextPos.x += velocity.x * delayLeft.GetSeconds();
        nextPos.y += velocity.y * delayLeft.GetSeconds();
        nextPos.z += velocity.z * delayLeft.GetSeconds();

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
        bool outOfDepth = false;
        if ((nextPos.z > max_depth && m_pitch < 0) || (nextPos.z < min_depth && m_pitch > 0))
        {
            keepOriginalState = false;
            outOfDepth = true;
        }
        if (/*keepOriginalState == false && */ outOfDepth == true)
        {
            double nextPitch = 0.0;
            double openAngle = 0.0;
            int count = 0;
            while (true)
            {
                openAngle = m_openAngle->GetValue() / 180.0 * M_PI;
                nextPitch = m_pitch >= 0 ? (openAngle + m_pitch - M_PI) : (M_PI - openAngle + m_pitch);

                if (std::fabs(nextPitch) < M_PI_2)
                {
                    NS_LOG_DEBUG("Get next pitch " << nextPitch * 180.0 / 3.14 << " ,openangle " << openAngle * 180.0 / 3.14 << " ,current position is " << position << ".");
                    break;
                }
                NS_LOG_WARN("Underwater gilder mobility do not find suitable pitch " << nextPitch * 180.0 / 3.14 << " ,openangle " << openAngle * 180.0 / 3.14 << "!");
                count++;
                if (count >= 5)
                {
                    NS_LOG_ERROR("Underwater gilder mobility can't find suitable pitch!");
                    return;
                }
            }
            m_pitch = nextPitch;
            double cosD = std::cos(m_direction);
            double cosP = std::cos(m_pitch);
            double sinD = std::sin(m_direction);
            double sinP = std::sin(m_pitch);
            velocity.x = m_speed * cosD * cosP;
            velocity.y = m_speed * sinD * cosP;
            velocity.z = -m_speed * sinP;
            m_helper.SetVelocity(velocity);
            m_helper.Unpause();
        }

        m_event = Simulator::Schedule(delayLeft, &UnderwaterGilderMobilityModel3::Start, this);
        if (keepOriginalState == false)
        {
            // NotifyCourseChange();
        }
        NotifyCourseChange();
    }
    ConstantVelocityHelper
    UnderwaterGilderMobilityModel3::DoGetMHelper() const
    {
        return m_helper;
    }
    Rectangle UnderwaterGilderMobilityModel3::DoGetMBounds() const
    {
        return m_bounds;
    }
    double UnderwaterGilderMobilityModel3::DoGetMDirection() const
    {
        return m_direction;
    }
    Ptr<UniformRandomVariable> UnderwaterGilderMobilityModel3::DoGetMOpenAngle() const
    {
        return m_openAngle;
    }
    double UnderwaterGilderMobilityModel3::DoGetMSpeed() const
    {
        return m_speed;
    }
    double UnderwaterGilderMobilityModel3::DoGetMaxDepth() const
    {
        return max_depth;
    }
    double UnderwaterGilderMobilityModel3::DoGetMinDepth() const
    {
        return min_depth;
    }
    double UnderwaterGilderMobilityModel3::DoGetMPitch() const
    {
        return m_pitch;
    }

}