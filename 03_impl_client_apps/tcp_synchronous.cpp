#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <charconv>

class SyncTCPClient
{
	public:
		SyncTCPClient(
			std::string_view &raw_ip_address,
			std::uint16_t port_num
		) : 
			m_ep(boost::asio::ip::make_address(raw_ip_address),port_num),
			m_sock(m_ioc)
		{
			m_sock.open(m_ep.protocol());
		}

		void connect()
		{
			m_sock.connect(m_ep);
		}

		void close()
		{
			m_sock.shutdown(
				boost::asio::ip::tcp::socket::shutdown_both
			);
			m_sock.close();
		}

		template< class Rep, class Period >
		std::string emulateLongComputationOp(const std::chrono::duration<Rep, Period>& duration)
		{
			 auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
			std::string request;
			request.reserve(42);
			std::array<char, 21u> buffer = { 0 }; // 20 is str length of int64_max with sign and 1 for zero termination
			if (auto[p, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), seconds); ec == std::errc())
			{
				request = m_op_name;
				request.append(buffer.data(), p - buffer.data());
				request.push_back('\n');
			}

			sendRequest(request);
			return receiveResponse();
		}

	private:
		void sendRequest(std::string_view request)
		{
			boost::asio::write(m_sock, boost::asio::buffer(request));
		}

		std::string receiveResponse()
		{
			boost::asio::streambuf buf;
			boost::asio::read_until(m_sock, buf, '\n');
			std::istream input(&buf);

			std::string response;
			std::getline(input, response);

			return response;
		}

	private:
		inline static constexpr char m_op_name[] = "EMULATE_LONG_COMP_OP ";
		boost::asio::io_context m_ioc;

		boost::asio::ip::tcp::endpoint m_ep;
		boost::asio::ip::tcp::socket m_sock;
};

// Run 'echo "" | awk '{  print "OK\n" }' | netcat -l 127.0.0.1 -p 3333'
// to test this example
int main()
{
	std::string_view raw_ip_address = "127.0.0.1";
	constexpr std::uint16_t port_num = 3333;

	try
	{
		SyncTCPClient client(raw_ip_address, port_num);

		// Sync connect
		client.connect();

		std::cout << "Sending request to the server... \n";

		using namespace std::chrono_literals;
		auto response = client.emulateLongComputationOp(10s);

		std::cout << "Response received: " << response << '\n';

		// Close the connection and free resources.
		client.close();
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';
		return e.code().value();
	}

	return 0;
}
