#include <boost/asio.hpp>
#include <iostream>
#include <memory>

// Keeps objects we need in a callback.
struct Session
{
	std::shared_ptr<boost::asio::ip::tcp::socket> sock;
	std::string buf;
};

// Function used as a callback for
// asynchronous writing operation.
void callback(
	const boost::system::error_code& ec,
	std::size_t bytes_transferred,
	std::shared_ptr<Session> s
)
{
	if (ec)
	{
		std::cerr << "Error occured! Error code = "
		<< ec.value()
		<< ". Message: " << ec.message()
		<< '\n';
	}
	
	// Here we know that all the data has
	// been written to the socket.
}

void writeToSocket(
	std::shared_ptr<boost::asio::ip::tcp::socket> sock
)
{
	auto s = std::make_shared<Session>();

	// Allocating and filling the buffer.
	s->buf = "Hello\n";
	s->sock = std::move(sock);

	std::string_view  buf = s->buf;
	auto sck_raw_ptr = s->sock.get();

	// Initiating asynchronous write operation.
	sck_raw_ptr->async_write_some(
		boost::asio::buffer(buf),
		[
			s=std::move(s),
			&fn=callback
		](auto ec, auto bt)
		{
			fn(ec, bt, std::move(s));
		}
	);
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

		// Allocating, opening and connecting a socket.
		auto sock = std::make_shared<boost::asio::ip::tcp::socket>(
			ioc,
			ep.protocol()
		);

		sock->connect(ep);

		writeToSocket(std::move(sock));

		ioc.run();
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
