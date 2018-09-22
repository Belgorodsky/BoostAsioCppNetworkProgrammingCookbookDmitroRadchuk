#include <boost/asio.hpp>
#include <iostream>
#include <string>

std::string readFromSocketDelim(boost::asio::ip::tcp::socket& sock)
{
	std::string ret_val;
	boost::asio::streambuf buf;
	// Synchronously read data from the socket until
	// '\n' symbol is encountered.
	boost::asio::read_until(sock, buf, '\n');
	
	// Because buffer 'buf' may contain some other data
	// after '\n' symbol, we have to parse the buffer and
	// extract only symbols before the delimiter.

	std::istream input_stream(&buf);
	std::getline(input_stream, ret_val);

	return ret_val;
}

// Run 'echo "" | awk '{  print "Hello \nworld \n" }' | netcat -l 127.0.0.1 -p 3333'
// before start
// to read "Hello " to current client
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

		std::cout << "readFromSocketDelim returns: '" << readFromSocketDelim(sock) << "'\n";
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
