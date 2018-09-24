#include <boost/asio.hpp>
#include <iostream>
#include <memory>

// Step 1.
// Keeps objects we need in a callback to
// identify whether all data has been written
// to the socket and to initiate next async
// writing operation if needed.
struct Session
{
	std::shared_ptr<boost::asio::ip::tcp::socket> sock;
	std::string buf;
	std::size_t total_bytes_written;
};

// Step 2.
// Function used as a callback for
// asynchronous writing operation.
// Checks if all data from the buffer has
// been written to the socket and initiates
// new asynchronous writing operation if needed.
void callback(
	const boost::system::error_code& ec,
	std::size_t bytes_transferred,
	std::shared_ptr<Session> s
)
{
	if (!ec && s)
	{
		s->total_bytes_written += bytes_transferred;

		if (s->total_bytes_written == s->buf.length())
		{
			return;
		}
		
		auto buf_begin = 
				s->buf.c_str() +
				s->total_bytes_written;
		auto buf_size = s->buf.length() -
				s->total_bytes_written;
		auto sck_raw_ptr = s->sock.get();

		sck_raw_ptr->async_write_some(
			boost::asio::buffer(
				buf_begin,
				buf_size
			),
			[
				s=std::move(s),
				&fn=callback
			](auto ec, auto bt)
			{
				fn(ec, bt, std::move(s));
			}
		);

		return;
	}

	std::cerr << "Error occured! Error code = "
	<< ec.value()
	<< ". Message: " << ec.message()
	<< '\n';
}

void writeToSocket(
	std::shared_ptr<boost::asio::ip::tcp::socket> sock
)
{
	auto s = std::make_shared<Session>();

	// Step 4. Allocating and filling the buffer.
	s->buf = "Hello\n";
	s->total_bytes_written = 0;
	s->sock = std::move(sock);

	auto buf_raw_ptr = s->buf.data();
	auto buf_size = s->buf.size();
	auto sck_raw_ptr = s->sock.get();

	// Step 5. Initiating asynchronous write operation.
	sck_raw_ptr->async_write_some(
		boost::asio::buffer(buf_raw_ptr, buf_size),
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

		// Step 3. Allocating, opening and connecting a socket.
		auto sock = std::make_shared<boost::asio::ip::tcp::socket>(
			ioc,
			ep.protocol()
		);

		sock->connect(ep);

		writeToSocket(std::move(sock));

		// Step 6.
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
