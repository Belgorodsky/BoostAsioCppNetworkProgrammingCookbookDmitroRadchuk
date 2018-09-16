/* endpoint_server.cpp */
#include <boost/asio.hpp>
#include <cinttypes>

int main()
{
	// Step 1. Here we assume that the server application has
	// already obtained the protocol port number
	uint16_t port_num = 3333;

	// Stap 2. Create special object of boost::asio::ip::address
	// class that specifies all IP-addresses available on host.
	// Note that here we assume that server works over IPv6 protocol.
	boost::asio::ip::address ip_address = boost::asio::ip::address_v6::any();

	// Step 3. 
	boost::asio::ip::tcp::endpoint ep(ip_address, port_num);

	// Step 4. The endpoint is created and can be used to
	// specify the IP addresses and a port number on which
	// the server application wants to listen for incoming
	// connections
}
