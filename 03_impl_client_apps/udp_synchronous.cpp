#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <array>
#include <charconv>
#include <chrono>

class SyncUDPClient
{
	public:
		SyncUDPClient() : 
			m_sock(m_ioc)
		{
			m_sock.open(boost::asio::ip::udp::v4());
		}

		template< class Rep, class Period >
		std::string emulateLongComputationOp(
			const std::chrono::duration<Rep, Period>& duration,
			std::string_view raw_ip_address,
			std::uint16_t port_num
		)
		{
			std::string request;
			request.reserve(42);
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
			std::array<char, 21u> buffer = { 0 }; // 20 is str length of int64_max with sign and 1 for zero termination
			if (auto[p, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), seconds); ec == std::errc())
			{
				request = m_op_name;
				request.append(buffer.data(), p - buffer.data());
				request.push_back('\n');
			}

			boost::asio::ip::udp::endpoint ep(
				boost::asio::ip::make_address(raw_ip_address),
				port_num
			);

			sendRequest(ep, request);
			return receiveResponse(ep);
		}

	private:
		void sendRequest(
			const boost::asio::ip::udp::endpoint &ep,
			std::string_view request
		)
		{
			m_sock.send_to(
				boost::asio::buffer(request),
				ep
			);
		}

		std::string receiveResponse(boost::asio::ip::udp::endpoint& ep)
		{
			std::string response(6, '\0');

			std::size_t bytes_received = m_sock.receive_from(
				boost::asio::buffer(response),
				ep
			);

			// trying to send shutdown without exception
			boost::system::error_code ec;
			m_sock.shutdown(boost::asio::ip::udp::socket::shutdown_receive, ec);
			if (ec.value() && ec.value() != boost::asio::error::basic_errors::not_connected)
			{
				std::cerr << "Error occured while shutdown. Code "
					<< ec.value() << '\n';
			}
			m_sock.close();
			m_sock.open(boost::asio::ip::udp::v4());
			response.resize(bytes_received);
			return response;
		}

	private:
		inline static constexpr char m_op_name[] = "EMULATE_LONG_COMP_OP ";
		boost::asio::io_context m_ioc;

		boost::asio::ip::udp::socket m_sock;
};

// Run 'echo "" | awk '{  print "OK\n" }' | netcat -u -l 127.0.0.1 -p 3333 & echo "" | awk '{  print "OK\n" }' | netcat -u -l 127.0.0.1 -p 3334 & sleep 15 && for pid in $(pgrep -P $$); do kill -2 $pid; done;'
// And you have 15 seconds to run this example program
int main()
{
	std::string_view server1_raw_ip_address = "127.0.0.1";
	constexpr std::uint16_t server1_port_num = 3333;

	std::string_view server2_raw_ip_address = "127.0.0.1";
	constexpr std::uint16_t server2_port_num = 3334;

	try
	{
		using namespace std::chrono_literals; // to write '10s' as ten seconds 
		SyncUDPClient client;

		std::cout << "Sending request to the server #1 ... \n";

		std::string response = client.emulateLongComputationOp(
			10s,
			server1_raw_ip_address,
			server1_port_num
		);

		std::cout << "Response from the server #1 received: "
		<< response << '\n';

		std::cout << "Sending request to the server #2 ... \n";

		response = client.emulateLongComputationOp(
			10s,
			server2_raw_ip_address,
			server2_port_num
		);

		std::cout << "Response from the server #2 received: "
		<< response << '\n';
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = " << e.code()
		<< ". Message: " << e.what() << '\n';

		return e.code().value();
	}

	return 0;
}
