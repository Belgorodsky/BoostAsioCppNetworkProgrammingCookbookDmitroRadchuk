#include <iostream>
#include <boost/asio.hpp>

int main()
{
	std::string_view src = "The world is mine!";

	std::string part1(6, '\0');
	std::string part2(6, '\0');
	std::string part3(6, '\0');

	std::array composite_buffer{
		boost::asio::mutable_buffer(part1.data(), part1.length()),
		boost::asio::mutable_buffer(part2.data(), part2.length()),
		boost::asio::mutable_buffer(part3.data(), part3.length()),
	};

	std::copy(
		src.cbegin(),
		src.cend(),
		boost::asio::buffers_begin(composite_buffer)
	);

	std::copy(
		boost::asio::buffers_begin(composite_buffer),
		boost::asio::buffers_end(composite_buffer), 
		std::ostream_iterator<char>(std::cout, "")
	);
	std::cout << '\n'; 
}
