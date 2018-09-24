#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <array>

// Step 1.
// Keeps objects we need in a callback 
struct Session
{
	std::shared_ptr<boost::asio::ip::tcp::socket> sock;
	std::unique_ptr<boost::asio::streambuf> buf;
};

// Step 2.
// Function used as a callback for
// asynchronous reading operation.
void callback(
	const boost::system::error_code &ec,
	std::size_t bytes_transferred,
	std::shared_ptr<Session> s
)
{
	if (!ec && s)
	{
		std::istream is(s->buf.get());
		std::string data; 
		std::getline(is, data);

		std::cout << "Read operation complete! Buffer as array of chars: '"
		<< data << "'\n";

		return;
	}

	std::cerr << "Error occured! Error code = "
	<< ec.value()
	<< ". Message: " << ec.message()
	<< '\n';
}

void readFromSocketDelim(
	std::shared_ptr<boost::asio::ip::tcp::socket> sock
)
{
	// Step 4. Allocating the buffer. 
	auto s = std::make_shared<Session>();

	s->sock = std::move(sock);
	s->buf = std::make_unique<boost::asio::streambuf>();

	auto sck_raw_ptr = s->sock.get();
	auto buf_raw_ptr = s->buf.get();

	// Step 5. Initiating asynchronous reading operation.
	boost::asio::async_read_until(
		*sck_raw_ptr,
		*buf_raw_ptr,
		'\n',
		[
			fn=callback,
			s=std::move(s)
		](auto &&ec, auto &&bt)
		{
			fn(
				std::forward<decltype(ec)>(ec),
				std::forward<decltype(bt)>(bt),
				std::move(s)
			);
		}
	);
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

		// Step 3. Allocating , opening and connecting a socket.
		auto sock = std::make_shared<boost::asio::ip::tcp::socket>(
			ioc,
			ep.protocol()
		);

		sock->connect(ep);

		readFromSocketDelim(std::move(sock));

		// Step 6.
		ioc.run();
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what()
		<< '\n';

		return e.code().value();
	}

	return 0;
}

