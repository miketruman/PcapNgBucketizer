#include <iostream>
#include "pcapplusplus/RawPacket.h"
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

class MetaInfo
{
public:
    MetaInfo(std::string const &a, std::string const& b, std::string const& c);
    std::string toString();
    void parseLayers(pcpp::RawPacket &rawPacket);
private:
    void addLayer(std::string const& s,std::string const& d, std::string const& t);
    void addLayer(uint16_t const& s, uint16_t const& d, std::string const& t);

    pt::ptree m_root, m_layers;
};

