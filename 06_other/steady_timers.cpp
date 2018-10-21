#include <boost/asio.hpp>
#include <iostream>

int main()
{
	using namespace std::chrono_literals;
	boost::asio::io_context ioc;
	boost::asio::steady_timer t1(ioc);
	t1.expires_from_now(2s);
	boost::asio::steady_timer t2(ioc);
	t2.expires_from_now(5s);

	t1.async_wait([&t2](auto ec)
	{
		if (!ec)
		{
			std::cout << "Timer #1 has expired!\n";
		}
		else if (boost::asio::error::operation_aborted == ec)
		{
			std::cout << "Timer #1 has been cancelled!\n";
		}
		else
		{
			std::cerr << "Error occured! Error code = "
			          << ec
					  << ". Message: "
					  << ec.message()
					  << '\n';
		}
		t2.cancel();
	});

	t2.async_wait([](auto ec)
	{
		if (!ec)
		{
			std::cout << "Timer #2 has expired!\n";
		}
		else if (boost::asio::error::operation_aborted == ec)
		{
			std::cout << "Timer #2 has been cancelled!\n";
		}
		else
		{
			std::cerr << "Error occured! Error code = "
			          << ec
					  << ". Message: "
					  << ec.message()
					  << '\n';
		}
	});

	ioc.run();

	return 0;
}
