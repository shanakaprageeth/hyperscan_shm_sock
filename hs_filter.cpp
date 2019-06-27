/*
 * Copyright (c) 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Hyperscan example program 2: pcapscan
 *
 * This example is a very simple packet scanning benchmark. It scans a given
 * PCAP file full of network traffic against a group of regular expressions and
 * returns some coarse performance measurements.  This example provides a quick
 * way to examine the performance achievable on a particular combination of
 * platform, pattern set and input data.
 *
 * Build instructions:
 *
 *     g++ -std=c++11 -O2 -o pcapscan pcapscan.cc $(pkg-config --cflags --libs libhs) -lpcap
 *
 * Usage:
 *
 *     ./pcapscan [-n repeats] <pattern file> <pcap file>
 *
 * We recommend the use of a utility like 'taskset' on multiprocessor hosts to
 * pin execution to a single processor: this will remove processor migration
 * by the scheduler as a source of noise in the results.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>


#include <cstring>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <unistd.h>

// We use the BSD primitives throughout as they exist on both BSD and Linux.
#define __FAVOR_BSD
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include <pcap.h>

#include </usr/local/include/hs/hs.h> 
#include "headers/door_docker_api.h"
#include "headers/door_docker_api.hpp"

// test
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream, std::stringbuf

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using namespace std::chrono;
// test
using std::stringstream;

using std::unordered_map;
using std::vector;

#define HS_DB_VERSION HS_VERSION_32BIT
#define HS_DB_MAGIC   (0xdbdbdbdbU)

hs_database_t *databaseHS_array[HS_DATABASE_NO];
hs_shared_db_control_t *hs_db_controller;
bridge_door_control_t *bridge_controller;
char *test_signals;



// Key for identifying a stream in our pcap input data, using data from its IP
// headers.
struct FiveTuple {
    unsigned int protocol;
    unsigned int srcAddr;
    unsigned int srcPort;
    unsigned int dstAddr;
    unsigned int dstPort;

    // Construct a FiveTuple from a TCP or UDP packet.
    FiveTuple(const struct ip *iphdr) {
        // IP fields
        protocol = iphdr->ip_p;
        srcAddr = iphdr->ip_src.s_addr;
        dstAddr = iphdr->ip_dst.s_addr;

        // UDP/TCP ports
        const struct udphdr *uh =
            (const struct udphdr *)(((const char *)iphdr) + (iphdr->ip_hl * 4));
        srcPort = uh->uh_sport;
        dstPort = uh->uh_dport;
    }

    bool operator==(const FiveTuple &a) const {
        return protocol == a.protocol && srcAddr == a.srcAddr &&
               srcPort == a.srcPort && dstAddr == a.dstAddr &&
               dstPort == a.dstPort;
    }
};

// A *very* simple hash function, used when we create an unordered_map of
// FiveTuple objects.
struct FiveTupleHash {
    size_t operator()(const FiveTuple &x) const {
        return x.srcAddr ^ x.dstAddr ^ x.protocol ^ x.srcPort ^ x.dstPort;
    }
};


// Packet data to be scanned.
vector<string> packets;
// The stream ID to which each packet belongsvector<size_t> stream_ids;
vector<size_t> stream_ids;
// The stream ID to which each packet belongsvector<size_t> stream_ids;
vector<size_t> stream_active;
// Map used to construct stream_ids
unordered_map<FiveTuple, size_t, FiveTupleHash> stream_map;
// Hyperscan temporary scratch space (used in both modes)
hs_scratch_t *scratch;
 // Vector of Hyperscan stream state (used in streaming mode)
vector<hs_stream_t *> streams;
// Count of matches found during scanning
size_t matchCount;

// Helper function. See end of file.
static bool payloadOffset(const unsigned char *pkt_data, unsigned int *offset,
                          unsigned int *length);


// Match event handler: called every time Hyperscan finds a match.
static
int onMatch(unsigned int id, unsigned long long from, unsigned long long to,
            unsigned int flags, void *ctx) {
    // Our context points to a size_t storing the match count
    size_t *matches = (size_t *)ctx;
    (*matches)++;
    return 0; // continue matching
}

// Simple timing class
class Clock {
public:    
    void start() {
        time_start = high_resolution_clock::now();
    }

    void stop() {
        time_end = high_resolution_clock::now();
    }

    double seconds() const {
        duration<double> time_span = duration_cast<duration<double>>(time_end - time_start);
        return time_span.count();
    }

    double secondsPassed() const {
        duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - time_start);
        return time_span.count();
    }
private:
    //std::chrono::high_resolution_clock::time_point<std::chrono::high_resolution_clock> time_start, time_end;
    high_resolution_clock::time_point time_start;
    high_resolution_clock::time_point time_end;
};

// Read a set of streams from a pcap file
bool readStreams(const char *pcapFile, int maxPackets) {
    // Open PCAP file for input
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcapHandle = pcap_open_offline(pcapFile, errbuf);
    if (pcapHandle == nullptr) {
        cerr << "ERROR: Unable to open pcap file \"" << pcapFile
             << "\": " << errbuf << endl;
        return false;
    }

    struct pcap_pkthdr pktHeader;
    const unsigned char *pktData;
    int packet_count = 0;
    while ((pktData = pcap_next(pcapHandle, &pktHeader)) != nullptr && packet_count<maxPackets) {
        unsigned int offset = 0, length = 0;
        if (!payloadOffset(pktData, &offset, &length)) {
            continue;
        }
        packet_count++;
        // Valid TCP or UDP packet
        const struct ip *iphdr = (const struct ip *)(pktData
                + sizeof(struct ether_header));
        const char *payload = (const char *)pktData + offset;

        size_t id = stream_map.insert(std::make_pair(FiveTuple(iphdr),
                                      stream_map.size())).first->second;

        packets.push_back(string(payload, length));
        stream_ids.push_back(id);
    }
    pcap_close(pcapHandle);

    return !packets.empty();
}

// Return the number of bytes scanned
size_t calc_bytes() {
    size_t sum = 0;
    for (const auto &packet : packets) {
        sum += packet.size();
    }
    return sum;
}

// Display some information about the compiled database and scanned data.
void displayStats() {
    size_t numPackets = packets.size();
    size_t numStreams = stream_map.size();
    size_t numBytes = calc_bytes();
    hs_error_t err;

    cout << numPackets << " packets in " << numStreams
         << " streams, totalling " << numBytes << " bytes." << endl;
    cout << "Average packet length: " << numBytes / numPackets << " bytes."
         << endl;
    cout << "Average stream length: " << numBytes / numStreams << " bytes."
         << endl;
    cout << endl;

    size_t dbStream_size = 0;
    err = hs_database_size(databaseHS_array[0], &dbStream_size);
    if (err == HS_SUCCESS) {
        cout << "Streaming mode Hyperscan database size    : "
             << dbStream_size << " bytes." << endl;
    } else {
        cout << "Error getting streaming mode Hyperscan database size"
             << endl;
    }

    size_t dbBlock_size = 0;
    err = hs_database_size(databaseHS_array[1], &dbBlock_size);
    if (err == HS_SUCCESS) {
        cout << "Standby mode Hyperscan database size        : "
             << dbBlock_size << " bytes." << endl;
    } else {
        cout << "Error getting block mode Hyperscan database size"
             << endl;
    }

    size_t stream_size = 0;
    err = hs_stream_size(databaseHS_array[0], &stream_size);
    if (err == HS_SUCCESS) {
        cout << "Streaming mode Hyperscan stream state size: "
             << stream_size << " bytes (per stream)." << endl;
    } else {
        cout << "Error getting stream state size" << endl;
    }
    err = hs_stream_size(databaseHS_array[1], &stream_size);
    if (err == HS_SUCCESS) {
        cout << "Streaming mode Hyperscan stream state size: "
             << stream_size << " bytes (per stream)." << endl;
    } else {
        cout << "Error getting stream state size" << endl;
    }
}

static void usage(const char *prog) {
    cerr << "Usage: " << prog << " [-n repeats] <pattern file> <pcap file>" << endl;
}

// Main entry point.
int main(int argc, char **argv) {

    
    unsigned int repeatCount = 1;

    // Process command line arguments.
    int opt;
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
        case 'n':
            repeatCount = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(-1);
        }
    }

    //if (argc - optind != 2) {
    //    usage(argv[0]);
    //    exit(-1);
    //}
    const char *pcapFile = argv[1];
    const char *test_signal_file_name = "/home/shanaka/repos/test_signals";   
    const char *bridge_door_control_file_name = "/home/shanaka/repos/bridge_door_control";
    const char *hd_control_file_name = "/home/shanaka/repos/hs_db_control";
    const char *hd_db_file_name = "/home/shanaka/repos/hs_db_handle";
    const char *hd_db_sb_file_name = "/home/shanaka/repos/hs_db_handle_standby";
     
    int options = PROT_READ | PROT_WRITE;
    int mode = MAP_SHARED;
    test_signals = (char *)set_up_mmap_slave(test_signal_file_name,  options, mode);
    bridge_controller = (bridge_door_control_t *)set_up_mmap_slave(bridge_door_control_file_name,  options, mode);
    hs_db_controller = (hs_shared_db_control_t *)(set_up_mmap_slave(hd_control_file_name,  options, mode));
    databaseHS_array[0] = (hs_database_t*)(set_up_mmap_slave(hd_db_file_name,  options, mode));  
    databaseHS_array[1] = (hs_database_t*)(set_up_mmap_slave(hd_db_sb_file_name,  options, mode));

    int packetLimit = 1000000;
    readStreams(pcapFile,packetLimit);
    hs_db_controller->running[0]= 0;
    hs_db_controller->running[1]= 0;

    cout<< packets.size() <<endl;
    hs_db_controller->newest_database = 0;
    if (hs_alloc_scratch(databaseHS_array[hs_db_controller->newest_database], &scratch) != HS_SUCCESS) {
      fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
      hs_free_database(databaseHS_array[hs_db_controller->newest_database]);
      exit (-1);
    }
     Clock clock;
     Clock clock_db;
    size_t bytes = calc_bytes();
    // Streaming mode scans.
    double secsStreamingScan = 0.0, secsStreamingOpenClose = 0.0;
    double secsDatabaseChanageStart = 0.0;
    double secsDatabaseChanageEnd = 0.0;
    clock.start();
    cout << "open stream start at : " << clock.secondsPassed()<< endl;
    secsStreamingOpenClose += clock.seconds();
    test_signals[0] = 0x1;
    for (int i =0; i < packets.size();i++){
        //change database if chage db required
        if (hs_db_controller->change_db == 1){
            cout << "db change start at : " << clock.secondsPassed()<< endl;
            secsDatabaseChanageStart = clock.secondsPassed();
            if (hs_alloc_scratch(databaseHS_array[hs_db_controller->init_database], &scratch) != HS_SUCCESS) {
              fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
              hs_free_database(databaseHS_array[hs_db_controller->newest_database]);
              exit (-1);
            } 
            hs_db_controller->newest_database = hs_db_controller->init_database;
            hs_db_controller->change_db = 0;
            cout << "db change end at   : " << clock.secondsPassed()<< endl;
            secsDatabaseChanageEnd = clock.secondsPassed();
        }
        const std::string &pkt = packets[i];
        if((stream_ids[i]+1)> streams.size()){
            streams.resize(stream_ids[i]+1);
            stream_active.resize(stream_ids[i]+1);
        }
        if(stream_active[stream_ids[i]] != 1){
            hs_error_t err = hs_open_stream(databaseHS_array[hs_db_controller->newest_database], 0, &streams[stream_ids[i]]);
            if (err != HS_SUCCESS) {
                cerr << "ERROR: Unable to open stream. Exiting." << endl;
                exit(-1);
            }
            hs_db_controller->running[hs_db_controller->newest_database] += 1;
            stream_active[stream_ids[i]] = 1;
        }
        hs_error_t err = hs_scan_stream(streams[stream_ids[i]],
                                            pkt.c_str(), pkt.length(), 0,
                                            scratch, onMatch, &matchCount);
        if (err != HS_SUCCESS) {
            cerr << "ERROR: Unable to scan packet. Exiting." << endl;
            exit(-1);
        }
    }
    cout << "scan stream ended at : " << clock.secondsPassed()<< endl;
        clock.stop();
        secsStreamingScan += clock.seconds();
        secsStreamingOpenClose += clock.seconds();
        // Collect data from streaming mode scans.
    
    double tputStreamScanning = (bytes * 8 * repeatCount) / secsStreamingScan;
    double tputStreamOverhead = (bytes * 8 * repeatCount) / (secsStreamingScan + secsStreamingOpenClose);
    size_t matchesStream = matchCount;
    double matchRateStream = matchesStream / ((bytes * repeatCount) / 1024.0); // matches per kilobyte
    double secsDatabaseChanage = (double)(1000000*(secsDatabaseChanageEnd - secsDatabaseChanageStart)); 
    
    cout << endl << "Streaming mode:" << endl << endl;
     cout << std::fixed << std::setprecision(4);

    cout << "  Database Change time:   "
              << secsDatabaseChanage << " us" << endl;
    cout << "  Total matches: " << matchesStream << endl;
    cout << std::fixed << std::setprecision(4);
    cout << "  Match rate:    " << matchRateStream << " matches/kilobyte" << endl;
    cout << std::fixed << std::setprecision(2);
    cout << "  Throughput (with stream overhead): "
              << tputStreamOverhead/1000000 << " megabits/sec" << endl;
    cout << "  Throughput (no stream overhead):   "
              << tputStreamScanning/1000000 << " megabits/sec" << endl;
    

    if (bytes < (2*1024*1024)) {
        cout << endl << "WARNING: Input PCAP file is less than 2MB in size." << endl
                  << "This test may have been too short to calculate accurate results." << endl;
    }
    test_signals[0] = 0x0;


    return 0;
}


/**
 * Helper function to locate the offset of the first byte of the payload in the
 * given ethernet frame. Offset into the packet, and the length of the payload
 * are returned in the arguments @a offset and @a length.
 */
static
bool payloadOffset(const unsigned char *pkt_data, unsigned int *offset,
                   unsigned int *length) {
    const ip *iph = (const ip *)(pkt_data + sizeof(ether_header));
    const tcphdr *th = nullptr;

    // Ignore packets that aren't IPv4
    if (iph->ip_v != 4) {
        return false;
    }

    // Ignore fragmented packets.
    if (iph->ip_off & htons(IP_MF | IP_OFFMASK)) {
        return false;
    }

    // IP header length, and transport header length.
    unsigned int ihlen = iph->ip_hl * 4;
    unsigned int thlen = 0;

    switch (iph->ip_p) {
    case IPPROTO_TCP:
        th = (const tcphdr *)((const char *)iph + ihlen);
        thlen = th->th_off * 4;
        break;
    case IPPROTO_UDP:
        thlen = sizeof(udphdr);
        break;
    default:
        return false;
    }

    *offset = sizeof(ether_header) + ihlen + thlen;
    *length = sizeof(ether_header) + ntohs(iph->ip_len) - *offset;

    return *length != 0;
}

