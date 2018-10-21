#include <boost/asio.hpp>
#include <iostream>

int main()
{
	// Step 1 and 2. Create and fill simple buffers.
	std::string_view part1 = "Hello ";
	std::string_view part2 = "my ";
	std::string_view part3 = "friend!";

	// Step 3 and 4. Create and fill an object representing a composite buffer.
	std::array composite_buffer{
		boost::asio::const_buffer(part1.data(), part1.length()),
		boost::asio::const_buffer(part2.data(), part2.length()),
		boost::asio::const_buffer(part3.data(), part3.length()),
	};

	// Step 5. Now composite_buffer can be used with Boost.Asio
	// output operation as if it was a simple buffer represented
	// by contiguous block of memory.

	std::copy(
		boost::asio::buffers_begin(composite_buffer),
		boost::asio::buffers_end(composite_buffer), 
		std::ostream_iterator<char>(std::cout, "")
	);
	std::cout << '\n'; 
}
