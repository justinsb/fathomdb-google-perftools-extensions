// See COPYRIGHT file for copyright information

#include "HttpServer.h"

#include <boost/thread/thread.hpp>
#include "HttpConnection.h"
#include "HttpRequestHandler.h"

using namespace std;

namespace fathomdb {
namespace http {

HttpServer::HttpServer(const string& address, const string& port, unique_ptr<HttpRequestHandler>&& request_handler, size_t thread_pool_size) :
	thread_pool_size_(thread_pool_size), signals_(io_service_), acceptor_(io_service_), new_connection_(), request_handler_(move(request_handler)) {
	// Register to handle the signals that indicate when the server should exit.
	// It is safe to register for the same signal multiple times in a program,
	// provided all registration for the specified signal is made through Asio.
	signals_.add(SIGINT);
	signals_.add(SIGTERM);
#if defined(SIGQUIT)
	signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
	signals_.async_wait(boost::bind(&HttpServer::handle_stop, this));

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(address, port);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();
}

void HttpServer::run() {
	start_accept();

	// Create a pool of threads to run all of the io_services.
	vector < shared_ptr<boost::thread> > threads;
	for (size_t i = 0; i < thread_pool_size_; ++i) {
		shared_ptr < boost::thread > thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));
		threads.push_back(thread);
	}

	// Wait for all threads in the pool to exit.
	for (size_t i = 0; i < threads.size(); ++i)
		threads[i]->join();
}

void HttpServer::start_accept() {
	new_connection_.reset(new HttpConnection(shared_from_this(), io_service_, *request_handler_));
	acceptor_.async_accept(new_connection_->socket(), boost::bind(&HttpServer::handle_accept, this, boost::asio::placeholders::error));
}

void HttpServer::handle_accept(const boost::system::error_code& e) {
	if (!e) {
		new_connection_->start();
	}

	start_accept();
}

void HttpServer::handle_stop() {
	io_service_.stop();
}

}
}
