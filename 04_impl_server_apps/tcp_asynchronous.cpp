#include <boost/asio.hpp>

#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include <charconv>

class Service
{
	public:
		void static startHandling(
			std::unique_ptr<boost::asio::ip::tcp::socket> sock_uptr
		)
		{
			auto service = std::unique_ptr<Service>(new Service(std::move(sock_uptr)));
			auto &&sock = *service->m_sock;
			auto &&request = service->m_request;

			boost::asio::async_read_until(
				sock,
				request,
				'\n',
				[svc=std::move(service)](auto &&ec, auto &&bt) mutable
				{
					Service::onRequestReceived(
						std::move(svc),
						std::forward<decltype(ec)>(ec),
						std::forward<decltype(bt)>(bt)
					);
				}
			);
		}
	private:
		void static onRequestReceived(
			std::unique_ptr<Service> &&service,
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (!ec)
			{
				// Process the request
				service->m_response = processRequest(service->m_request);

				auto sock_raw_ptr = service->m_sock.get();
				auto buf = boost::asio::buffer(service->m_response);

				// Initiate asynchronous write operation.
				boost::asio::async_write(
					*sock_raw_ptr,
					buf,
					[svc=std::move(service)](auto &&ec, auto &&bt) mutable
					{
						Service::onResponseSent(
							std::forward<decltype(ec)>(ec),
							std::forward<decltype(bt)>(bt)
						);
					}
				);

				return;
			}

			std::cerr << "Error occured! Error code = "
			<< ec
			<< '\n';
		}

		void static onResponseSent(
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (!ec)
			{
				std::cerr << "Error occured! Error code = "
				<< ec
				<< '\n';
			}
		}

		std::string_view static processRequest(boost::asio::streambuf &request_buf)
		{
			// In this method we parse the request, process it
			// and prepare the request.
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

			return error_occured ? "ERROR\n" : "OK\n";
		}
	private:
		Service(
			std::unique_ptr<boost::asio::ip::tcp::socket> &&sock
		) : m_sock(std::move(sock))
		{}
	private:
		std::unique_ptr<boost::asio::ip::tcp::socket> m_sock;
		std::string_view m_response;
		boost::asio::streambuf m_request;
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
		{}

		// Start accepting incoming connection requests.
		void start()
		{
			m_acceptor.listen();
			initAccept();
		}

		// Stop accepting incoming connection requests.
		void stop()
		{
			m_isStopped = true;
		}
	private:
		void initAccept()
		{
			auto sock_ptr = std::make_unique<boost::asio::ip::tcp::socket>(m_ioc);
			auto &&sock = *sock_ptr;

			m_acceptor.async_accept(
				sock,
				[this, s=std::move(sock_ptr)](auto &&ec) mutable
				{
					onAccept(std::forward<decltype(ec)>(ec), std::move(s));
				}
			);
		}

		void onAccept(
			const boost::system::error_code &ec,
			std::unique_ptr<boost::asio::ip::tcp::socket> &&sock
		)
		{
			if (!ec)
			{
				Service::startHandling(std::move(sock));

				// Init next async accept operation if
				// acceptor has not been stopped yet.
				if (!m_isStopped)
				{
					initAccept();
					return;
				}
				// Stop accepting incoming connections
				// and free allocated resources.
				m_acceptor.close();
				return;
			}
			
			std::cerr << "Error occured! Error code = "
			<< ec
			<< '\n';
		}
	private:
		boost::asio::io_context &m_ioc;
		boost::asio::ip::tcp::acceptor m_acceptor;
		std::atomic<bool> m_isStopped{false};
};

class Server
{
	public:
		// Start the server.
		void start(std::uint16_t port_num, std::size_t thread_pool_size)
		{
			assert(0 < thread_pool_size);

			// Create and start Acceptor.
			m_acc = std::make_unique<Acceptor>(m_ioc, port_num);
			m_acc->start();

			// Create specified number of threads and
			// add them to the pool.
			for (std::size_t i = 0; i != thread_pool_size; ++i)
			{
				m_thread_pool.emplace_back([&ioc=m_ioc]{ioc.run();});
			}
		}

		// Stop the server.
		void stop()
		{
			m_acc->stop();
			m_ioc.stop();

			for (auto &&th : m_thread_pool)
				if (th.joinable()) th.join();
		}
	private:
		boost::asio::io_context m_ioc;
		using work_guard = 
			boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
		work_guard m_work{boost::asio::make_work_guard(m_ioc)};
		std::unique_ptr<Acceptor> m_acc;
		std::vector<std::thread> m_thread_pool;
};

constexpr std::size_t DEFAULT_THREAD_POOL_SIZE = 2;

// Run tcp_asynchronous client from 03_impl_client_apps
// to test this example
int main()
{
	std::uint16_t port_num = 3334;

	try
	{
		Server srv;
		std::size_t thread_pool_size = std::thread::hardware_concurrency();
		if (!thread_pool_size) thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
		srv.start(port_num, thread_pool_size);
		std::cin.get();
		srv.stop();
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Error occured! Error code = "
		<< e.code()
		<< '\n';
	}

	return 0;
}
