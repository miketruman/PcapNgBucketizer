#include "MetaInfo.h"
#include <iostream>
#include "pcapplusplus/PacketUtils.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "pcapplusplus/IPv4Layer.h"
#include "pcapplusplus/IPv6Layer.h"
#include "pcapplusplus/TcpLayer.h"
#include "pcapplusplus/UdpLayer.h"
#include "pcapplusplus/EthLayer.h"
#include "pcapplusplus/VlanLayer.h"
#include "pcapplusplus/PPPoELayer.h"
#include "boost/lexical_cast.hpp"


MetaInfo::MetaInfo(std::string const &a, std::string const& b, std::string const& c)
    {
        m_root.put("a", a);
        m_root.put("b", b);
        m_root.put("c", c);
    }
void MetaInfo::addLayer(std::string const& s,std::string const& d, std::string const& t)
    {
        pt::ptree layer;
        if( ! s.empty())
            layer.put("s", s);
        if( ! d.empty())
            layer.put("d", d);
        layer.put("t", t);
        m_layers.push_back(std::make_pair("", layer));
    }
void MetaInfo::addLayer(uint16_t const& s, uint16_t const& d, std::string const& t)
    {
        pt::ptree layer;
        layer.put("s", s);
        layer.put("d", d);
        layer.put("t", t);
        m_layers.push_back(std::make_pair("", layer));
    }
    std::string MetaInfo::toString()
    {
        m_root.add_child("l", m_layers);
        std::stringstream ss;
        pt::write_json(ss, m_root, false);
//	std::cout << ss.str() << std::endl;
        return ss.str();
    }
    void MetaInfo::parseLayers(pcpp::RawPacket &rawPacket)
    {
        pcpp::Packet parsedPacket(&rawPacket, false, pcpp::TCP | pcpp::UDP);
        pcpp::Layer* layer= parsedPacket.getFirstLayer();
        while (layer != NULL)
        {
            switch(layer->getProtocol())
            {
            case pcpp::Ethernet:
            {
                pcpp::EthLayer* ethLayer = (pcpp::EthLayer*)layer;
                addLayer(ethLayer->getSourceMac().toString(),ethLayer->getDestMac().toString(),"Eth");
                break;
            }
            case pcpp::IPv4:
            {
                pcpp::IPv4Layer* ipv4Layer = (pcpp::IPv4Layer*)layer;
                addLayer(ipv4Layer->getSrcIpAddress().toString(),ipv4Layer->getDstIpAddress().toString(),"IPv4");
                break;
            }

            case pcpp::TCP:
            {
                pcpp::TcpLayer* tcpLayer = (pcpp::TcpLayer*)layer;
                addLayer(be16toh(tcpLayer->getTcpHeader()->portSrc),be16toh(tcpLayer->getTcpHeader()->portDst),"TCP");
                break;
            }
            case pcpp::UDP:
            {
                pcpp::UdpLayer* udpLayer = (pcpp::UdpLayer*)layer;
                addLayer(be16toh(udpLayer->getUdpHeader()->portSrc),be16toh(udpLayer->getUdpHeader()->portDst),"UDP");
                break;
            }
            case pcpp::IPv6:
            {
                pcpp::IPv6Layer* ipv6Layer = (pcpp::IPv6Layer*)layer;
                addLayer(ipv6Layer->getSrcIpAddress().toString(),ipv6Layer->getDstIpAddress().toString(),"IPv6");
                break;
            }
            case pcpp::MPLS:
            {
                addLayer("","","MPLS");
                break;
            }
            case pcpp::GREv1:
            case pcpp::GREv0:
            case pcpp::GRE:
                addLayer("","","GRE");
                break;
            case pcpp::PPPoE:
                addLayer("","","PPPoE");
                break;
            case pcpp::PPP_PPTP:
                addLayer("","","PPP_PPTP");
                break;
            case pcpp::VLAN:
            {
                pcpp::VlanLayer* vlanLayer = (pcpp::VlanLayer*)layer;
                addLayer("",boost::lexical_cast<std::string>(vlanLayer->getVlanID()),"VLAN");
                break;
            }
            default:
                break;
            };
            layer = layer->getNextLayer();

        }
    }

