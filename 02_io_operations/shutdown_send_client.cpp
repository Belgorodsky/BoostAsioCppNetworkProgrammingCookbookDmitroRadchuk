#include <boost/asio.hpp>
#include <boost/current_function.hpp>
#include <iostream>

void communicate(boost::asio::ip::tcp::socket &sock)
{
	// Allocating and filling the buffer with
	// binary data.
	constexpr std::array<char,6> request_buf{ 0x48, 0x65, 0x0, 0x6c, 0x6c, 0x6f };

	// Sending the request data.
	boost::asio::write(sock, boost::asio::buffer(request_buf));

	// Shutting down the socket to let the
	// server know that we've send the whole
	// request.
	sock.shutdown(boost::asio::socket_base::shutdown_send);

	// We use extensible buffer for resonse
	// because we don't lnow the size of the
	// response message.
	boost::asio::streambuf response_buf;

	boost::system::error_code ec;
	boost::asio::read(sock, response_buf, ec);

	if (ec == boost::asio::error::eof)
	{
		// Whole response message has been received.
		// Here we can handle it.
		std::cout << BOOST_CURRENT_FUNCTION << " Message: \n";
		std::istream is(&response_buf);
		for (std::string line; std::getline(is, line); )
		{
			std::cout << line << '\n';
		}
		std::cout << "\nEnd of message\n";
	}
	else
	{
		throw boost::system::system_error(ec);
	}
}

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

		communicate(sock);
	}
	catch (boost::system::system_error& e)
	{
		std::cerr << "Error occured! Error code =" << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
