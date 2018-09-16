#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <cinttypes>

int main()
{
	// Step 1. Assume that the client application has already
	// obtained the IP address and protocol port number of the
	// target server.
	std::string_view raw_ip_address = "127.0.0.1";
	uint16_t port_num = 3333;

	try
	{
		// Step 2. Creating an endpoint designating
		// a target server application.
		boost::asio::ip::tcp::endpoint ep(
			boost::asio::ip::make_address(raw_ip_address),
			port_num
		);

		boost::asio::io_context ioc;

		// Step 3. Creating and opening a socket.
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());

		// Step 4. Connecting a socket.
		sock.connect(ep);

		// At this point socket 'sock' is connected to
		// the server application and can be used
		// to send data to or receive data from it.
	}
	// Overloads of boost::asio::ip::make_address and
	// boost::asio::ip::tcp::socket::connect used here throw
	// exceptions in case of error condition.
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error ocured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
