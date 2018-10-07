#include <boost/predef.h> // Tools to identify the OS.

// We need this to enable cancelling of I/O operations on
// Windows XP, Windows Server 2003 and earlier.
// Refer to "https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/reference/basic_stream_socket/cancel/overload1.html"
// for details.
#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0501

#if _WIN32_WINNT <= 0x0502 // Windows Server 2003 or earliner.
	#define BOOST_ASIO_DISABLE_IOCP
	#define BOOST_ASIO_ENABLE_CANCELIO
#endif
#endif

#include <boost/asio.hpp>
#include <boost/current_function.hpp>

#include <thread>
#include <mutex>
#include <memory>
#include <iostream>
#include <charconv>

namespace http_errors
{
	enum http_error_codes
	{
		invalid_response = 1
	};

	class http_errors_category
		: public boost::system::error_category
	{
		public:
			const char* name() const BOOST_SYSTEM_NOEXCEPT
			{
				return "http_errors";
			}

			std::string message(int e) const
			{
				switch(e)
				{
					case invalid_response:
						return "Server response cannot be parsed.";
					default:
						return "Unknown error.";
				}
			}
	};

	const boost::system::error_category&
	get_http_errors_category()
	{
		static http_errors_category cat;
		return cat;
	}

	boost::system::error_code
	make_error_code(http_error_codes e)
	{
		return boost::system::error_code(
			static_cast<int>(e),
			get_http_errors_category()
		);
	}
} // namespace http_errors

namespace boost::system
{
	template <>
	struct is_error_code_enum<http_errors::http_error_codes>
	{
		BOOST_STATIC_CONSTANT(bool, value = true);
	};
} // namespace boost::system

class HTTPClient;
class HTTPRequest;
class HTTPResponse;

using Callback = std::function<
	void (const HTTPRequest&,const HTTPResponse&,const boost::system::error_code&)
>;

class HTTPResponse
{
	friend class HTTPRequest;
	public:
		std::uint16_t get_status_code() const
		{
			return m_status_code;
		}

		const std::string &get_status_message() const
		{
			return m_status_message;
		}

		const std::map<std::string,std::string> &get_headers()
		{
			return m_headers;
		}

		const std::istream &get_response() const
		{
			return m_response_stream;
		}
	private:
		HTTPResponse() = default;
		void add_header(const std::string &name, std::string_view value)
		{
			m_headers[name] = value;
		}

		void add_header(std::string &&name,std::string &&value)
		{
			m_headers.emplace(std::move(name), std::move(value));
		}
	private:
		std::uint16_t m_status_code = 404;	// HTTP status code.
		std::string m_status_message;		// HTTP status message.

		// Response headers.
		std::map<std::string,std::string> m_headers;
		boost::asio::streambuf m_response_buf;
		std::iostream m_response_stream{&m_response_buf};
};

class HTTPRequest
{
	friend class HTTPClient;
	public:
		void set_host(std::string_view host)
		{
			m_host = host;
		}
		void set_port(std::uint16_t port)
		{
			m_port = port;
		}
		void set_uri(std::string_view uri)
		{
			m_uri = uri;
		}
		void set_callback(Callback callback)
		{
			m_callback = std::move(callback);
		}
		std::string_view get_host() const
		{
			return m_host;
		}
		std::uint16_t get_port() const
		{
			return m_port;
		}
		std::string_view get_uri() const
		{
			return m_uri;
		}
		std::size_t get_id() const
		{
			return m_id;
		}
		void execute()
		{
			// Ensure that preconditions hold.
			assert(m_port != 0);
			assert(m_host.length());
			assert(m_uri.length());
			assert(m_callback);
			std::string port_str(5,'\0');
			if (auto [p,er] = std::to_chars(
					port_str.data(),
					port_str.data()+port_str.size(),
					m_port
				);
				er == std::errc()
			)
			{
				port_str.resize(p - port_str.data());
			}
			else
			{
				on_finish(http_errors::invalid_response);
				return;
			}

			
			if (m_was_cancelled)
			{
				on_finish(boost::asio::error::operation_aborted);
				return;
			}

			// Resolve the host name.
			m_resolver.async_resolve(
				m_host,
				port_str,
				boost::asio::ip::tcp::resolver::numeric_service,
				[this](auto &&ec, auto &&it)
				{
					on_host_name_resolved(
						std::forward<decltype(ec)>(ec),
						std::forward<decltype(it)>(it)
					);
				}
			);
		}

		void cancel()
		{
			m_was_cancelled = true;
			m_resolver.cancel();
			if (m_sock.is_open())
			{
				m_sock.cancel();
			}
		}
	private:
		HTTPRequest(
			boost::asio::io_context &ioc,
			std::size_t id
		) :
		m_id(id),
		m_sock(ioc),
		m_resolver(ioc),
		m_ioc(ioc)
		{}

		void on_host_name_resolved(
			const boost::system::error_code &ec,
			boost::asio::ip::tcp::resolver::iterator it
		)
		{
			if (!ec)
			{
				if (m_was_cancelled)
				{
					on_finish(boost::asio::error::operation_aborted);
					return;
				}

				// Connect to the host.
				boost::asio::async_connect(
					m_sock,
					it,
					[this](auto &&ec, auto &&it)
					{
						on_connection_established(
							std::forward<decltype(ec)>(ec),
							std::forward<decltype(it)>(it)
						);
					}
				);

				return;
			}
			on_finish(ec);
		}

		void on_connection_established(
			const boost::system::error_code &ec,
			boost::asio::ip::tcp::resolver::iterator it
		)
		{
			if (!ec)
			{
				m_request_buf = 
					"GET " + m_uri + " HTTP/1.1\r\n" +
					"HOST: " + m_host + "\r\n" +
					"\r\n";
				
				if (m_was_cancelled)
				{
					on_finish(boost::asio::error::operation_aborted);
					return;
				}

				// Send the request message.
				boost::asio::async_write(
					m_sock,
					boost::asio::buffer(m_request_buf),
					[this](auto &&ec, auto &&bt)
					{
						on_request_sent(
							std::forward<decltype(ec)>(ec),
							std::forward<decltype(bt)>(bt)
						);
					}
				);

				return;
			}
			on_finish(ec);
		}

		void on_request_sent(
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (!ec)
			{
				m_sock.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
				
				if (m_was_cancelled)
				{
					on_finish(boost::asio::error::operation_aborted);
					return;
				}

				// Read the status line.
				boost::asio::async_read_until(
					m_sock,
					m_response.m_response_buf,
					"\r\n",
					[this](auto &&ec, auto &&bt)
					{
						on_status_line_received(
							std::forward<decltype(ec)>(ec),
							std::forward<decltype(bt)>(bt)
						);
					}
				);

				return;
			}
			on_finish(ec);
		}

		void on_status_line_received(
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (!ec)
			{
				// Parse the status line.
				std::string http_version;
				std::string str_status_code;
				std::string status_message;

				auto &&response_stream = m_response.m_response_stream;
				response_stream >> http_version;

				if (http_version != "HTTP/1.1")
				{
					// Response is incorrect.
					on_finish(http_errors::invalid_response);
					return;
				}

				response_stream >> str_status_code;

				// Convert status code to integer
				std::uint16_t status_code = 200;
				if (
					auto[p,ec] = std::from_chars(
						str_status_code.data(),
						str_status_code.data() + str_status_code.length(),
						status_code
					);
					ec != std::errc()
				)
				{
					// Response is incorrect.
					on_finish(http_errors::invalid_response);
					return;
				}

				std::getline(response_stream, status_message, '\r');
				// Remove symbol '\r' from the buffer.
				response_stream.get();

				m_response.m_status_code = status_code;
				m_response.m_status_message = std::move(status_message);
				
				if (m_was_cancelled)
				{
					on_finish(boost::asio::error::operation_aborted);
					return;
				}

				// At this point the status line is successfully
				// received and parsed.
				// Now read the response headers.
				boost::asio::async_read_until(
					m_sock,
					m_response.m_response_buf,
					"\r\n\r\n",
					[this](auto &&ec, auto &&bt)
					{
						on_headers_received(
							std::forward<decltype(ec)>(ec),
							std::forward<decltype(bt)>(bt)
						);
					}
				);

				return;
			}
			on_finish(ec);
		}

		void on_headers_received(
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (!ec)
			{
				// Parse and store headers.
				std::string header, header_name, header_value;

				auto &&response_stream = m_response.m_response_stream;
				while (std::getline(response_stream, header, '\r'))
				{
					if (header.empty()) break;
					// Remove '\n' symbol from the stream.
					response_stream.get();

					auto separator_pos = header.find(':');
					if (separator_pos != std::string::npos)
					{
						header_name = header.substr(0, separator_pos);
						auto separator_next = separator_pos + 1;
						if (separator_next < header.length())
							header_value = header.substr(separator_next);
						else
							header_value.clear();

						m_response.m_headers.emplace(
							std::move(header_name),
							std::move(header_value)
						);
					}
				}
				
				if (m_was_cancelled)
				{
					on_finish(boost::asio::error::operation_aborted);
					return;
				}

				boost::asio::async_read(
					m_sock,
					m_response.m_response_buf,
					[this](auto &&ec, auto &&bt)
					{
						on_response_body_received(
							std::forward<decltype(ec)>(ec),
							std::forward<decltype(bt)>(bt)
						);
					}
				);

				return;
			}
			on_finish(ec);
		}

		void on_response_body_received(
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			if (ec == boost::asio::error::eof)
				on_finish(boost::system::error_code());
			else
				on_finish(ec);
		}

		void on_finish(const boost::system::error_code &ec)
		{
			m_callback(*this, m_response, ec);
		}
	private:
		// Request parameters.
		constexpr inline std::uint16_t static DEFAULT_PORT = 80;
		std::uint16_t m_port = DEFAULT_PORT;
		std::string m_host;
		std::string m_uri;

		// Object unique identifier.
		std::size_t m_id = std::numeric_limits<std::size_t>::max();

		// Callback to be called when request completes.
		Callback m_callback;

		// Buffer containing the request line.
		std::string m_request_buf;

		boost::asio::ip::tcp::socket m_sock;
		boost::asio::ip::tcp::resolver m_resolver;

		HTTPResponse m_response;

		std::atomic<bool> m_was_cancelled{false};

		boost::asio::io_context &m_ioc;
};

class HTTPClient
{
	public:
		std::unique_ptr<HTTPRequest> create_request(std::size_t id)
		{
			return std::unique_ptr<HTTPRequest>(new HTTPRequest(m_ioc, id));
		}

		void close()
		{
			// Destroy the work
			m_work.reset();

			// Waiting for the I/O thread to exit
			m_thread.join();
		}

	private:
		using work_guard = 
			boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
		boost::asio::io_context m_ioc;
		work_guard m_work = boost::asio::make_work_guard(m_ioc);
		std::thread m_thread{[this]{ m_ioc.run(); }};
};

std::mutex stream_mtx;
#include <chrono>

void handle(
	const HTTPRequest &request,
	const HTTPResponse &response,
	const boost::system::error_code &ec
)
{
	if (!ec)
	{
		std::cout << "Request #" << request.get_id()
		<< " has completed. Response: "
		<< response.get_response().rdbuf()
		<< std::endl;
	}
	else if (ec == boost::asio::error::operation_aborted)
	{
		std::cerr<< "Thread#" << std::this_thread::get_id() << ' '
		<< BOOST_CURRENT_FUNCTION << ' '
		<< " Request #" << request.get_id()
		<< " has been cancelled by the user."
		<< std::endl;
	}
	else
	{
		std::cerr<< "Thread#" << std::this_thread::get_id() << ' '
		<< BOOST_CURRENT_FUNCTION << ' '
		<< " Request #" << request.get_id()
		<< " failed! Error code = " << ec
		<< " message = " << ec.message()
		<< std::endl;
	}
}

int main()
{
	try
	{
		HTTPClient client;

		auto request_one = client.create_request(1);
		request_one->set_host("localhost");
		request_one->set_uri("/");
		request_one->set_port(80);
		request_one->set_callback(handle);

		request_one->execute();

		auto request_two = client.create_request(2);
		request_two->set_host("localhost");
		request_two->set_uri("/example.html");
		request_two->set_port(80);
		request_two->set_callback(handle);

		request_two->execute();
		request_two->cancel();

		auto request_thr = client.create_request(3);
		request_thr->set_host("127.0.0.1");
		request_thr->set_uri("/index.html");
		request_thr->set_port(80);
		request_thr->set_callback(handle);

		request_thr->execute();

		// Do nothing until enter pressed
		std::cin.get();

		client.close();
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << "Thread#" << std::this_thread::get_id() << ' '
		<< BOOST_CURRENT_FUNCTION << ' '
		<< " Error occured! Error code = " << e.code()
		<< '\n';

		return e.code().value();
	}

	return 0;
}
