#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include <charconv>

class Service
{
	public:
		Service() = default;

		void handleClient(
			boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &ssl_stream)
		{
			try
			{
				// Blocks until the handshake completes.
				ssl_stream.handshake(
					boost::asio::ssl::stream_base::server
				);

				boost::asio::streambuf request_buf;
				boost::asio::read_until(ssl_stream, request_buf, '\n');
				std::istream is(&request_buf);
				std::string request_str;
				std::getline(is, request_str);
				std::string_view request = request_str;

				// Emulate request processing.
				std::string_view op = "EMULATE_LONG_COMP_OP ";
				auto pos = request.find(op);
				int sec_count = 0;
				bool error_occured = pos == std::string_view::npos;
				if (!error_occured)
				{
					auto sec_str_v = request.substr(pos + op.length()); 
					if (
							auto [ptr, ec] = std::from_chars(sec_str_v.data(), sec_str_v.data()+sec_str_v.length(), sec_count);
							!std::make_error_code(ec)
					   )
						std::this_thread::sleep_for(std::chrono::seconds(sec_count));
					else
						error_occured = true;
				}

				std::string_view response = error_occured ? "ERROR\n" : "OK\n";

				// Sending response.
				boost::asio::write(ssl_stream,boost::asio::buffer(response));
			}
			catch (boost::system::system_error &e)
			{
				std::cerr << "Error occured! Error code = "
				<< e.code() << e.what() << '\n';
			}
		}
};

class Acceptor
{
	public:
		Acceptor(
			boost::asio::io_context &ioc,
			std::uint16_t port_num
		) :
		m_ioc(ioc),
		m_acceptor(
			m_ioc,
			boost::asio::ip::tcp::endpoint(
				boost::asio::ip::address_v4::any(),
				port_num
			)
		),
		m_ssl_context(boost::asio::ssl::context::sslv23_server)
		{
			// Setting up the context.
			m_ssl_context.set_options(
				boost::asio::ssl::context::default_workarounds |
				boost::asio::ssl::context::no_sslv2 |
				boost::asio::ssl::context::single_dh_use
			);
			
			m_ssl_context.set_password_callback(
				[this](auto ml, auto purp)
				{
					return get_password(ml,purp);
				}
			);

			m_ssl_context.use_certificate_chain_file("user.crt");
			m_ssl_context.use_private_key_file(
				"user.key",
				boost::asio::ssl::context::pem
			);
			m_ssl_context.use_tmp_dh_file("dh2048.pem");

			m_acceptor.listen();
		}

		void accept()
		{
			boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
				ssl_stream(m_ioc, m_ssl_context);

			m_acceptor.accept(ssl_stream.lowest_layer());

			Service svc;
			svc.handleClient(ssl_stream);
		}
	private:
		std::string get_password(
			std::size_t max_length,
			boost::asio::ssl::context::password_purpose purpose
		) const
		{
			return "pass";
		}

	private:
		boost::asio::io_context &m_ioc;
		boost::asio::ip::tcp::acceptor m_acceptor;

		boost::asio::ssl::context m_ssl_context;
};

class Server
{
	public:

		~Server()
		{ 
			if (m_thread.joinable())
				m_thread.join();
		}

		void start(std::uint16_t port_num)
		{
			m_thread = std::thread(&Server::run, this , port_num);
		}

		void stop()
		{
			m_stop = true;
			m_thread.join();
		}
	private:
		void run(std::uint16_t port_num)
		{
			Acceptor acc(m_ioc, port_num);

			while (!m_stop)
			{
				acc.accept();
			}
		}
	private:
		std::thread m_thread;
		std::atomic<bool> m_stop{false};
		boost::asio::io_context m_ioc;
};

// Run tcp_asynchronous client ssl_synchronous_client 
// to test this example
int main()
{
	std::uint16_t port_num = 3333;

	try
	{
		Server srv;
		srv.start(port_num);

		std::cin.get();
		srv.stop();
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = "
		<< e.code() << '\n';
	}

	return 0;
}
