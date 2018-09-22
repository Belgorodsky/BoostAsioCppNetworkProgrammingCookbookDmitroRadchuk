#include <boost/asio.hpp>
#include <iostream>
#include <string>

std::string readFromSocket(boost::asio::ip::tcp::socket& sock)
{
	constexpr std::size_t MESSAGE_SIZE = 7;
	std::string buf(MESSAGE_SIZE, 0);
	std::size_t total_bytes_read = 0;

	while (total_bytes_read != MESSAGE_SIZE)
	{
		total_bytes_read += sock.receive(
			boost::asio::buffer(
				buf.data() + total_bytes_read,
				MESSAGE_SIZE - total_bytes_read
			),
			boost::asio::socket_base::message_do_not_route
		);
	}

	return buf;
}

// Run 'echo "" | awk '{  print "Hello\n" }' | netcat -l 127.0.0.1 -p 3333' before start
// to receive "Hello\n" to current client
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

		std::cout << "readFromSocket returns: '" << readFromSocket(sock) << "'\n";
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
