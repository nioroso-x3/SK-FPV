#ifndef WFB_STATS_H
#define WFB_STATS_H


#include <iostream>
#include <string>
#include <cstring>  // For std::memcpy
#include <thread>
#include <arpa/inet.h> // For htons, ntohs, etc.
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <msgpack.hpp>
#include <map>

#define WFB_BUFFER_SIZE 1048576

class wfb_stats{
  public:
    wfb_stats();
    int open(const char* host, int port);
    //this will start a loop that will update the values in a string to array map for each rx channel found.
    int start(std::map<std::string,std::vector<int>> &wfb_rx);
  private:
    ssize_t recv_fixed(int sock, char* buffer, size_t length);   
    void parse_pkt(msgpack::object &msg, int &c_pkt, int &l_pkt);
    void parse_ant(msgpack::object &msg, int &n_pkt, int &n_rssi, int &n_snr);
    int parse_msg(msgpack::object &msg, std::string& name, int &n_rssi, int &n_snr, int &pkt, int &c_pkt, int &l_pkt);
    int sock;

};

#endif
