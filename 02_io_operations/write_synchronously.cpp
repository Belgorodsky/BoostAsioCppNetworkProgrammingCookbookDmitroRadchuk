#include <boost/asio.hpp>
#include <iostream>

void writeToSocket(boost::asio::ip::tcp::socket& sock)
{
	// Step 2. Allocating and filling the buffer.
	std::string_view buf = "Hello\n";

	std::size_t total_bytes_written = 0;

	// Step 3. Run the loop until all data is written
	// to the socket.
	while (total_bytes_written != buf.length())
	{
		total_bytes_written += sock.write_some(
			boost::asio::buffer(
				buf.data() + total_bytes_written,
				buf.length() - total_bytes_written
			)
		);
	}
}

// Run 'netcat -l 127.0.0.1 -p 3333' before start
// to get message from current client
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

		// Step 1. Allocating and opening the socket.
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());

		sock.connect(ep);

		writeToSocket(sock);
	}
	catch(boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code() 
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
