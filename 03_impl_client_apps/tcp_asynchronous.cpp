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

#include <thread>
#include <mutex>
#include <memory>
#include <iostream>
#include <charconv>

// Interface of class represents a context of a single request.
class ISession
{
	public:
		virtual ~ISession() = default;

		virtual void cancel() = 0;

		virtual void invokeCallback(
			const boost::system::error_code&
		) = 0;

		virtual bool isSessionWasCancelled() const = 0;
		virtual std::size_t getID() const = 0;

		virtual void shutdown(
			boost::asio::ip::tcp::socket::shutdown_type, 
			boost::system::error_code&
		) = 0;

	protected:
		virtual boost::asio::ip::tcp::socket& sock() = 0;
		virtual const boost::asio::ip::tcp::endpoint& ep() const = 0;
		virtual boost::asio::const_buffer getWriteBuffer() const = 0;
		virtual boost::asio::streambuf&  getResponseBuffer() = 0;
};

// Base Session
class BaseSession : public ISession
{
	public:
		template <class Callback>
		void asyncConnect(Callback &&callback)
		{
			constexpr bool is_valid_callback = std::is_invocable_r_v<
				void,
				decltype(callback),
				const boost::system::error_code&
			>;
			static_assert(is_valid_callback, "invalid callback");
			sock().async_connect(
				ep(),
				std::forward<Callback>(callback)
			);
		}

		template <class Callback>
		void asyncWrite(Callback &&callback) 
		{
			constexpr bool is_valid_callback = std::is_invocable_r_v<
				void,
				decltype(callback),
				const boost::system::error_code&,
				std::size_t
			>;
			static_assert(is_valid_callback, "invalid callback");
			boost::asio::async_write(
				sock(),
				getWriteBuffer(),
				std::forward<Callback>(callback)
			);
		}

		template <class Callback>
		void asyncReadUntil(std::string_view delim, Callback &&callback)
		{
			constexpr bool is_valid_callback = std::is_invocable_r_v<
				void,
				decltype(callback),
				const boost::system::error_code&,
				std::size_t
			>;
			static_assert(is_valid_callback, "invalid callback");
			boost::asio::async_read_until(
				sock(),
				getResponseBuffer(),
				delim,
				std::forward<Callback>(callback)
			);
		}
};

// Class represents a context of a single request.
// Callback is invocable type which is called when a request is complete.
template< class Callback >
class Session final : public BaseSession
{
	public:
		Session(
			boost::asio::io_context &ioc,
			std::string_view raw_ip_address,
			std::uint16_t port_num,
			const std::string &request,
			std::size_t id,
			Callback &&callback
		) :
		m_sock(ioc),
		m_ep(boost::asio::ip::make_address(raw_ip_address),port_num),
		m_request(request),
		m_id(id),
		m_callback(std::forward<Callback>(callback))
		{
			constexpr bool is_valid_callback = std::is_invocable_r_v<
				void,
				decltype(m_callback),
				std::size_t,
				const std::string&,
				const boost::system::error_code&
			>;
			static_assert(is_valid_callback, "invalid callback");

			m_sock.open(m_ep.protocol());
		}

		~Session() override = default;

		void cancel() override
		{
			m_was_cancelled = true;
			m_sock.cancel();
		}

		void invokeCallback(
			const boost::system::error_code &ec
		) override
		{
			std::string response;
			std::istream is(&m_response_buf);
			std::getline(is, response);

			m_callback(m_id, response, ec);
		}

		bool isSessionWasCancelled() const override
		{
			return m_was_cancelled;
		}

		std::size_t getID() const override
		{
			return m_id;
		}

		void shutdown(
			boost::asio::ip::tcp::socket::shutdown_type type, 
			boost::system::error_code& ec
		) override
		{
			m_sock.shutdown(type,ec);
		}

	private:
		boost::asio::ip::tcp::socket& sock() override
		{
			return m_sock;
		}

		const boost::asio::ip::tcp::endpoint& ep() const override
		{
			return  m_ep;
		}

		boost::asio::const_buffer getWriteBuffer() const override
		{
			return boost::asio::buffer(m_request);
		}

		boost::asio::streambuf&  getResponseBuffer() override
		{
			return m_response_buf;
		}

	private:
		boost::asio::ip::tcp::socket m_sock; // Socket used for cmmunication.
		boost::asio::ip::tcp::endpoint m_ep; // Remote endpoint.
		const std::string m_request;		 // Request string.

		// streambuf where the response will be stored.
		boost::asio::streambuf m_response_buf;

		// Contains the description of an error if one occurs during
		// the request life cycle.
		boost::system::error_code m_ec;

		const std::size_t m_id; // Unique ID assigned to the request.

		// Pointer to the function to be called when the request
		// completes.
		std::decay_t<Callback> m_callback;

		std::atomic<bool> m_was_cancelled{false};
};

class AsyncTCPClient
{
	public:
		// C++ noncopyable and nonmoveable 
		AsyncTCPClient(const AsyncTCPClient&) = delete;
		AsyncTCPClient &operator=(const AsyncTCPClient&) = delete;

		AsyncTCPClient() = default;

		template< class Rep, class Period, class Callback >
		void emulateLongComputationOp(
			const std::chrono::duration<Rep, Period>& duration,
			std::string_view raw_ip_address,
			std::uint16_t port_num,
			Callback &&callback,
			std::size_t request_id
		)
		{
			// Preparing the request string.
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

			std::shared_ptr<BaseSession> session(new Session<Callback>(
					m_ioc,
					raw_ip_address,
					port_num,
					request,
					request_id,
					std::forward<Callback>(callback)
				)
			);

			// Add new session to the map of active sessions so
			// that we can access it if the user decides to cannel
			// the corresponding request before if completes.
			// Because active sessions map can be accessed from
			// multiple threads, we guard it with a mutex to avoid
			// data corruption.
			{
				std::lock_guard lock(m_active_sessions_guard);
				m_active_sessions[request_id] = session;
			}
			
			auto session_raw_ptr = session.get();
			session_raw_ptr->asyncConnect(
				[this, session=std::move(session)](auto &&ec) mutable
				{
					onConnect(std::move(session), ec);
				}
			);
		}

		void cancelRequest(std::size_t request_id)
		{
			std::lock_guard lock(m_active_sessions_guard);

			if (
				auto it = m_active_sessions.find(request_id);
				it != m_active_sessions.end()
			)
			{
				it->second->cancel();
			}
		}

		void close()
		{
			// Destroy work object. This allows the I/O thread to
			// exits the event loop when are no more pending
			// asynchronous operations.
			m_work.reset();

			// Wait for the I/O thread to exit.
			m_thread.join();
		}

	private:
		void onConnect(
			std::shared_ptr<BaseSession> session, 
			const boost::system::error_code &ec
		)
		{
			if (ec)
			{
				onRequestComplete(std::move(session), ec);
				return;
			}

			if (bool cancelled = session->isSessionWasCancelled(); cancelled)
			{
				onRequestComplete(std::move(session), ec, cancelled);
				return;
			}

			auto session_raw_ptr = session.get();
			session_raw_ptr->asyncWrite(
				[this, session=std::move(session)](auto &&ec, auto bt) mutable
				{
					onWriteComplete(std::move(session), ec, bt);
				}
			);
		}

		void onWriteComplete(
			std::shared_ptr<BaseSession> session, 
			const boost::system::error_code &ec,
			std::size_t bytes_transferred
		)
		{
			std::ignore = bytes_transferred;
			if (ec)
			{
				onRequestComplete(std::move(session), ec);
				return;
			}

			if (bool cancelled = session->isSessionWasCancelled(); cancelled)
			{
				onRequestComplete(std::move(session), ec, cancelled);
				return;
			}
			
			auto session_raw_ptr = session.get();
			session_raw_ptr->asyncReadUntil(
				"\n",
				[this, session=std::move(session)](auto &&ec, auto bt) mutable
				{
					bool cancelled = session->isSessionWasCancelled();
					onRequestComplete(std::move(session), ec, cancelled);
				}
			);
		}

		void onRequestComplete(
			std::shared_ptr<BaseSession> session, 
			const boost::system::error_code &ec,
			bool cancelled = false
		)
		{
			// Shutting down the connection. This method may
			// fail in case socket is not connected. We don't care
			// about the error code if this function fails.
			boost::system::error_code ignored_ec;
			session->shutdown(
				boost::asio::ip::tcp::socket::shutdown_both,
				ignored_ec
			);

			// Remove session from the map of active sessions.
			{
				std::lock_guard lock(m_active_sessions_guard);
				if (
					auto it = m_active_sessions.find(session->getID()); 
					it != m_active_sessions.end()
				)
					m_active_sessions.erase(it);
			}

			boost::system::error_code actual_ec = 
				cancelled ? boost::asio::error::operation_aborted : ec;

			// Call the callback provided by the user.
			session->invokeCallback(actual_ec);
		}
	private:
		inline static constexpr char m_op_name[] = "EMULATE_LONG_COMP_OP ";
		boost::asio::io_context m_ioc;
		std::map<std::size_t, std::shared_ptr<BaseSession>> m_active_sessions;
		std::mutex m_active_sessions_guard;
		using work_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
		std::unique_ptr<work_type> m_work{ std::make_unique<work_type>(boost::asio::make_work_guard(m_ioc)) };
		std::thread m_thread{ [this](){ m_ioc.run(); } };
};

void handler(
	std::size_t request_id, 
	const std::string& response,
	const boost::system::error_code& ec
)
{
	if (!ec)
	{
		std::cout << "Request #" << request_id
		<< " has completed. Response: "
		<< response << '\n';
	}
	else if (ec == boost::asio::error::operation_aborted)
	{
		std::cout << "Request #" << request_id
		<< " has been cancelled by the user.\n";
	}
	else
	{
		std::cerr << "Request #" << request_id
		<< " failed! Error = " << ec
		<< '\n';
	}
}

// Run 'echo "" | awk '{  print "OK\n" }' | netcat -l 127.0.0.1 -p 3333 &\
echo "" | awk '{  print "OK\n" }' | netcat -l 127.0.0.1 -p 3334 &\
echo "" | awk '{  print "OK\n" }' | netcat -l 127.0.0.1 -p 3335 &'
// to test this example
int main()
{
	try
	{
		AsyncTCPClient client;

		using namespace std::chrono_literals;
		// Here we emulate the user's behaviour.

		// User initiates a request with id 1.
		client.emulateLongComputationOp(10s, "127.0.0.1", 3333, handler, 1);
		// Then does nothing for 1 microseconds.
		std::this_thread::sleep_for(1us);
		// Then initiates another request with id 2.
		client.emulateLongComputationOp(11s, "127.0.0.1", 3334, handler, 2);
		// Then decides to cancel the request with id 1.
		client.cancelRequest(1);
		// Initiates one more request assigning id 3 to it.
		client.emulateLongComputationOp(12s, "127.0.0.1", 3335, handler, 3);
		// Does nothing for another 15 seconds.
		std::this_thread::sleep_for(15s);
		// Decides to exit the application.
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
