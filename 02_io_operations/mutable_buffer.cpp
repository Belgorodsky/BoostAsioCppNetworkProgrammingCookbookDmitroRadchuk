#include <boost/asio.hpp>
#include <iostream>
#include <memory> // For std::unique_ptr
#include <cinttypes>
#include <string>
#include <algorithm>

int main()
{
	// We expect to receive a block of data no more than 20 bytes
	// long
	constexpr size_t BUF_SIZE_BYTES = 20;

	// Step 1. Allocating the buffer.
	auto buf = std::make_unique<uint8_t[]>(BUF_SIZE_BYTES);

	// Step 2. Creating buffer representation that stisfies
	// MutableBufferSrquence concept requirements.
	boost::asio::mutable_buffer intput_buffer = 
		boost::asio::buffer(static_cast<void*>(buf.get()),
		BUF_SIZE_BYTES
	);

	// Step 3. 'input_buf' is the representation of the buffer
	// 'buf' that can be used in Boost.Asio intput operations.

	std::string_view sv = "World";
	std::copy(sv.begin(), sv.end(), boost::asio::buffers_begin(intput_buffer));

	std::copy(
		boost::asio::buffers_begin(intput_buffer),
		boost::asio::buffers_end(intput_buffer), 
		std::ostream_iterator<char>(std::cout, "")
	);
	std::cout << '\n'; 
}
