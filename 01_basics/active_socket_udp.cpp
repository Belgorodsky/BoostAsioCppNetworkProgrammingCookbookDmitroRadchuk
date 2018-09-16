/* active_socket_udp.cpp */
#include <boost/asio.hpp>
#include <iostream>

int main()
{
	// Step 1. An instance of 'io_context' class is required by
	// socket constructor.
	boost::asio::io_context ioc;

	// Step 2. Creating an object of 'udp' class representing 
	// a UDP protocol with IPv6 as underlining protocol.
	boost::asio::ip::udp protocol = boost::asio::ip::udp::v6();

	// Step 3. Instantiating an active UDP socket object.
	boost::asio::ip::udp::socket sock(ioc);

	// Used to store information about error that happens
	// while opening the socket.
	boost::system::error_code ec;

	// Step 4. Opening the socket.
	sock.open(protocol, ec);

	if (ec)
	{
		// Failed to open the socket.
		std::cerr 
			<< "Failed to open the socket! Error code = "
			<< ec.value() << ". Message: " << ec.message()
			<< '\n';
			return ec.value();
	}

	return 0;
}
