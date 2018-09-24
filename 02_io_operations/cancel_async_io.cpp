#include <boost/asio.hpp>
#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0501

#if _WIN32_WINNT <= 0x0502 // Windows Server 2003 or earlier.
#define BOOST_ASIO_DISABLE_IOCP
#define BOOST_ASIO_ENABLE_CANCELIO
#endif
#endif

#include <boost/asio.hpp>
#include <iostream>
#include <thread>

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

		auto sock = std::make_shared<boost::asio::ip::tcp::socket>(ioc, ep.protocol());

		sock->async_connect(
			ep,
			[s=sock](auto &&ec)
			{
				// If asynchronous operation has been
				// cancelled or an error occured during
				// execution, ec contains corresponding
				// error code.
				if (ec)
				{
					if (ec == boost::asio::error::operation_aborted)
					{
						std::cout << "Operation cancelled!\n";
					}
					else
					{
						std::cerr << "Error occured!"
						<< " Error code = "
						<< ec.value()
						<< ". Message: "
						<< ec.message();
					}
					return;
				}

			}

			// At this point the socket is connected and
			// can be used for communication with
			// remote application.
		);

		std::thread worker_thread([&ioc]()
			{
				try
				{
					ioc.run();
				}
				catch (boost::system::system_error &e)
				{
					std::cerr << "Error occured!"
					<< " Error code = " << e.code()
					<< ". Message: " << e.what() << '\n';
				}
			}
		);

		// Cancelling the initiated operation.
		sock->cancel();

		// Waiting for the worker thread to complete.
		worker_thread.join();
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
