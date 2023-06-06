/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 */

#include "ns3/aqua-sim-header-mac.h"
#include "aqua-sim-pt-tag.h"

#include "aqua-sim-header-routing.h"
#include "aqua-sim-header-mac.h"
#include "aqua-sim-header-transport.h"
#include "aqua-sim-transport.h"

#include "aqua-sim-tap-bridge.h"

#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/ethernet-header.h"
#include "ns3/llc-snap-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/enum.h"
#include "ns3/ipv4.h"
#include "ns3/simulator.h"
#include "ns3/realtime-simulator-impl.h"
#include "ns3/unix-fd-reader.h"
#include "ns3/uinteger.h"
#include "ns3/tap-bridge.h"
#include "ns3/aqua-sim-header.h"
#include "ns3/aqua-sim-socket-address.h"
#include "ns3/aqua-sim-net-device.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cerrno>
#include <limits>
#include <cstdlib>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <linux/if_tun.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace ns3
{

	//Ptr<Node> sink_node = NULL;

	NS_LOG_COMPONENT_DEFINE("AquaSimTapBridge");

	FdReader::Data AquaSimTapBridgeFdReader::DoRead(void)
	{
		NS_LOG_FUNCTION_NOARGS();

		uint32_t bufferSize = 65536;
		uint8_t *buf = (uint8_t *)std::malloc(bufferSize);
		NS_ABORT_MSG_IF(buf == 0, "malloc() failed");

		NS_LOG_LOGIC("Calling read on tap device fd " << m_fd);
		ssize_t len = read(m_fd, buf, bufferSize);
		if (len <= 0)
		{
			NS_LOG_INFO("TapBridgeFdReader::DoRead(): done");
			std::free(buf);
			buf = 0;
			len = 0;
		}

		return FdReader::Data(buf, len);
	}

#define TAP_MAGIC 95549

	NS_OBJECT_ENSURE_REGISTERED(AquaSimTapBridge);

TypeId AquaSimTapBridge::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::AquaSimTapBridge").SetParent<NetDevice>().SetGroupName(
					"AquaSimTapBridge").AddConstructor<AquaSimTapBridge>().AddAttribute(
					"Mtu", "The MAC-level Maximum Transmission Unit",
					UintegerValue(0),
					MakeUintegerAccessor(&AquaSimTapBridge::SetMtu,
							&AquaSimTapBridge::GetMtu),
					MakeUintegerChecker<uint16_t>()).AddAttribute("DeviceName",
					"The name of the tap device to create.", StringValue(""),
					MakeStringAccessor(&AquaSimTapBridge::m_tapDeviceName),
					MakeStringChecker()).AddAttribute("Gateway",
					"The IP address of the default gateway to assign to the host machine, when in ConfigureLocal mode.",
					Ipv4AddressValue("255.255.255.255"),
					MakeIpv4AddressAccessor(&AquaSimTapBridge::m_tapGateway),
					MakeIpv4AddressChecker()).AddAttribute("IpAddress",
					"The IP address to assign to the tap device, when in ConfigureLocal mode.  "
							"This address will override the discovered IP address of the simulated device.",
					Ipv4AddressValue("255.255.255.255"),
					MakeIpv4AddressAccessor(&AquaSimTapBridge::m_tapIp),
					MakeIpv4AddressChecker()).AddAttribute("MacAddress",
					"The MAC address to assign to the tap device, when in ConfigureLocal mode.  "
							"This address will override the discovered MAC address of the simulated device.",
					Mac48AddressValue(Mac48Address("ff:ff:ff:ff:ff:ff")),
					MakeMac48AddressAccessor(&AquaSimTapBridge::m_tapMac),
					MakeMac48AddressChecker()).AddAttribute("Netmask",
					"The network mask to assign to the tap device, when in ConfigureLocal mode.  "
							"This address will override the discovered MAC address of the simulated device.",
					Ipv4MaskValue("255.255.255.255"),
					MakeIpv4MaskAccessor(&AquaSimTapBridge::m_tapNetmask),
					MakeIpv4MaskChecker()).AddAttribute("Start",
					"The simulation time at which to spin up the tap device read thread.",
					TimeValue(Seconds(0.)),
					MakeTimeAccessor(&AquaSimTapBridge::m_tStart),
					MakeTimeChecker()).AddAttribute("Stop",
					"The simulation time at which to tear down the tap device read thread.",
					TimeValue(Seconds(0.)),
					MakeTimeAccessor(&AquaSimTapBridge::m_tStop),
					MakeTimeChecker()).AddAttribute("Mode",
					"The operating and configuration mode to use.",
					EnumValue(USE_LOCAL),
					MakeEnumAccessor(&AquaSimTapBridge::SetMode),
					MakeEnumChecker(CONFIGURE_LOCAL, "ConfigureLocal",
							USE_LOCAL, "UseLocal", USE_BRIDGE, "UseBridge"));
	return tid;
}


	AquaSimTapBridge::AquaSimTapBridge() 
	:m_node(0), m_ifIndex(0), m_sock(-1), m_startEvent(), 
	m_stopEvent(), m_fdReader(0),m_ns3AddressRewritten(false)
	{
		NS_LOG_FUNCTION_NOARGS();
		m_packetBuffer = new uint8_t[65536];
		Start(m_tStart);
	}

	AquaSimTapBridge::~AquaSimTapBridge()
	{
		NS_LOG_FUNCTION_NOARGS();

		StopTapDevice();

		delete[] m_packetBuffer;
		m_packetBuffer = 0;

		m_bridgedDevice = 0;
	}

	void AquaSimTapBridge::DoDispose()
	{
		NS_LOG_FUNCTION_NOARGS();
		NetDevice::DoDispose();
	}

	void AquaSimTapBridge::Start(Time tStart)
	{
		//std::cout << "AquaSimTapBridge::Start" << std::endl;
		NS_LOG_FUNCTION(tStart);

		//
		// Cancel any pending start event and schedule a new one at some relative time in the future.
		//
		Simulator::Cancel(m_startEvent);
		m_startEvent = Simulator::Schedule(tStart,
										   &AquaSimTapBridge::StartTapDevice, this);
	}

	void AquaSimTapBridge::Stop(Time tStop)
	{
		NS_LOG_FUNCTION(tStop);
		//
		// Cancel any pending stop event and schedule a new one at some relative time in the future.
		//
		Simulator::Cancel(m_stopEvent);
		m_startEvent = Simulator::Schedule(tStop, &AquaSimTapBridge::StopTapDevice,
										   this);
	}

	void AquaSimTapBridge::StartTapDevice(void)
	{
		NS_LOG_FUNCTION_NOARGS();

		NS_ABORT_MSG_IF(m_sock != -1,
						"TapBridge::StartTapDevice(): Tap is already started");

		//
		// A similar story exists for the node ID.  We can't just naively do a
		// GetNode ()->GetId () since GetNode is going to give us a Ptr<Node> which
		// is reference counted.  We need to stash away the node ID for use in the
		// read thread.
		//
		m_nodeId = GetNode()->GetId();

		//
		// Spin up the tap bridge and start receiving packets.
		//
		NS_LOG_LOGIC("Creating tap device");

		//
		// Call out to a separate process running as suid root in order to get the
		// tap device allocated and set up.  We do this to avoid having the entire
		// simulation running as root.  If this method returns, we'll have a socket
		// waiting for us in m_sock that we can use to talk to the newly created
		// tap device.
		//
		CreateTap();

		// Declare the link up

		//std::ostringstream interest;
		//interest << "Hello!" << '\0';

		NotifyLinkUp();

		//
		// Now spin up a read thread to read packets from the tap device.
		//
		NS_ABORT_MSG_IF(m_fdReader != 0,
						"TapBridge::StartTapDevice(): Receive thread is already running");
		NS_LOG_LOGIC("Spinning up read thread");
		m_fdReader = Create<AquaSimTapBridgeFdReader>();
		//	std::cout << "AquaSimTapBridge::StartTapDevice" << std::endl;

		//ReadCallback((uint8_t*)interest.str().c_str(), interest.str().length());

		m_fdReader->Start(m_sock,
						  MakeCallback(&AquaSimTapBridge::ReadCallback, this));
	}

	void AquaSimTapBridge::StopTapDevice(void)
	{
		NS_LOG_FUNCTION_NOARGS();

		if (m_fdReader != 0)
		{
			m_fdReader->Stop();
			m_fdReader = 0;
		}

		if (m_sock != -1)
		{
			close(m_sock);
			m_sock = -1;
		}
	}
	void AquaSimTapBridge::CreateTap(void)
	{

		struct ifreq ifr;
		int fd, err;
		//char *clonedev = "/dev/net/tun";

		if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
		{
			m_sock = fd;
		}

		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

		if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
		{
			close(fd);
		}
		system("sudo ip link set dev tun0 up");
		system("sudo ip address add dev tun0 10.1.1.1/24");
		//	   std::cout<<"TUN name is "<<ifr.ifr_name<<std::endl;
		m_sock = fd;
	}


	void AquaSimTapBridge::ReadCallback(uint8_t *buf, ssize_t len)
	{
		//std::cout << "AquaSimTapBridge::ReadCallback" << std::endl;
		NS_LOG_FUNCTION_NOARGS();

		NS_ASSERT_MSG(buf != 0, "invalid buf argument");
		NS_ASSERT_MSG(len > 0, "invalid len argument");

		//
		// It's important to remember that we're in a completely different thread
		// than the simulator is running in.  We need to synchronize with that
		// other thread to get the packet up into ns-3.  What we will need to do
		// is to schedule a method to deal with the packet using the multithreaded
		// simulator we are most certainly running.  However, I just said it -- we
		// are talking about two threads here, so it is very, very dangerous to do
		// any kind of reference counting on a shared object.  Just don't do it.
		// So what we're going to do is pass the buffer allocated on the heap
		// into the ns-3 context thread where it will create the packet.
		//

		NS_LOG_INFO(
			"TapBridge::ReadCallback(): Received packet on node " << m_nodeId);
		NS_LOG_INFO("TapBridge::ReadCallback(): Scheduling handler");
		Simulator::ScheduleWithContext(m_nodeId, Seconds(0.0),
									   MakeEvent(&AquaSimTapBridge::ForwardToBridgedDevice, this, buf,
												 len));
	}

	void AquaSimTapBridge::ForwardToBridgedDevice(uint8_t *buf, ssize_t len)
	{
		//    std::cout<<"forwardtobridgedevice\n";
		//    std::cout<<"I am coming\n";
		//
		// There are three operating modes for the TapBridge
		//
		// CONFIGURE_LOCAL means that ns-3 will create and configure a tap device
		// and we are expected to use it.  The tap device and the ns-3 net device
		// will have the same MAC address by definition.  Thus Send and SendFrom
		// are equivalent in this case.  We use Send to allow all ns-3 devices to
		// participate in this mode.
		//
		// USE_LOCAL mode tells us that we have got to USE a pre-created tap device
		// that will have a different MAC address from the ns-3 net device.  We
		// also enforce the requirement that there will only be one MAC address
		// bridged on the Linux side so we can use Send (instead of SendFrom) in
		// the linux to ns-3 direction.  Again, all ns-3 devices can participate
		// in this mode.
		//
		// USE_BRIDGE mode tells us that we are logically extending a Linux bridge
		// on which lies our tap device.  In this case there may be many linux
		// net devices on the other side of the bridge and so we must use SendFrom
		// to preserve the possibly many source addresses.  Thus, ns-3 devices
		// must support SendFrom in order to be considered for USE_BRIDGE mode.
		//

		//
		// First, create a packet out of the byte buffer we received and free that
		// buffer.
		//
		Ptr<Packet> packet = Create<Packet>(reinterpret_cast<const uint8_t *>(buf),
											len);
		std::free(buf);
		buf = 0;

		//
		// Make sure the packet we received is reasonable enough for the rest of the
		// system to handle and get it ready to be injected directly into an ns-3
		// device.  What should come back is a packet with the Ethernet header
		// (and possibly an LLC header as well) stripped off.
		//
		Address src, dst;
		uint16_t type = 0;
		//std::cout<<"Received packet from Linux\n";
		Ptr<Packet> p = Filter(packet, &src, &dst, &type);
		uint8_t *m_buf;
		Ptr<Packet> pkt;
		Address dest;
		m_buf = new uint8_t[65536];
		packet->CopyData(m_buf, packet->GetSize());

		//print buf context
		//  std::cout<<"Receive Linux Packet Content:";
		//  std::cout<<m_buf+14<<std::endl;
		//testing:the 14th byte is the begin of packet(only testing in my computer(CentOS7))
		//packet type is DATA when the begin of the packet is '3',and packet type is CTS when the begin of the packet is '2' and other is unreconginized
		//  std::cout<<std::endl<<"Packet Start Content:"<<*(m_buf+14)<<std::endl;
		char startContent = *(m_buf + 14);
		if (startContent == '3')
		{
			std::cout << "I am a DATA PACKET" << std::endl;
			std::ostringstream interest;
			interest << m_buf + 23 << '\0';
			uint32_t packetSeq = (*(m_buf + 19) - '0') * 10 + (*(m_buf + 20) - '0') - 1;
			std::cout << "New Packet Data Seq is :" << packetSeq << std::endl;
			uint32_t packetCount = (*(m_buf + 21) - '0') * 10 + (*(m_buf + 22) - '0');
			std::cout << "New Packet Data Count is :" << packetCount << std::endl;
			int SA = (*(m_buf + 15) - '0') * 10 + (*(m_buf + 16) - '0');
			std::cout << "New Packet Data Source is :" << SA << std::endl;
			int DA = (*(m_buf + 17) - '0') * 10 + (*(m_buf + 18) - '0');
			std::cout << "New Packet Data Destination is :" << DA << std::endl;
			std::string hexstr = interest.str().c_str();
			std::cout << hexstr << std::endl;
			std::string norstr = HexToStr(hexstr);
			std::cout << norstr << std::endl;
			packet = Create<Packet>((uint8_t *)norstr.c_str(), norstr.length());
			TransportHeader tsh;
			AquaSimPtTag ptag;
			AquaSimHeader ash;
			MacHeader mach;
			tsh.SetSequenceNumber(0);
			tsh.SetSubSequenceNumber(packetSeq);
			tsh.SetCount(packetCount);
			/*	-- spinach
			uint16_t checksum = AquaSimTransport::CheckSum(m_buf,
														   packet->GetSize());
			tsh.SetCheckSum(checksum);
			tsh.SetCount(packetCount);
			tsh.SetSequenceNumber(packetSeq);
			tsh.SetSourceAddress(
				AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(SA)));
			tsh.SetDestAddress(
				AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(DA)));
			tsh.SetPType(TransportHeader::DATA);
			*/
			mach.SetDemuxPType(MacHeader::UWPTYPE_OTHER);
			ptag.SetPacketType(AquaSimPtTag::PT_HalfReality_DATA);
			ash.SetDirection(AquaSimHeader::DOWN);
			ash.SetErrorFlag(false);
			ash.SetTxTime(Time(-1)); //flag to be dealt with later.
			ash.SetSAddr(
				AquaSimAddress::ConvertFrom(AquaSimAddress::IntConvertTo(SA)));
			AquaSimAddress m = AquaSimAddress::ConvertFrom(
				AquaSimAddress::IntConvertTo(DA));
			dest = m;
			ash.SetDAddr(m);
			ash.SetNumForwards(0);
			packet->AddPacketTag(ptag);
			packet->AddHeader(tsh);
			packet->AddHeader(ash);
			//make a data packet
		}
		else if (startContent == '2')
		{
			std::cout << "Ghost Node Receive a CTS PACKET and Send to Simulator"
					  << std::endl;
			std::ostringstream interest;
			interest << m_buf + 14 << '\0';
			int num = (*(m_buf + 15) - '0') * 10 + (*(m_buf + 16) - '0');
			packet = Create<Packet>((uint8_t *)interest.str().c_str(),
									interest.str().length());
			AquaSimPtTag ptag;
			AquaSimHeader ash;
			ptag.SetPacketType(AquaSimPtTag::PT_HalfReality_CTS);
			ash.SetDirection(AquaSimHeader::DOWN);
			ash.SetErrorFlag(false);
			ash.SetSAddr(
				AquaSimAddress::ConvertFrom(
					GetNode()->GetDevice(0)->GetAddress()));
			// 	std::cout<<"New Packet CTS Source:"<<ash.GetSAddr()<<std::endl;
			AquaSimAddress m = AquaSimAddress::ConvertFrom(
				AquaSimAddress::IntConvertTo(num));
			dest = m;
			ash.SetDAddr(m);
			//   std::cout<<"New Packet CTS Destination:"<<ash.GetDAddr()<<std::endl;
			ash.SetNumForwards(0);
			packet->AddPacketTag(ptag);
			packet->AddHeader(ash);
			//make a CTS packet
		}
		else
		{
			//unreconginized packet
			p = 0;
		}
		if (p == 0)
		{
			//   std::cout<<"p=0\n";
			NS_LOG_LOGIC(
				"TapBridge::ForwardToBridgedDevice:  Discarding packet as unfit for ns-3 consumption");
			return;
		}
		NS_ASSERT_MSG(m_mode == CONFIGURE_LOCAL,
					  "TapBridge::ForwardToBridgedDevice(): Internal error");
		StaticCast<AquaSimNetDevice>(m_node->GetDevice(0))->GetRouting()->Recv(packet, dest, 0);
	}

	Ptr<Packet> AquaSimTapBridge::Filter(Ptr<Packet> p, Address *src, Address *dst,
										 uint16_t *type)
	{
		NS_LOG_FUNCTION(p);
		uint32_t pktSize;

		//
		// We have a candidate packet for injection into ns-3.  We expect that since
		// it came over a socket that provides Ethernet packets, it should be big
		// enough to hold an EthernetHeader.  If it can't, we signify the packet
		// should be filtered out by returning 0.
		//
		pktSize = p->GetSize();
		EthernetHeader header(false);
		Ipv4Header h;
		if (pktSize < header.GetSerializedSize())
		{
			return 0;
		}

		uint32_t headerSize = p->PeekHeader(header);
		p->PeekHeader(h);
		p->RemoveAtStart(headerSize);

		NS_LOG_LOGIC("Pkt source is " << header.GetSource());
		NS_LOG_LOGIC("Pkt destination is " << header.GetDestination());
		NS_LOG_LOGIC("Pkt LengthType is " << header.GetLengthType());

		//
		// If the length/type is less than 1500, it corresponds to a length
		// interpretation packet.  In this case, it is an 802.3 packet and
		// will also have an 802.2 LLC header.  If greater than 1500, we
		// find the protocol number (Ethernet type) directly.
		//
		if (header.GetLengthType() <= 1500)
		{
			*src = header.GetSource();
			*dst = header.GetDestination();

			pktSize = p->GetSize();
			LlcSnapHeader llc;
			if (pktSize < llc.GetSerializedSize())
			{
				return 0;
			}

			p->RemoveHeader(llc);
			*type = llc.GetType();
		}
		else
		{
			*src = header.GetSource();
			*dst = header.GetDestination();
			*type = header.GetLengthType();
		}

		//
		// What we give back is a packet without the Ethernet header (nor the
		// possible llc/snap header) on it.  We think it is ready to send on
		// out the bridged net device.
		//
		*dst = h.GetDestination(); //add by guo
		return p;
	}

	Ptr<NetDevice> AquaSimTapBridge::GetBridgedNetDevice(void)
	{
		NS_LOG_FUNCTION_NOARGS();
		return m_bridgedDevice;
	}

	void AquaSimTapBridge::SetBridgedNetDevice(Ptr<NetDevice> bridgedDevice)
	{
		NS_LOG_FUNCTION(bridgedDevice);

		NS_ASSERT_MSG(m_node != 0,
					  "TapBridge::SetBridgedDevice:  Bridge not installed in a node");
		NS_ASSERT_MSG(bridgedDevice != this,
					  "TapBridge::SetBridgedDevice:  Cannot bridge to self");
		NS_ASSERT_MSG(m_bridgedDevice == 0,
					  "TapBridge::SetBridgedDevice:  Already bridged");

		if (!Mac48Address::IsMatchingType(bridgedDevice->GetAddress()))
		{
			NS_FATAL_ERROR(
				"TapBridge::SetBridgedDevice: Device does not support eui 48 addresses: cannot be added to bridge.");
		}

		if (m_mode == USE_BRIDGE && !bridgedDevice->SupportsSendFrom())
		{
			NS_FATAL_ERROR(
				"TapBridge::SetBridgedDevice: Device does not support SendFrom: cannot be added to bridge.");
		}

		//
		// We need to disconnect the bridged device from the internet stack on our
		// node to ensure that only one stack responds to packets inbound over the
		// bridged device.  That one stack lives outside ns-3 so we just blatantly
		// steal the device callbacks.
		//
		// N.B This can be undone if someone does a RegisterProtocolHandler later
		// on this node.
		//
		//bridgedDevice->SetReceiveCallback(MakeCallback(&AquaSimTapBridge::DiscardFromBridgedDevice, this));
		bridgedDevice->SetReceiveCallback(
			MakeCallback(&AquaSimTapBridge::ReceiveFromBridgedDevice, this));
		m_bridgedDevice = bridgedDevice;
	}

	bool AquaSimTapBridge::DiscardFromBridgedDevice(Ptr<NetDevice> device,
													Ptr<const Packet> packet, uint16_t protocol, const Address &src)
	{
		NS_LOG_FUNCTION(device << packet << protocol << src);
		NS_LOG_LOGIC("Discarding packet stolen from bridged device " << device);
		return true;
	}

	bool AquaSimTapBridge::ReceiveFromBridgedDevice(Ptr<NetDevice> device,
													Ptr<const Packet> packet, uint16_t protocol, const Address &src)
	{
		//std::cout<<"receive from CsmaNetDevice\n";

		sleep(5);
		int port = 1235;				 //从命令行获取端口号
		if (port < 1025 || port > 65535) //0~1024一般给系统使用，一共可以分配到65535
		{
			printf("端口号范围应为1025~65535");
			return -1;
		}

		//1 创建udp通信socket
		int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (udp_socket_fd == -1)
		{
			perror("socket failed!\n");
			return -1;
		}

		//设置目的IP地址
		struct sockaddr_in dest_addr = {0};
		dest_addr.sin_family = AF_INET;					   //使用IPv4协议
		dest_addr.sin_port = htons(port);				   //设置接收方端口号
		dest_addr.sin_addr.s_addr = inet_addr("10.1.1.1"); //设置接收方IP

		Ptr<Packet> pkt = packet->Copy();
		std::cout << pkt->GetSize() << std::endl;
		AquaSimPtTag ptag;
		AquaSimHeader asHeader;
		pkt->RemoveHeader(asHeader);

		pkt->RemovePacketTag(ptag);
		std::cout << pkt << std::endl;
		uint8_t *m_Buffer;

		if (ptag.GetPacketType() == AquaSimPtTag::PT_HalfReality_RTS)
		{
			m_Buffer = new uint8_t[65536];
			pkt->CopyData(m_Buffer, pkt->GetSize());
			std::cout << "Ghost Node Receive a RTS Packet and Send to Reality"
					  << std::endl;
		}
		else if (ptag.GetPacketType() == AquaSimPtTag::PT_HalfReality_ACK)
		{
			m_Buffer = new uint8_t[65536];
			pkt->CopyData(m_Buffer, pkt->GetSize());
			std::cout << "ReceiveFromCsmaNetDevice ACK Packet" << std::endl;
			std::cout << "FAMA_DATA_ACK Packet Source is:" << asHeader.GetSAddr()
					  << std::endl;
			std::cout << "FAMA_DATA_ACK Packet Destination is:"
					  << asHeader.GetDAddr() << std::endl;
		}
		else
		{
			TransportHeader tsh;
			pkt->RemoveHeader(tsh);
			int packetSeq = tsh.GetSequenceNumber();
			int packetCount = tsh.GetCount();
			m_Buffer = new uint8_t[65536];
			pkt->CopyData(m_Buffer, pkt->GetSize());
			std::cout << pkt->GetSize() << std::endl;

			std::cout << "Ghost Node Receive a DATA Packet and Send to Reality"
					  << std::endl;
			std::stringstream ss;
			//ss << "3" <<asHeader.GetSAddr()<<asHeader.GetDAddr()<<m_Buffer;
			ss << "3" << asHeader.GetSAddr() << "20";
			if (packetSeq < 9)
			{
				ss << "0" << packetSeq + 1;
			}
			else
			{
				ss << packetSeq + 1;
			}
			if (packetCount < 9)
			{
				ss << "0" << packetCount;
			}
			else
			{
				ss << packetCount;
			}
			for (int j = 0; j < int(pkt->GetSize()); j++)
			{
				const std::string hex = "0123456789ABCDEFG";
				ss << hex[m_Buffer[j] >> 4] << hex[m_Buffer[j] & 0xf];
			}
			std::string combined = ss.str();
			std::cout << combined.length() << std::endl;
			std::strncpy((char *)m_Buffer, combined.c_str(), combined.length());
		}
		std::stringstream ss;
		ss << m_Buffer;
		std::string combined = ss.str();
		std::cout << "packet content is:" << combined.length() << " " << combined
				  << std::endl;
		int size = combined.length();
		std::strncpy((char *)m_Buffer, combined.c_str(), size);
		sendto(udp_socket_fd, m_Buffer, size, 0, (struct sockaddr *)&dest_addr,
			   sizeof(dest_addr));
		memset(m_Buffer, 0, packet->GetSize()); //清空存留消息
		//3 关闭通信socket
		close(udp_socket_fd);
		return true;
	}


	void AquaSimTapBridge::SetIfIndex(const uint32_t index)
	{
		NS_LOG_FUNCTION_NOARGS();
		m_ifIndex = index;
	}

	uint32_t AquaSimTapBridge::GetIfIndex(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return m_ifIndex;
	}

	Ptr<Channel> AquaSimTapBridge::GetChannel(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return 0;
	}

	void AquaSimTapBridge::SetAddress(Address address)
	{
		NS_LOG_FUNCTION(address);
		m_address = Mac48Address::ConvertFrom(address);
	}

	Address AquaSimTapBridge::GetAddress(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return m_address;
	}

	void AquaSimTapBridge::SetMode(enum Mode mode)
	{
		NS_LOG_FUNCTION(mode);
		m_mode = mode;
	}

	AquaSimTapBridge::Mode AquaSimTapBridge::GetMode(void)
	{
		NS_LOG_FUNCTION_NOARGS();
		return m_mode;
	}

	bool AquaSimTapBridge::SetMtu(const uint16_t mtu)
	{
		NS_LOG_FUNCTION_NOARGS();
		m_mtu = mtu;
		return true;
	}

	uint16_t AquaSimTapBridge::GetMtu(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return m_mtu;
	}

	void AquaSimTapBridge::NotifyLinkUp(void)
	{
		NS_LOG_FUNCTION_NOARGS();
		if (!m_linkUp)
		{
			m_linkUp = true;
			m_linkChangeCallbacks();
		}
	}

	bool AquaSimTapBridge::IsLinkUp(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return m_linkUp;
	}

	void AquaSimTapBridge::AddLinkChangeCallback(Callback<void> callback)
	{
		NS_LOG_FUNCTION_NOARGS();
		m_linkChangeCallbacks.ConnectWithoutContext(callback);
	}

	bool AquaSimTapBridge::IsBroadcast(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return true;
	}

	Address AquaSimTapBridge::GetBroadcast(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return Mac48Address("ff:ff:ff:ff:ff:ff");
	}

	bool AquaSimTapBridge::IsMulticast(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return true;
	}

	Address AquaSimTapBridge::GetMulticast(Ipv4Address multicastGroup) const
	{
		NS_LOG_FUNCTION(this << multicastGroup);
		Mac48Address multicast = Mac48Address::GetMulticast(multicastGroup);
		return multicast;
	}

	bool AquaSimTapBridge::IsPointToPoint(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return false;
	}

	bool AquaSimTapBridge::IsBridge(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		//
		// Returning false from IsBridge in a device called TapBridge may seem odd
		// at first glance, but this test is for a device that bridges ns-3 devices
		// together.  The Tap bridge doesn't do that -- it bridges an ns-3 device to
		// a Linux device.  This is a completely different story.
		//
		return false;
	}

	bool AquaSimTapBridge::Send(Ptr<Packet> packet, const Address &dst,
								uint16_t protocol)
	{
		NS_LOG_FUNCTION(packet << dst << protocol);
		NS_FATAL_ERROR(
			"TapBridge::Send: You may not call Send on a TapBridge directly");
		return false;
	}

	bool AquaSimTapBridge::SendFrom(Ptr<Packet> packet, const Address &src,
									const Address &dst, uint16_t protocol)
	{
		NS_LOG_FUNCTION(packet << src << dst << protocol);
		NS_FATAL_ERROR(
			"TapBridge::Send: You may not call SendFrom on a TapBridge directly");
		return false;
	}

	Ptr<Node> AquaSimTapBridge::GetNode(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return m_node;
	}

	void AquaSimTapBridge::SetNode(Ptr<Node> node)
	{
		NS_LOG_FUNCTION_NOARGS();
		m_node = node;
	}

	bool AquaSimTapBridge::NeedsArp(void) const
	{
		NS_LOG_FUNCTION_NOARGS();
		return true;
	}

	void AquaSimTapBridge::SetReceiveCallback(NetDevice::ReceiveCallback cb)
	{
		NS_LOG_FUNCTION_NOARGS();
		m_rxCallback = cb;
	}

	void AquaSimTapBridge::SetPromiscReceiveCallback(
		NetDevice::PromiscReceiveCallback cb)
	{
		NS_LOG_FUNCTION_NOARGS();
		m_promiscRxCallback = cb;
	}

	bool AquaSimTapBridge::SupportsSendFrom() const
	{
		NS_LOG_FUNCTION_NOARGS();
		return true;
	}

	Address AquaSimTapBridge::GetMulticast(Ipv6Address addr) const
	{
		NS_LOG_FUNCTION(this << addr);
		return Mac48Address::GetMulticast(addr);
	}

	std::string AquaSimTapBridge::StringToHex(const std::string &data)
	{
		const std::string hex = "0123456789ABCDEF";
		std::stringstream ss;

		for (std::string::size_type i = 0; i < data.size(); ++i)
			ss << hex[(unsigned char)data[i] >> 4]
			   << hex[(unsigned char)data[i] & 0xf];
		std::cout << ss.str() << std::endl;
		return ss.str();
	}

	std::string AquaSimTapBridge::HexToStr(const std::string &str)
	{
		std::string result;
		for (size_t i = 0; i < str.length(); i += 2)
		{
			std::string byte = str.substr(i, 2);
			char chr = (char)(int)strtol(byte.c_str(), NULL, 16);
			result.push_back(chr);
		}
		return result;
	}

} // namespace ns3
