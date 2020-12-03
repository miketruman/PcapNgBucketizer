all:
	g++ main.cpp MetaInfo.cpp --static -lPcap++ -lPacket++ -lCommon++ -lboost_iostreams  -lpcap -lpthread -lboost_program_options -lboost_regex -lboost_filesystem -lboost_system -lz -lpthread  -std=c++11 -o a.bin
	./a.bin -f gre_and_4over6.cap
