#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <cinttypes>
#include <algorithm>

int main()
{
	std::string_view buf = "Hello"; // Step 1 and Step 2 in single line
	// Step 3. Creating buffer representation that satisfies 
	// ConstBufferSequence concept requirements.
	boost::asio::const_buffer output_buf = boost::asio::buffer(buf);

	// Step 4. 'output_buf' is the representation of the
	// buffer 'buf' that can be used in Boost.Asio output
	// operations.

	std::copy(
		boost::asio::buffers_begin(output_buf),
		boost::asio::buffers_end(output_buf), 
		std::ostream_iterator<char>(std::cout, "")
	);
	std::cout << '\n'; 

	return 0;
}
