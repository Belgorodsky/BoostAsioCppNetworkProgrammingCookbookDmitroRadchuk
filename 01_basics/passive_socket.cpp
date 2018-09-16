/* passive_socket.cpp */
#include <boost/asio.hpp>
#include <iostream>

int main()
{
	// Step 1. An instance of 'io_context' class is required by 
	// socket constructor.
	boost::asio::io_context ioc;

	// Step 2. Creating an object  of 'tcp' class representing
	// a TCP protocol with IPv6 as underlying protocol.
	auto protocol = boost::asio::ip::tcp::v6();

	// Step 3. Instantiating an acceptor socket object.
	boost::asio::ip::tcp::acceptor acceptor(ioc);

	// Used to store information about error that happens
	// while opening the acceptor socket.
	boost::system::error_code ec;

	// Step 4. Opening the acceptor socket.
	acceptor.open(protocol, ec);

	if (ec)
	{
		// Failed to open the socket.
		std::cerr 
			<< "Failed to open the acceptor socket! "
			<< "Error code = " << ec.value() << ". Message: "
			<< ec.message() << '\n';

		return ec.value();
	}

	std::cin.get();

	return 0;
}
