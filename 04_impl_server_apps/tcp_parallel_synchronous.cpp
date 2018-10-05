#include <boost/asio.hpp>

#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include <charconv>

class Service
{
	public:
		std::unique_ptr<Service> static create()
		{
			return std::unique_ptr<Service>(new Service);
		}
		
		void static startHandlingClient(
			std::unique_ptr<boost::asio::ip::tcp::socket> sock
		)
		{
			std::thread(
				&Service::handleClient,
				std::move(Service::create()),
				std::move(sock)
			).detach();
		}
	private:
		Service() = default;

		void handleClient(
			std::unique_ptr<boost::asio::ip::tcp::socket> sock
		)
		{
			try
			{
				boost::asio::streambuf request_buf;
				boost::asio::read_until(*sock, request_buf, '\n');
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
							auto [ptr, ec] = std::from_chars(
								sec_str_v.data(), 
								sec_str_v.data()+sec_str_v.length(), 
								sec_count
							);
							!std::make_error_code(ec)
					   )
						std::this_thread::sleep_for(std::chrono::seconds(sec_count));
					else
						error_occured = true;
				}

				std::string_view response = error_occured ? "ERROR\n" : "OK\n";

				// Sending response.
				boost::asio::write(*sock,boost::asio::buffer(response));
			}
			catch (boost::system::system_error &e)
			{
				std::cerr << "Error occured! Error code = "
				<< e.code() << '\n';
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
		)
		{
			m_acceptor.listen();
		}

		void accept()
		{
			auto sock = std::make_unique<boost::asio::ip::tcp::socket>(m_ioc);

			m_acceptor.accept(*sock);

			Service::startHandlingClient(std::move(sock));
		}

	private:
		boost::asio::io_context &m_ioc;
		boost::asio::ip::tcp::acceptor m_acceptor;
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

// Run tcp_asynchronous client from 03_impl_client_apps
// to test this example
int main()
{
	std::uint16_t port_num = 3335;

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
