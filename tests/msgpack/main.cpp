#include <iostream>
#include <msgpack.hpp>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

const int BUFFER_SIZE = 4096;

int main() {
    const char* host = "127.0.0.1"; // Replace with the desired host
    const int port = 8003;         // Replace with the desired port
    std::cout << "msgpack::type::NIL: " << msgpack::type::NIL << std::endl;
    std::cout << "msgpack::type::BOOLEAN: " << msgpack::type::BOOLEAN << std::endl;
    std::cout << "msgpack::type::POSITIVE_INTEGER: " << msgpack::type::POSITIVE_INTEGER << std::endl;
    std::cout << "msgpack::type::NEGATIVE_INTEGER: " << msgpack::type::NEGATIVE_INTEGER << std::endl;
    std::cout << "msgpack::type::FLOAT32: " << msgpack::type::FLOAT32 << std::endl;
    std::cout << "msgpack::type::FLOAT64: " << msgpack::type::FLOAT64 << std::endl;
    std::cout << "msgpack::type::STR: " << msgpack::type::STR << std::endl;
    std::cout << "msgpack::type::BIN: " << msgpack::type::BIN << std::endl;
    std::cout << "msgpack::type::EXT: " << msgpack::type::EXT << std::endl;
    std::cout << "msgpack::type::ARRAY: " << msgpack::type::ARRAY << std::endl;
    std::cout << "msgpack::type::MAP: " << msgpack::type::MAP << std::endl;

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

    char buffer[BUFFER_SIZE];

    // Receive data from the server
    ssize_t received_size = recv(sock, buffer, BUFFER_SIZE, 0);
    if (received_size < 0) {
        std::cerr << "Error receiving data." << std::endl;
        close(sock);
        return -1;
    }
    std::cout << "Received " << received_size << " bytes\n";
    for(int i = 0; i < received_size; ++i){
       std::cout << buffer[i] << " ";
       if (i %8 == 0) std::cout << "\n";
    }
    // Process received data with MessagePack
    try {
        msgpack::object_handle oh = msgpack::unpack(buffer, received_size);
        msgpack::object deserialized = oh.get();

        // Print the deserialized data
        std::cout << "Received data: " << deserialized << std::endl;
        std::cout << "MSG Type: " << deserialized.type << "\n";
	std::cout << msgpack::type::POSITIVE_INTEGER << "\n";
        if (deserialized.type == msgpack::type::POSITIVE_INTEGER) {
		int v = deserialized.as<int>();
		std::cout << "value: " << v << "\n";
	}	       

    } catch (const std::exception& ex) {
        std::cerr << "Error deserializing data: " << ex.what() << std::endl;
    }

    // Close the socket
    close(sock);

    return 0;
}
