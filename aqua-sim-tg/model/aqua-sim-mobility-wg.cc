#include "ns3/simulator.h"
#include <algorithm>
#include <cmath>
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "aqua-sim-mobility-wg.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WaveGilderMobilityModel");

NS_OBJECT_ENSURE_REGISTERED (WaveGilderMobilityModel);

TypeId
WaveGilderMobilityModel::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::WaveGilderMobilityModel")
        .SetParent<MobilityModel> ()
        .SetGroupName ("Mobility")
        .AddConstructor<WaveGilderMobilityModel> ()
        .AddAttribute ("VariableSpeed", "Set gilder speed is Variable. Default false.",
                        BooleanValue(0),
                        MakeBooleanAccessor (&WaveGilderMobilityModel::m_isVarSpeed),
                        MakeBooleanChecker())
        .AddAttribute ("Bounds", "The 2d bounding area",
                        RectangleValue (Rectangle (-100, 100, -100, 100)),
                        MakeRectangleAccessor (&WaveGilderMobilityModel::m_bounds),
                        MakeRectangleChecker ())
        .AddAttribute ("Speed", "A random variable to control the speed (m/s).",
                        StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
                        MakePointerAccessor (&WaveGilderMobilityModel::m_rndSpeed),
                        MakePointerChecker<UniformRandomVariable> ())
        .AddAttribute ("Pause", "A random variable to control the pause (s).",
                        StringValue ("ns3::ConstantRandomVariable[Constant=0.1]"),
                        MakePointerAccessor (&WaveGilderMobilityModel::m_pause),
                        MakePointerChecker<ConstantRandomVariable> ())
        .AddAttribute ("Direction",
                        "A random variable used to assign the direction.",
                        StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=6.283185307]"),
                        MakePointerAccessor (&WaveGilderMobilityModel::m_rndDirection),
                        MakePointerChecker<UniformRandomVariable> ())
    ;
    return tid;
}

WaveGilderMobilityModel::WaveGilderMobilityModel ()
{
    NS_LOG_FUNCTION(this);
    m_waveModel = WaveUtil::GetInstance();
    Simulator::ScheduleNow(&WaveGilderMobilityModel::Init, this);
}

WaveGilderMobilityModel::~WaveGilderMobilityModel()
{
    
}
void
WaveGilderMobilityModel::Init()
{
    NS_LOG_FUNCTION(this);
    m_lastupdate = Seconds(0);
    m_speed = 0.0;
    m_direction = 0.0;
    m_pitch = 0.0;
    m_event = Simulator::ScheduleNow(&WaveGilderMobilityModel::Start, this);
    m_helper.Unpause();
}
void 
WaveGilderMobilityModel::DoDispose (void)
{
  MobilityModel::DoDispose ();
}

void
WaveGilderMobilityModel::DoInitialize (void)
{
    // ResetDirectionAndSpeed ();
    MobilityModel::DoInitialize ();
}

Vector
WaveGilderMobilityModel::DoGetPosition (void) const
{
    m_helper.UpdateWithBounds (m_bounds);
    Vector vector = m_helper.GetCurrentPosition();
    double z = m_waveModel->GetHeight(vector.x, vector.y, Simulator::Now());
    return Vector(vector.x, vector.y, z);
}
void
WaveGilderMobilityModel::DoSetPosition (const Vector &position)
{
    m_helper.SetPosition (position);
    Simulator::Remove (m_event);
    m_event.Cancel ();
    m_event = Simulator::ScheduleNow (&WaveGilderMobilityModel::Start, this);
}
Vector
WaveGilderMobilityModel::DoGetVelocity (void) const
{
  return m_helper.GetVelocity ();
}
int64_t
WaveGilderMobilityModel::DoAssignStreams (int64_t stream)
{
    m_rndDirection->SetStream (stream + 2);
    m_rndSpeed->SetStream (stream + 3);
    m_pause->SetStream (stream + 4);
    return 5;
}

void 
WaveGilderMobilityModel::Start (void)
{
    NS_LOG_FUNCTION(this);
    if (m_speed == 0.0) {
        m_speed = m_rndSpeed->GetValue();
        m_direction = m_rndDirection->GetValue();
        // Default slide down at the beginning
        double cosD = std::cos (m_direction);
        double sinD = std::sin (m_direction);
        //Set the velocity vector to give to the constant velocity helper
        m_helper.SetVelocity(Vector(m_speed*cosD, m_speed*sinD, 0));
    }
    NS_LOG_INFO("Mobility current speed is " << m_speed 
                << ", current direction is " << m_direction << ".");
    m_helper.Update();
    m_helper.Unpause();

    Time delay = Seconds(m_pause->GetValue());
    DoWalk(delay);
}

void 
WaveGilderMobilityModel::DoWalk (Time delayLeft)
{
    NS_LOG_FUNCTION(this << delayLeft);
    m_helper.UpdateWithBounds(m_bounds);
    Vector position = m_helper.GetCurrentPosition();
    Vector velocity = m_helper.GetVelocity();
    Vector nextPos  = position;
    nextPos.x += velocity.x * delayLeft.GetSeconds ();
    nextPos.y += velocity.y * delayLeft.GetSeconds ();
    nextPos.z = m_waveModel->GetHeight(nextPos.x, nextPos.y, Simulator::Now() + delayLeft);

    // Make sure that the position by the next time step is still within the boundary.
    // If out of bounds, then alter the velocity vector and average direction to keep the position in bounds

    NS_LOG_INFO(Simulator::Now().GetSeconds() << "s, next postition is " << nextPos << ".");
    bool keepOriginalState = true;
    // Check whether the node is out of bounds(2D)
    if (!m_bounds.IsInside(nextPos)) {
        NS_LOG_DEBUG("Wave gilder mobility has been moved to the boundary. Next position is " << nextPos);
        if (nextPos.x > m_bounds.xMax || nextPos.x < m_bounds.xMin) {
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
        m_helper.Unpause ();
    }

    m_event = Simulator::Schedule(delayLeft, &WaveGilderMobilityModel::Start, this);
    if (keepOriginalState == false) {
        // NotifyCourseChange();
    }
    NotifyCourseChange();
}


WaveUtil* WaveUtil::single;
std::mutex WaveUtil:: s_mutex;
WaveUtil::WaveUtil() {
        wave_div_num = 10;
        dire_div_num = 5;
        beta1 = 0.0081;
        beta2 = 0.74;
        g = 9.8;
        U = 5;
        delta_omega = 0.1;
        delta_theta = M_PI / 5.0;
        Init();
}
void 
WaveUtil::Init() {
    std::vector<double> tmp(dire_div_num);
    epsilon.resize(wave_div_num, tmp);
    alpha.resize(wave_div_num, tmp);
    omegas.resize(wave_div_num);
    thetas.resize(dire_div_num);
    
    Ptr<UniformRandomVariable> t_rand = CreateObject<UniformRandomVariable>();
    
    for (int i = 0; i < wave_div_num; i++) {
        omegas[i] = t_rand->GetValue(1, 3);
    }
    for (int i = 0; i < dire_div_num; i++) {
        thetas[i] = t_rand->GetValue(0, 2*M_PI);
    }
    for (int i = 0; i < wave_div_num; i++) {
        for (int j = 0; j < dire_div_num; j++) {
            epsilon[i][j] = t_rand->GetValue(0, 2*M_PI);
            alpha[i][j] = std::sqrt(2 * delta_theta * delta_omega * DirectionSpectrum(omegas[i], thetas[j]));
        }
    }
}
double 
WaveUtil::GetHeight(double x, double y, Time t)
{
    x = x / 4.0;
    y = y / 4.0;
    t = Seconds(t.GetSeconds() / 4.0);
    double height = 0.0;
    for (int i = 0; i < omegas.size(); i++) {
        for (int j = 0; j < thetas.size(); j++) {
            double k = omegas[i] * omegas[i] / g;
            double tmp1 = k*x*std::cos(thetas[j]) + k*y*std::sin(thetas[j]);
            double tmp2 = omegas[i] * t.GetSeconds() + epsilon[i][j];
            height += alpha[i][j] * std::cos(tmp1 - tmp2);
        }
    }
    return height;
}
double 
WaveUtil::DirectionSpectrum(double omega, double theta)
{
    return WaveSpectrum(omega) * WaveDirection(theta);
}
double 
WaveUtil::WaveSpectrum(double omega)
{
    double powOmega = omega * omega * omega * omega * omega;
    double tmp = -beta2 * std::pow(g / (U * omega), 4);
    return beta1 * g * g / powOmega * std::exp(tmp);
}
double 
WaveUtil::WaveDirection(double theta)
{
    // std::assert(theta >= -2*M_PI && theta <= 2*M_PI);
    double tmp = std::cos(theta);
    return 2 / M_PI * tmp * tmp;
}

}