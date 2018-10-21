#include <boost/asio.hpp>
#include <iostream>

// Run 'echo "" | awk '{  print "Hello\n" }' | netcat -l 127.0.0.1 -p 3333' before start
int main()
{
	boost::asio::ip::tcp::iostream stream("localhost", "3333");
	if (!stream)
	{
		std::cerr << "Error occured! Error code = "
		          << stream.error()
				  << '\n';
		return -1;
	}

	stream << "Request.";
	stream.flush();

	using namespace std::chrono_literals;
	stream.expires_after(2s);
	std::cout << "Response: " << stream.rdbuf() << '\n';

	return 0;
}
