/* dns_resolve.cpp */

#include <boost/asio.hpp>
#include <string>
#include <cinttypes>
#include <iostream>

int main()
{
	std::string_view host = "yandex.ru";
	std::string_view port = "443";

	boost::asio::io_context ioc;

	boost::asio::ip::tcp::resolver  resolver(ioc);

	boost::system::error_code ec;
	boost::asio::ip::tcp::resolver::iterator end;
	for(
		auto it = resolver.resolve(host, port, ec);
		it != end;
		++it
	)
	{
		if (ec)
		{
			std::cerr  
				<< "Error code = " << ec.value() << ". Message: "
				<< ec.message() << '\n';
		}
		else
		{
			boost::asio::ip::tcp::endpoint ep = *it;
			std::cout << host << ':' << port << " is " <<  ep << '\n';
		}
	}

	
	return 0;
}
