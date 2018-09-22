#include <boost/asio.hpp>
#include <iostream>
#include <string>

std::string readFromSocketEnhanced(boost::asio::ip::tcp::socket& sock)
{
	constexpr std::size_t MESSAGE_SIZE = 7;
	std::string buf(MESSAGE_SIZE, 0);

	std::size_t total_bytes_read = 
		boost::asio::read(
			sock,
			boost::asio::buffer(
				buf.data(),
				MESSAGE_SIZE
			)
		);
	
	// resize to total_bytes_read + '\0'
	buf.resize( total_bytes_read + 1);

	return buf;
}

// Run 'echo "" | awk '{  print "Hello\n" }' | netcat -l 127.0.0.1 -p 3333' before start
// to read "Hello\n" to current client
int main()
{
	std::string_view raw_ip_address = "127.0.0.1";
	std::uint16_t port_num = 3333;

	try
	{
		boost::asio::ip::tcp::endpoint ep(
			boost::asio::ip::make_address(raw_ip_address),
			port_num
		);

		boost::asio::io_context ioc;

		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());

		sock.connect(ep);

		std::cout << "readFromSocketEnhanced returns: '" << readFromSocketEnhanced(sock) << "'\n";
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
