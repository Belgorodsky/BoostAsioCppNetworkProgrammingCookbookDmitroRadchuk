#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <string>
#include <charconv>

class SyncSSLClient
{
	public:
		SyncSSLClient(
			std::string_view &raw_ip_address,
			std::uint16_t port_num
		) : 
			m_ep(boost::asio::ip::make_address(raw_ip_address),port_num),
			m_ssl_context(boost::asio::ssl::context::sslv3_client),
			m_ssl_stream(m_ioc, m_ssl_context)
		{
			// Set verification mode and designate that
			// we want to perform verification.
			m_ssl_stream.set_verify_mode(boost::asio::ssl::verify_peer);

			// Set verification callback.
			m_ssl_stream.set_verify_callback(
				[this](bool preverified, auto &&context)
				{
					return on_peer_verify(preverified, context);
				}
			);
		}

		void connect()
		{
			// Connect the TCP socket.
			m_ssl_stream.lowest_layer().connect(m_ep);

			// Perform the SSL handshake.
			m_ssl_stream.handshake(boost::asio::ssl::stream_base::client);
		}

		void close()
		{
			// We ignore any erors that might occur
			// during shutdown as we anyway can't
			// do anything about them.
			boost::system::error_code ec;
			
			m_ssl_stream.shutdown(ec); // Shutdown SSL.
			
			// Shutdown the socket
			m_ssl_stream.lowest_layer().shutdown(
				boost::asio::ip::tcp::socket::shutdown_both,
				ec
			);
			m_ssl_stream.lowest_layer().close();
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
		bool on_peer_verify(
			bool preverified,
			boost::asio::ssl::verify_context& context
		)
		{
			// Here the certificate should be verified and the 
			// verification result should be returned.
			return true;
		}
		void sendRequest(std::string_view request)
		{
			boost::asio::write(m_ssl_stream, boost::asio::buffer(request));
		}

		std::string receiveResponse()
		{
			boost::asio::streambuf buf;
			boost::asio::read_until(m_ssl_stream, buf, '\n');
			std::istream input(&buf);

			std::string response;
			std::getline(input, response);

			return response;
		}

	private:
		inline static constexpr char m_op_name[] = "EMULATE_LONG_COMP_OP ";
		boost::asio::io_context m_ioc;

		boost::asio::ip::tcp::endpoint m_ep;
		boost::asio::ssl::context m_ssl_context;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_ssl_stream;
};

int main()
{
	std::string_view raw_ip_address = "127.0.0.1";
	constexpr std::uint16_t port_num = 3333;

	try
	{
		SyncSSLClient client(raw_ip_address, port_num);

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
