#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <cinttypes>

int main()
{
	// Step 1. Assume that the client application has already
	// obtained the DNS name and protocol port number and
	// represented them as strings
	std::string_view host = "www.boost.org";
	std::string_view port = "443";

	// Used by a 'resolver' and a 'socket'
	boost::asio::io_context ioc;

	// Creating a resolver's query is deprecated.
	// Skip this step

	// Creating  a resolver 
	boost::asio::ip::tcp::resolver resolver(ioc);

	try
	{
		// Step 2. Resolving a DNS name.
		auto it = resolver.resolve(
			host,
			port,
			boost::asio::ip::tcp::resolver::query::numeric_service
		);

		// Step 3. Creating a socket.
		boost::asio::ip::tcp::socket sock(ioc);

		// Step 4. boost::asio::connect method iterates over
		// each endpoint until successfully connects to one
		// of them. It will throw an exception if it fails
		// to connect to all the endpoints or if other
		// error occurs.
		boost::asio::connect(sock, it);

		// At this point socket 'sock' is connected to
		// the server application and can be used
		// to send data or receive data from it.
	}
	// Overloads of boost::asio::ip::tcp::resolver::resolve and
	// boost::asio::connect used here throw
	// exceptions in case of error condition.
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
