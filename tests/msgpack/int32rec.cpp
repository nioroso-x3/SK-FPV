#include <iostream>
#include <sstream>
#include <string>
#include <cstring>  // For std::memcpy
#include <arpa/inet.h> // For htons, ntohs, etc.
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <msgpack.hpp>

// Buffer size for receiving messages
const int BUFFER_SIZE = 1024*1024;

// Function to receive a fixed number of bytes
ssize_t recv_fixed(int sock, char* buffer, size_t length) {
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

void parse_pkt(msgpack::object &msg, int &c_pkt, int &l_pkt){
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

void parse_ant(msgpack::object &msg, int &n_pkt, int &n_rssi, int &n_snr){
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

int parse_msg(msgpack::object &msg, std::string& name, int &n_rssi, int &n_snr, int &pkt, int &c_pkt, int &l_pkt){
  
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


// Main function implementing an Int32StringReceiver-like behavior
int main() {
    const char* host = "127.0.0.1"; // Replace with the desired host
    const int port = 8003;         // Replace with the desired port

    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error creating socket." << std::endl;
        return -1;
    }

    // Setup server address structure
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Connection failed." << std::endl;
        return -1;
    }

    std::cout << "Connected to " << host << ":" << port << std::endl;

    while (true) {
        // Read the length (4 bytes in network byte order)
        uint32_t length;
        ssize_t bytes_received = recv_fixed(sock, reinterpret_cast<char*>(&length), sizeof(length));
        if (bytes_received <= 0) {
            std::cerr << "Error receiving length or connection closed." << std::endl;
            break;
        }

        // Convert length from network byte order to host byte order
        length = ntohl(length);

        // Validate message length
        if (length > BUFFER_SIZE) {
            std::cerr << "Received message length is too large: " << length << " bytes." << std::endl;
            break;
        }

        // Read the actual message based on the length
        char* message = new char[length + 1];
        bytes_received = recv_fixed(sock, message, length);
        if (bytes_received <= 0) {
            std::cerr << "Error receiving message or connection closed." << std::endl;
            delete[] message;
            break;
        }

        // Null-terminate and display the message
        message[length] = '\0';
        msgpack::object_handle oh = msgpack::unpack(message, length);
        msgpack::object deserialized = oh.get();
        std::stringstream ss;
        ss << deserialized;
        //std::cout << ss.str() << "\n";
        try{ 
           std::string name;
           int n_rssi, n_snr, pkt, c_pkt, l_pkt;
           int success = parse_msg(deserialized, name, n_rssi, n_snr, pkt, c_pkt, l_pkt);
           if (success == 0) std::cout << name << " " << n_rssi << " " << n_snr << " " << pkt << " " << c_pkt << " " << l_pkt << "\n";
        } catch(std::exception e) {
           delete[] message;
           continue;
        }

        delete[] message;
    }

    // Close the socket
    close(sock);
    return 0;
}
