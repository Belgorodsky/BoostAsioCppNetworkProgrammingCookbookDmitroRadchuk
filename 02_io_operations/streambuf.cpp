#include <boost/asio.hpp>
#include <iostream>

int main()
{
	boost::asio::streambuf buf;

	std::ostream output(&buf);

	// Writting the message to the stream-based buffer.
	output << "Message1\nMessage2";

	// Now we want to read all data from a streambuf
	// until '\n' delimiter.
	// Instantiate an input stream which uses our
	// stream buffer.
	std::istream input(&buf);

	// We'll read data into 'message1' string in loop
	
	for (std::string message1; std::getline(input, message1) ; )
	{
		// and output in stdout
		std::cout << message1 << std::endl;
	}

	return 0;
}
