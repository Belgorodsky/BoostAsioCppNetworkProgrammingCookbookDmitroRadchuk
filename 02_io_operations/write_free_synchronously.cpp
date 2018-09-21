#include <boost/asio.hpp>
#include <iostream>

void writeToSocketEnhanced(boost::asio::ip::tcp::socket& sock)
{
	// Step 2. Allocating and filling the buffer.
	std::string_view buf = "Hello\n";

	// Step 3. Run the loop until all data is written
	// to the socket.
	std::size_t total_bytes_written = 
		boost::asio::write(
			sock,
			boost::asio::buffer(buf)
	);

	std::cout << "total_bytes_written: " << total_bytes_written << '\n';
}

// Run 'netcat -l 127.0.0.1 -p 3333' before start 
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

		writeToSocketEnhanced(sock);
	}
	catch(boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code() 
		<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}
