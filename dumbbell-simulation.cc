/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
*/

#include <cmath>
#include <iostream>
#include <sstream>

// ns3 includes
#include "ns3/log.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/node-list.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/vector.h"
#include "ns3/ipv6-address-generator.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/stats-module.h"

NS_LOG_COMPONENT_DEFINE ("DumbbellSimulation");

using namespace ns3;

int main (int argc, char *argv[])
{

  LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
  //Declarations of helpers
  NodeContainer          m_leftLeaf;
  NetDeviceContainer     m_leftLeafDevices;
  NodeContainer          m_rightLeaf;
  NetDeviceContainer     m_rightLeafDevices;
  NodeContainer          m_routers;
  NetDeviceContainer     m_routerDevices; // just two connecting the routers
  NetDeviceContainer     m_leftRouterDevices;
  NetDeviceContainer     m_rightRouterDevices;

  // IP Containers
  Ipv4InterfaceContainer m_leftLeafInterfaces;
  Ipv4InterfaceContainer m_leftRouterInterfaces;
  Ipv4InterfaceContainer m_rightLeafInterfaces;
  Ipv4InterfaceContainer m_rightRouterInterfaces;
  Ipv4InterfaceContainer m_routerInterfaces;

  //Point to Point helpers
  PointToPointHelper longFlowPtP;
  PointToPointHelper shortFlowPtP;
  PointToPointHelper pointToPointBottleneck;
  pointToPointBottleneck.SetDeviceAttribute  ("DataRate", StringValue ("10Mbps"));
  pointToPointBottleneck.SetChannelAttribute ("Delay", StringValue ("10ms"));
  longFlowPtP.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  shortFlowPtP.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  longFlowPtP.SetChannelAttribute ("Delay", StringValue ("50ms"));
  shortFlowPtP.SetChannelAttribute ("Delay", StringValue ("5ms"));

  uint32_t LeftCount = 20;
  uint32_t RightCount = 20;

  // Create bottleneck routers and connections
  m_routers.Create (2);
  m_routerDevices = pointToPointBottleneck.Install(m_routers);

  // Create the leaf nodes
  m_leftLeaf.Create (LeftCount);
  m_rightLeaf.Create (RightCount);

  // Left long links
  for (uint32_t i = 0; i < 10; ++i)
    {
      NetDeviceContainer c = longFlowPtP.Install (m_routers.Get (0),
                                                 m_leftLeaf.Get (i));
      m_leftRouterDevices.Add (c.Get (0));
      m_leftLeafDevices.Add (c.Get (1));
    }

  // Left short links
  for (uint32_t i = 10; i < 20; ++i)
    {
      NetDeviceContainer c = shortFlowPtP.Install (m_routers.Get (0),
                                                 m_leftLeaf.Get (i));
      m_leftRouterDevices.Add (c.Get (0));
      m_leftLeafDevices.Add (c.Get (1));
    }

  // Right long links
  for (uint32_t i = 0; i < 10; ++i)
    {
      NetDeviceContainer c = longFlowPtP.Install (m_routers.Get (1),
                                                  m_rightLeaf.Get (i));
      m_rightRouterDevices.Add (c.Get (0));
      m_rightLeafDevices.Add (c.Get (1));
    }
  // Right short links
  for (uint32_t i = 10; i < 20; ++i)
    {
      NetDeviceContainer c = longFlowPtP.Install (m_routers.Get (1),
                                                  m_rightLeaf.Get (i));
      m_rightRouterDevices.Add (c.Get (0));
      m_rightLeafDevices.Add (c.Get (1));
    }

  // Install Stack
  InternetStackHelper stack;
  stack.Install (m_routers);
  stack.Install (m_leftLeaf);
  stack.Install (m_rightLeaf);

  //Assign IP addresses
  Ipv4AddressHelper leftIp = Ipv4AddressHelper ("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper rightIp = Ipv4AddressHelper ("10.2.1.0", "255.255.255.0");
  Ipv4AddressHelper routerIp = Ipv4AddressHelper ("10.3.1.0", "255.255.255.0");
  // Assign the router network
  m_routerInterfaces = routerIp.Assign (m_routerDevices);
  // Assign to left side 
  for (uint32_t i = 0; i < LeftCount; ++i)
    {
      NetDeviceContainer ndc;
      ndc.Add (m_leftLeafDevices.Get (i));
      ndc.Add (m_leftRouterDevices.Get (i));
      Ipv4InterfaceContainer ifc = leftIp.Assign (ndc);
      m_leftLeafInterfaces.Add (ifc.Get (0));
      m_leftRouterInterfaces.Add (ifc.Get (1));
      leftIp.NewNetwork ();
    }
  // Assign to right side
  for (uint32_t i = 0; i < RightCount; ++i)
    {
      NetDeviceContainer ndc;
      ndc.Add (m_rightLeafDevices.Get (i));
      ndc.Add (m_rightRouterDevices.Get (i));
      Ipv4InterfaceContainer ifc = rightIp.Assign (ndc);
      m_rightLeafInterfaces.Add (ifc.Get (0));
      m_rightRouterInterfaces.Add (ifc.Get (1));
      rightIp.NewNetwork ();
    }

  NS_LOG_INFO("Setting up simulation!");
  //Set up simulation
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable"));
  ApplicationContainer clientApps;

  for (uint32_t i = 0; i < RightCount; ++i)
    {
      // Create an on/off app sending packets to the same leaf right side
      AddressValue remoteAddress (InetSocketAddress (m_leftLeafInterfaces.GetAddress (i), 1000));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (m_rightLeaf.Get (i)));
    }
    
  // std::string probeName = "ns3::Ipv4PacketProbe";    
  // std::string probeTrace = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
  // GnuplotHelper plotHelper;

  // Configure the plot.  The first argument is the file name prefix
  // for the output files generated.  The second, third, and fourth
  // arguments are, respectively, the plot title, x-axis, and y-axis labels
  // plotHelper.ConfigurePlot ("dumbbell-packet",
  //                           "Packet Byte Count vs. Time",
  //                           "Time (Seconds)",
  //                           "Packet Byte Count");

  // Specify the probe type, probe path (in configuration namespace), and
  // probe output trace source ("OutputBytes") to plot.  The fourth argument
  // specifies the name of the data series label on the plot.  The last
  // argument formats the plot by specifying where the key should be placed.
  // plotHelper.PlotProbe (probeName,
  //                       probeTrace,
  //                       "OutputBytes",
  //                       "Packet Byte Count",
  //                       GnuplotAggregator::KEY_BELOW);

  // // Use FileHelper to write out the packet byte count over time
  // FileHelper fileHelper;

  // // Configure the file to be written, and the formatting of output data.
  // fileHelper.ConfigureFile ("seventh-packet-byte-count",
  //                           FileAggregator::FORMATTED);

  // // Set the labels for this formatted output file.
  // fileHelper.Set2dFormat ("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");

  // // Specify the probe type, probe path (in configuration namespace), and
  // // probe output trace source ("OutputBytes") to write.
  // fileHelper.WriteProbe (probeName,
  //                        probeTrace,
  //                        "OutputBytes");

  clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds (10.0));
  

  //Run simulation
  //
  NS_LOG_INFO("Starting simulation!");
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO("Simulation complete.");
  return 0;
}
