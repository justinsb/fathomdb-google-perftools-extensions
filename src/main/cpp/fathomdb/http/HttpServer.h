// See COPYRIGHT file for copyright information

#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include <boost/noncopyable.hpp>
#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>

namespace fathomdb {
namespace http {
using namespace std;
class HttpRequestHandler;
class HttpConnection;

/// The top-level class of the HTTP server.
class HttpServer: public enable_shared_from_this<HttpServer>, private boost::noncopyable {
public:
	/// Construct the server to listen on the specified TCP address and port, and
	/// serve up files from the given directory.
	explicit HttpServer(const string& address, const string& port, unique_ptr<HttpRequestHandler>&& request_handler, size_t thread_pool_size);

	/// Run the server's io_service loop.
	void run();

private:
	/// Initiate an asynchronous accept operation.
	void start_accept();

	/// Handle completion of an asynchronous accept operation.
	void handle_accept(const boost::system::error_code& e);

	/// Handle a request to stop the server.
	void handle_stop();

	/// The number of threads that will call io_service::run().
	size_t thread_pool_size_;

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	/// The signal_set is used to register for process termination notifications.
	boost::asio::signal_set signals_;

	/// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor_;

	/// The next connection to be accepted.
	shared_ptr<HttpConnection> new_connection_;

	/// The handler for all incoming requests.
	unique_ptr<HttpRequestHandler> request_handler_;
};

}
}

#endif /* HTTPSERVER_H_ */
