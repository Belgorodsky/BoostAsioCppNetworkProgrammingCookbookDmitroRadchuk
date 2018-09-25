#include <boost/asio.hpp>
#include <boost/current_function.hpp>
#include <iostream>

void processRequest(boost::asio::ip::tcp::socket &sock)
{
	// We use extensible buffer because we don't
	// know the size of the request message.
	boost::asio::streambuf request_buf;

	boost::system::error_code ec;

	// Receiving the request.
	boost::asio::read(sock, request_buf, ec);

	std::cout << BOOST_CURRENT_FUNCTION <<  " Message: \n";
	std::istream is(&request_buf);
	for (std::string line; std::getline(is, line); )
	{
		std::cout << line << '\n';
	}
	std::cout << "\nEnd of message\n";

	if (ec != boost::asio::error::eof)
		throw boost::system::system_error(ec);

	// Request received. Sending response
	// Allocating and filling the buffer with
	// binary data.
	constexpr std::array<char, 3> response_buf = { 0x48, 0x69, 0x21 };

	// Sending the request data.
	boost::asio::write(sock, boost::asio::buffer(response_buf));

	// Shutting down the socket to let the 
	// client know that we've send the whole
	// response.
	sock.shutdown(boost::asio::socket_base::shutdown_send);
}


int main()
{
	std::uint16_t port_num = 3333;

	try
	{
		boost::asio::ip::tcp::endpoint ep(
			boost::asio::ip::address_v4::any(),
			port_num
		);

		boost::asio::io_context ioc;

		boost::asio::ip::tcp::acceptor acceptor(ioc, ep);

		boost::asio::ip::tcp::socket sock(ioc);

		acceptor.accept(sock);

		processRequest(sock);
	}
	catch(boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
