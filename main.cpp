#include "MetaInfo.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include "pcapplusplus/PcapFileDevice.h"
#include "pcapplusplus/PacketUtils.h"
#include "pcapplusplus/PcapLiveDeviceList.h"

using namespace boost::program_options;

std::string output = "output", file, interfaceIP, comment;

size_t bucketTimeout = 60;

class PcapFile
{
public:
    typedef std::set<uint32_t> HashSet;
    PcapFile(uint32_t time) :m_time(time), m_count(0)
    {
        m_fileName = output + "/." + boost::lexical_cast<std::string>(time) + ".pcapng";
        m_pcapNgWriter = new pcpp::PcapNgFileWriterDevice(m_fileName.c_str());
        m_pcapNgWriter->open();
    }
    void write(pcpp::RawPacket &rawPacket, uint32_t const& hash, bool firstPacket )
    {
        if(firstPacket)
        {
            MetaInfo metaInfo("t1","t2","t3");
            metaInfo.parseLayers(rawPacket);
            m_pcapNgWriter->writePacket(rawPacket, metaInfo.toString().c_str());
            m_hashSet.insert(hash);
        }
        else
        {
            m_pcapNgWriter->writePacket(rawPacket);
        }

        ++m_count;
    }
    ~PcapFile()
    {
        m_pcapNgWriter->close();
        std::cout << "file= " << boost::lexical_cast<std::string>(m_time) << " hash.size=" << m_hashSet.size() << " count=" << m_count << std::endl;
        std::string fileName = output + "/" + boost::lexical_cast<std::string>(m_time) + ".pcapng";
        boost::filesystem::rename(m_fileName,fileName);
    }
    HashSet const& getHashSet() const
    {
        return m_hashSet;
    }
private:
    pcpp::PcapNgFileWriterDevice *m_pcapNgWriter;
    HashSet m_hashSet;
    uint32_t m_time;
    uint32_t m_count;
    std::string m_fileName;
};

class WritePacket
{
    typedef boost::shared_ptr<PcapFile> PcapFilePointer;
    typedef std::map<uint32_t, uint32_t> HashMap;
    typedef std::map<uint32_t, PcapFilePointer> TimePcapFile;
    void writeFile(uint32_t time, pcpp::RawPacket &rawPacket, uint32_t const& hash, bool firstPacket)
    {
        TimePcapFile::const_iterator itr = m_timePcapFile.find(time);
        if(itr != m_timePcapFile.end())
        {
            itr->second->write(rawPacket, hash, firstPacket);
        }
        else
        {
            PcapFilePointer pcapFilePtr(new PcapFile(time));
            m_timePcapFile.insert(std::pair<uint32_t, PcapFilePointer>(time, pcapFilePtr));
            pcapFilePtr->write(rawPacket, hash, firstPacket);
        }
        clock(time);
    }
public:
    void write(pcpp::RawPacket &rawPacket)
    {
        pcpp::Packet parsedPacket(&rawPacket, false, pcpp::TCP | pcpp::UDP);

        uint32_t hash = pcpp::hash5Tuple(&parsedPacket,true);
        HashMap::const_iterator itr = m_hashMap.find(hash);
        if(itr != m_hashMap.end())
        {
            writeFile(itr->second, rawPacket, hash, false);
        }
        else
        {
            timespec timestamp = rawPacket.getPacketTimeStamp();
            m_hashMap[hash] = timestamp.tv_sec;
            writeFile(timestamp.tv_sec, rawPacket, hash, true);
        }

    }
    void clock(uint32_t const& time)
    {
        if( ! m_timePcapFile.empty())
        {
            TimePcapFile::const_iterator it = m_timePcapFile.begin();
            if((it->first + bucketTimeout) < time)
            {
                PcapFile::HashSet const& hashSet = it->second->getHashSet();
                for(PcapFile::HashSet::const_iterator del = hashSet.begin(); del != hashSet.end(); ++del)
                {
                    m_hashMap.erase(*del);
                }
                m_timePcapFile.erase(it);
            }
        }
    }
private:
    HashMap m_hashMap;
    TimePcapFile m_timePcapFile;
};

static void onPacketArrive(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{
    // extract the stats object form the cookie
    WritePacket* writePacket = (WritePacket*)cookie;
    writePacket->write(*packet);
}


int main(int argc, char* argv[])
{
    options_description desc("Allowed options");
    desc.add_options()

    ("help,h", "print usage message")
    ("output,o", value(&output), "pathname for output")
    ("file,f",value(&file), "pathname for file input")
    ("interface,i",value(&file), "ip of capture device interface")
    ("comment,c",value(&comment), "add comment to each packet")
    ("buckettimeout,b",value(&bucketTimeout), "bucket timeout");

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);


    if (vm.count("help") || vm.empty()) {
        std::cout << desc << "\n";
        return 0;
    }
    if (vm.count("output")) {
        output = vm["output"].as<std::string>();
    }
    if (vm.count("interface")) {
        interfaceIP = vm["interface"].as<std::string>();
    }
    if( !output.empty() && boost::filesystem::create_directory(output))
    {
        std::cerr<< "Directory Created: "<< output <<std::endl;
    }
    if (vm.count("file")) {
        file = vm["file"].as<std::string>();
    }
    if (vm.count("comment")) {
        comment = vm["comment"].as<std::string>();
    }
    if (vm.count("buckettimeout")) {
        bucketTimeout = vm["buckettimeout"].as<size_t>();
    }
    boost::filesystem::create_directory(output);

    WritePacket writePacket;

    if( !file.empty())
    {
        pcpp::PcapFileReaderDevice pcapReader(file.c_str());
        pcapReader.open();

        pcpp::RawPacket rawPacket;

        while (pcapReader.getNextPacket(rawPacket)) {
            writePacket.write(rawPacket);
        }
        pcapReader.close();
    }
    else if(!interfaceIP.empty())
    {
        std::cout << "interface=" << interfaceIP << std::endl;
        pcpp::PcapLiveDevice* dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIP.c_str());
        if (dev == NULL)
        {
            printf("Cannot find interface with IPv4 address of '%s'\n", interfaceIP.c_str());
            exit(1);
        }
        if (!dev->open())
        {
            printf("Cannot open device\n");
            exit(1);
        }
        dev->startCapture(onPacketArrive, &writePacket);
        while(true)
        {
            sleep(1);
        }
    }

}
