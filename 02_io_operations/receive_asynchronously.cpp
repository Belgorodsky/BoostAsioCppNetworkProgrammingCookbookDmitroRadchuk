#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <array>

// Step 1.
// Keeps objects we need in a callback to
// indentify whether all data has been read
// from the socket and to initiate next async
// reading operation if needed
template<std::size_t buf_size>
struct Session
{
	std::shared_ptr<boost::asio::ip::tcp::socket> sock;
	std::array<std::uint8_t, buf_size> buf = { 0 };
	std::size_t total_bytes_read = 0;
};

// Step 2.
// Function used as a callback for
// asynchronous reading operation.
// Checks if all data has been read
// from the socket and initiates
// new reading operation if needed.
template<std::size_t buf_size>
void callback(
	const boost::system::error_code &ec,
	std::size_t bytes_transferred,
	std::shared_ptr<Session<buf_size>> s
)
{
	if (!ec && s)
	{
		s->total_bytes_read += bytes_transferred;

		auto buf_raw_ptr = s->buf.data();
		auto sck_raw_ptr = s->sock.get();
		auto offset = s->total_bytes_read;

		if (offset == buf_size)
		{
			std::string_view buf_view(
				reinterpret_cast<const char*>(buf_raw_ptr),
				buf_size
			);
			std::cout << "Read operation complete! Buffer as array of chars: '"
			<< buf_view << "'\n";
			return;
		}

		sck_raw_ptr->async_receive(
			boost::asio::buffer(
				buf_raw_ptr + 
				offset,
				buf_size - 
				offset
			),
			boost::asio::socket_base::message_do_not_route,
			[
				fn=callback<buf_size>,
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

		return;
	}

	std::cerr << "Error occured! Error code = "
	<< ec.value()
	<< ". Message: " << ec.message()
	<< '\n';
}

void readFromSocket(
	std::shared_ptr<boost::asio::ip::tcp::socket> sock
)
{
	constexpr std::size_t MESSAGE_SIZE = 7;

	// Step 4. Allocating the buffer. 
	auto s = std::make_shared<Session<MESSAGE_SIZE>>();

	s->sock = std::move(sock);
	auto buf_raw_ptr = s->buf.data();
	auto sck_raw_ptr = s->sock.get();

	// Step 5. Initiating asynchronous reading operation.
	sck_raw_ptr->async_receive(
		boost::asio::buffer(buf_raw_ptr, MESSAGE_SIZE),
		boost::asio::socket_base::message_do_not_route,
		[
			fn=callback<MESSAGE_SIZE>,
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

		// Step 3. Allocating , opening and connecting a socket.
		auto sock = std::make_shared<boost::asio::ip::tcp::socket>(
			ioc,
			ep.protocol()
		);

		sock->connect(ep);

		readFromSocket(std::move(sock));

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

