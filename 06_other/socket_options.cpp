#include <boost/asio.hpp>
#include <iostream>

int main()
{
	try
	{
		boost::asio::io_context ioc;

		// Create and open a TCP socket.
		boost::asio::ip::tcp::socket sock(ioc, boost::asio::ip::tcp::v4());

		// Create an object representing receive buffer
		// size option.
		boost::asio::socket_base::receive_buffer_size cur_buf_size;

		// Get the currently set value of the option.
		sock.get_option(cur_buf_size);

		std::cout << "Current receive buffer size is "
		          << cur_buf_size.value() << " bytes.\n";

		// Create an object representing receive buffer
		// size option with new value.
		boost::asio::socket_base::receive_buffer_size new_buf_size(
			cur_buf_size.value() * 2
		);

		// Set new value of the option.
		sock.set_option(new_buf_size);

		sock.get_option(cur_buf_size);
		std::cout << "New receive buffer size is "
		<< cur_buf_size.value() << " bytes.\n";
	}
	catch(boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
