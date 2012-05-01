// See COPYRIGHT file for copyright information

#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include <boost/noncopyable.hpp>
#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

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

	// Run the webserver until a Stop request is received
	void Run() {
		RunAsync();
		WaitForExit();
	}

	// Run the webserver; does not wait for exit
	void RunAsync();

	// Stop the webserver (asynchronously)
	void Stop(bool sync = true);

	// Wait for all threads to exit
	void WaitForExit();

private:
	/// Initiate an asynchronous accept operation.
	void start_accept();

	/// Handle completion of an asynchronous accept operation.
	void handle_accept(const boost::system::error_code& e);

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

	vector < shared_ptr<boost::thread> > threads_;
};

}
}

#endif /* HTTPSERVER_H_ */
