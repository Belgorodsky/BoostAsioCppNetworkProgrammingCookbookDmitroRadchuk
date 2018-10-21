#include <boost/asio.hpp>
#include <iostream>

// Run 'echo "" | awk '{  print "Hello\n" }' | netcat -l 127.0.0.1 -p 3333' before start
int main()
{
	boost::asio::io_context ioc;
	boost::asio::ip::tcp::acceptor acceptor(
		ioc,
		boost::asio::ip::tcp::endpoint(
			boost::asio::ip::tcp::v4(),
			3333
		)
	);

	boost::asio::ip::tcp::iostream stream;
	acceptor.accept(*stream.rdbuf());

	stream << "Response.";
	std::cout << "Request: " << stream.rdbuf() << '\n';

	return 0;
}
