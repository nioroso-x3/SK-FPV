#include "wfb_stats.h"
wfb_stats::wfb_stats(){
  sock = -1;
}

int wfb_stats::open(const char* host, int port){

    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "WFB: Error creating socket." << std::endl;
        sock = -1;
        return -1;
    }

    // Setup server address structure
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &server_address.sin_addr) <= 0) {
        std::cerr << "WFB: Invalid address/ Address not supported" << std::endl;
        sock = -1;
        return -2;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "WFB: Connection failed." << std::endl;
        sock = -1;
        return -3;
    }

    std::cout << "WFB: Connected to wfb " << host << ":" << port << std::endl;
    return 0;

}

ssize_t wfb_stats::recv_fixed(int sock, char* buffer, size_t length) {
    size_t total_received = 0;
    while (total_received < length) {
        ssize_t bytes_received = recv(sock, buffer + total_received, length - total_received, 0);
        if (bytes_received <= 0) {
            return bytes_received; // Return error or closed connection
        }
        total_received += bytes_received;
    }
    return total_received;
}
void wfb_stats::parse_pkt(msgpack::object &msg, int &c_pkt, int &l_pkt){
  for (size_t i = 0; i < msg.via.map.size; ++i) {
    msgpack::object key = msg.via.map.ptr[i].key;
    msgpack::object value = msg.via.map.ptr[i].val;
    std::string key_str;
    if (key.type == msgpack::type::STR) {
      key_str = key.as<std::string>();
      if (key_str == "fec_rec"){
        if (value.type == msgpack::type::ARRAY && value.via.array.size == 2) c_pkt = value.via.array.ptr[0].as<int>();
        else c_pkt = 0;
      }
      if (key_str == "lost"){
        if (value.type == msgpack::type::ARRAY && value.via.array.size == 2) l_pkt = value.via.array.ptr[0].as<int>();
        else l_pkt = 0;
      }
    }
  }
}

void wfb_stats::parse_ant(msgpack::object &msg, int &n_pkt, int &n_rssi, int &n_snr){
  int rssi = -1000;
  int snr = 0;
  int pkt = 0;
  //searches and displays the highest rssi and snr seen by the antennas.
  for (size_t i = 0; i < msg.via.map.size; ++i) {
    msgpack::object key = msg.via.map.ptr[i].key;
    msgpack::object value = msg.via.map.ptr[i].val;
    if (key.type == msgpack::type::ARRAY) {
      if (value.type == msgpack::type::ARRAY && value.via.array.size == 7){
         int t_pkt = value.via.array.ptr[0].as<int>();
         pkt = pkt > t_pkt ? pkt : t_pkt;
         int t_rssi = value.via.array.ptr[2].as<int>();
         rssi = rssi > t_rssi ? rssi : t_rssi;
         int t_snr = value.via.array.ptr[5].as<int>();
         snr = snr > t_snr ? snr : t_snr;         
      }
    }
  }
  n_pkt = pkt;
  n_rssi = rssi;
  n_snr = snr;
}

int wfb_stats::parse_msg(msgpack::object &msg, std::string& name, int &n_rssi, int &n_snr, int &pkt, int &c_pkt, int &l_pkt){
  
  for (size_t i = 0; i < msg.via.map.size; ++i) {
    msgpack::object key = msg.via.map.ptr[i].key;
    msgpack::object value = msg.via.map.ptr[i].val;
    
    std::string key_str;
    if (key.type == msgpack::type::STR) {
      key_str = key.as<std::string>();
      if (key_str == "id"){
         std::string n = value.as<std::string>();
         if (n.find("rx") != std::string::npos) name = n;
         else return -1;

      }
      if (key_str == "packets"){
        parse_pkt(value, c_pkt, l_pkt);
      }
      if (key_str == "rx_ant_stats"){
        parse_ant(value, pkt, n_rssi, n_snr);
      }
    }
  }
  return 0;
}
int wfb_stats::start(std::map<std::string,std::vector<int>> &wfb_rx){
    while (true) {
        // Read the length (4 bytes in network byte order)
        uint32_t length;
        ssize_t bytes_received = recv_fixed(sock, reinterpret_cast<char*>(&length), sizeof(length));
        if (bytes_received <= 0) {
            std::cerr << "WFB: Error receiving length or connection closed." << std::endl;
            return -1;
        }

        // Convert length from network byte order to host byte order
        length = ntohl(length);

        // Validate message length
        if (length > WFB_BUFFER_SIZE) {
            std::cerr << "WFB: Received message length is too large: " << length << " bytes." << std::endl;
            continue;
        }

        // Read the actual message based on the length
        char* message = new char[length + 1];
        bytes_received = recv_fixed(sock, message, length);
        if (bytes_received <= 0) {
            std::cerr << "WFB: Error receiving message or connection closed." << std::endl;
            delete[] message;
            continue;
        }

        // Null-terminate and process the message
        message[length] = '\0';
        msgpack::object_handle oh = msgpack::unpack(message, length);
        msgpack::object deserialized = oh.get();
        try{ 
           std::string name;
           int n_rssi, n_snr, pkt, c_pkt, l_pkt;
           int success = parse_msg(deserialized, name, n_rssi, n_snr, pkt, c_pkt, l_pkt);
           if (success == 0) {
               if (wfb_rx.find(name) == wfb_rx.end()){
                  std::vector<int> data;
                  data.resize(5);
                  wfb_rx.insert(std::make_pair(name,data));
               }
               wfb_rx.at(name)[0] = n_rssi;
               wfb_rx.at(name)[1] = n_snr;
               wfb_rx.at(name)[2] = pkt;
               wfb_rx.at(name)[3] = c_pkt;
               wfb_rx.at(name)[4] = l_pkt;
               //std::cout << name << " " << n_rssi << " " << n_snr << " " << pkt << " "<< c_pkt << " " << l_pkt << std::endl;
           }
        } catch(std::exception e) {
           delete[] message;
           continue;
        }

        delete[] message;
    }
    close(sock);
    return 0;
}

