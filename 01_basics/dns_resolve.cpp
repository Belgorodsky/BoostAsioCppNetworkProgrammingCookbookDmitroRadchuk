/* dns_resolve.cpp */
#include <boost/asio.hpp>
#include <iostream>

int main()
{
	// Step 1. Assume that the client application has already
	// obtained the DNS name and protocol port number and
	// represented them as strings.
	std::string_view host = "google.com";
	std::string_view port = "443";

	// Step 2.
	boost::asio::io_context ioc;

	// Step 3. Creating a query is deprecated 
	// Skip this step

	// Step 4. Creating  a resolver 
	boost::asio::ip::tcp::resolver resolver(ioc);

	// Used to store information about error that happens
	// during the resolution process.
	boost::system::error_code ec;

	// Step 5.
	auto it = resolver.resolve(
		host,
		port,
		boost::asio::ip::tcp::resolver::query::numeric_service,
		ec
	);

	// Handling errors if any
	if (ec)
	{
		// Failed to resolve the DNS name. Breaking execution.
		std::cerr << "Failed to resolve a DNS name. "
		<< "Error code = " << ec.value()
		<< ". Message = " << ec.message()
		<< '\n';

		return ec.value();
	}

	std::cout << host << ':' << port << " resolved to " << it->endpoint() << '\n';

	return 0;
}
