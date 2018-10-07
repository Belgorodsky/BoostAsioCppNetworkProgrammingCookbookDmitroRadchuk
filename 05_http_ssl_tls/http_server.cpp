#include <boost/asio.hpp>

#include <filesystem>
#include <fstream>
#include <atomic>
#include <thread>
#include <iostream>
#include <array>
#include <charconv>

class Service
{
	public:
		void static start_handling(
			std::string_view resource_root,
			std::unique_ptr<boost::asio::ip::tcp::socket> sock_uptr
		)
		{
			auto service = std::unique_ptr<Service>(
				new Service(
					resource_root,
					std::move(sock_uptr)
				)
			);
			auto &&sock = *service->m_sock;
			auto &&request = service->m_request;
			boost::asio::async_read_until(
				sock,
				request,
				"\r\n",
				[svc=std::move(service)](auto &&ec, auto &&bt) mutable
				{
					Service::on_request_received(
						std::move(svc),
						std::forward<decltype(ec)>(ec),
						std::forward<decltype(bt)>(bt)
					);
				}
			);
		}

	private:
		Service(
			std::string_view resource_root,
			std::unique_ptr<boost::asio::ip::tcp::socket> sock
		) : 
		m_resource_root(resource_root),
		m_sock(std::move(sock))
		{}

		void static on_request_received(
			std::unique_ptr<Service> &&service,
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (!ec)
			{
				// Parse the request line
				std::string request_line;
				std::istream request_stream(&service->m_request);
				std::getline(request_stream, request_line, '\r');
				// Remove symbol '\n' from the buffer.
				request_stream.get();

				// Parse the request line.
				std::string request_method;
				std::istringstream request_line_stream(request_line);
				request_line_stream >> request_method;
				// We only support GET method
				if ("GET" != request_method)
				{
					// Unsupported method.
					service->m_response_status_code = 501;
					send_response(std::move(service));
					return;
				}

				request_line_stream >> service->m_requested_resource;

				std::string request_http_version;
				request_line_stream >> request_http_version;

				if ("HTTP/1.1" != request_http_version)
				{
					// Unsupported HTTP version or bad request.
					service->m_response_status_code = 505;
					send_response(std::move(service));
					return;
				}

				// At this point the request line is successfully
				// received and parsed. Now read the request headers.
				auto &&sock = *service->m_sock;
				auto &&request = service->m_request;
				boost::asio::async_read_until(
					sock,
					request,
					"\r\n\r\n",
					[svc=std::move(service)](auto &&ec, auto &&bt) mutable
					{
						Service::on_headers_received(
							std::move(svc),
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

			if (ec == boost::asio::error::not_found)
			{
				// No delimiter has been found in the
				// request message.

				service->m_response_status_code = 413;
				send_response(std::move(service));
				return;
			}
		}

		void static on_headers_received(
			std::unique_ptr<Service> service,
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (!ec)
			{
				// Parse and store headers.
				std::istream request_stream(&service->m_request);
				std::string header_name, header_value;

				while (std::getline(request_stream, header_name, ':'))
				{
					if (std::getline(request_stream, header_value,'\r'))
					{
						// Remove symbol '\n' from the stream.
						request_stream.get();
						service->m_request_headers.emplace(
							std::move(header_name),
							std::move(header_value)
						);
					}
				}
				// Now we have all we need to process the request.
				service->process_request();
				send_response(std::move(service));
				return;
			}

			std::cerr << "Error occured! Error code = "
			<< ec
			<< '\n';

			if (ec == boost::asio::error::not_found)
			{
				// No delimiter has been found in the
				// request message.

				service->m_response_status_code = 413;
				send_response(std::move(service));
				return;
			}
		}

		void process_request()
		{
			// Read file.
			std::string resource_file_path =
				m_resource_root + m_requested_resource;

			std::error_code ec;
			if (!std::filesystem::is_regular_file(resource_file_path, ec))
			{
				resource_file_path.clear();
				if (m_requested_resource.find_first_not_of("/") == std::string::npos)
				{
					for (auto &de : std::filesystem::directory_iterator(m_resource_root))
					{
						if (de.is_regular_file(ec))
						{
							auto &&path = de.path().generic_string();
							auto &&filename = de.path().filename().generic_string();
							if (!filename.find("index"))
								resource_file_path = path;
						}
					}
				}

				if (!std::filesystem::exists(resource_file_path, ec))
				{
					// Resource not found.
					m_response_status_code = 404;
					return;
				}
			}

			std::ifstream resource_fstream(
				resource_file_path,
				std::ifstream::binary
			);

			if (!resource_fstream.is_open())
			{
				// Could not open file.
				// Something bad happened.
				m_response_status_code = 500;
				return;
			}

			// Find out file size.
			resource_fstream.seekg(0, std::ifstream::end);
			m_resource_buffer.resize(
				static_cast<std::size_t>(
					resource_fstream.tellg()
				)
			);

			resource_fstream.seekg(std::ifstream::beg);
			resource_fstream.read(m_resource_buffer.data(), m_resource_buffer.size());
			std::string buffer(5,'\0'); 
			if (auto[p, ec] = std::to_chars(
				buffer.data(), 
				buffer.data() + buffer.size(), 
				m_resource_buffer.size());
				ec == std::errc()
			)
			{
				buffer.resize(p - buffer.data());
			}
			else
			{
				// Could not open file.
				// Something bad happened.
				m_response_status_code = 500;
			}
			m_response_headers = "content-length: " + buffer + "\r\n\r\n";
		}

		void static send_response(
			std::unique_ptr<Service> service
		)
		{
			service->m_sock->shutdown(
				boost::asio::ip::tcp::socket::shutdown_receive
			);

			auto status_line_sv = http_status_table.at(
				service->m_response_status_code
			);
			service->m_response_status_line = "HTTP/1.1 ";
			service->m_response_status_line.append(status_line_sv);
			service->m_response_status_line.append(" \r\n");

			std::vector response_buffers = {
				boost::asio::buffer(service->m_response_status_line),
				boost::asio::buffer(service->m_response_headers)
			};
			response_buffers.reserve(3);

			if (service->m_resource_buffer.size())
				response_buffers.push_back(
					boost::asio::buffer(service->m_resource_buffer)
				);

			// Initiate asynchronous write operation.
			auto &&sock = *service->m_sock;
			boost::asio::async_write(
				sock,
				response_buffers,
				[svc=std::move(service)](auto &&ec, auto &&bt) mutable
				{
					svc->on_headers_received(
						std::forward<decltype(ec)>(ec),
						std::forward<decltype(bt)>(bt)
					);
				}
			);
		}

		void on_headers_received(
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (ec)
			{
				std::cerr << "Error occured! Error code = "
				<< ec
				<< '\n';
			}
		}
	private:
		std::string m_resource_root;
		std::unique_ptr<boost::asio::ip::tcp::socket> m_sock;
		boost::asio::streambuf m_request;
		std::map<std::string,std::string> m_request_headers;
		std::string m_requested_resource;
		
		std::vector<char> m_resource_buffer;
		std::uint16_t m_response_status_code = 200;
		std::string m_response_headers = "\r\n\r\n";
		std::string m_response_status_line;

		static const inline std::map<std::size_t, std::string_view> http_status_table =
		{
			{200, "200 OK"},
			{404, "404 Not Found"},
			{413, "413 Request Entity Too Large"},
			{500, "500 Server Error"},
			{501, "501 Not Implemented"},
			{505, "505 HTTP Version Not Supported"},
		};
};

class Acceptor
{
	public:
		Acceptor(
			std::string_view resources_root_path,
			boost::asio::io_context &ioc,
			std::uint16_t port_num
		) :
		m_resources_root_path(resources_root_path),
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
				Service::start_handling(
					m_resources_root_path,
					std::move(sock)
				);

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
		std::string m_resources_root_path;
		boost::asio::io_context &m_ioc;
		boost::asio::ip::tcp::acceptor m_acceptor;
		std::atomic<bool> m_isStopped{false};
};

class Server
{
	public:
		// Start the server.
		void start(
			std::string_view root_path,
			std::uint16_t port_num,
			std::size_t thread_pool_size
		)
		{
			assert(std::filesystem::is_directory(root_path));
			assert(0 < thread_pool_size);

			// Create and start Acceptor.
			m_acc = std::make_unique<Acceptor>(root_path, m_ioc, port_num);
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
int main(int argc, char *argv[])
{
	std::string_view root_dir = 1 < argc ? argv[1] : "/var/www/html/";

	std::uint16_t port_num = 80;

	try
	{
		Server srv;
		std::size_t thread_pool_size = std::thread::hardware_concurrency();
		if (!thread_pool_size) thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
		srv.start(root_dir, port_num, thread_pool_size);
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
