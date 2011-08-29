// See COPYRIGHT file for copyright information
#ifndef HTTPCONNECTION_H_
#define HTTPCONNECTION_H_

#include <memory>

#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>

#include "HttpRequest.h"
#include "HttpRequestParser.h"
#include "HttpResponse.h"

namespace fathomdb {
namespace http {
using namespace std;
class HttpRequestHandler;
class HttpServer;
class SleepHandler;

class HttpConnection: public enable_shared_from_this<HttpConnection> , private boost::noncopyable {
public:
	/// Construct a connection with the given io_service.
	explicit HttpConnection(shared_ptr<HttpServer> server, boost::asio::io_service& io_service, HttpRequestHandler& handler);

	/// Get the socket associated with the connection.
	boost::asio::ip::tcp::socket& socket();

	/// Start the first asynchronous operation for the connection.
	void start();

private:
	/// Handle completion of a read operation.
	void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred);

	/// Handle completion of a write operation.
	void handle_write(const boost::system::error_code& e);

	void handleWaitComplete(const boost::system::error_code& e, shared_ptr<SleepHandler> sleepHandler);

	void buildReply(bool continuation);
	void sendReply();

	// Pointer to parent, to keep it alive
	shared_ptr<HttpServer> server_;

	/// Strand to ensure the connection's handlers are not called concurrently.
	boost::asio::io_service::strand strand_;

	/// Socket for the connection.
	boost::asio::ip::tcp::socket socket_;

	/// The handler used to process the incoming request.
	HttpRequestHandler& request_handler_;

	/// Buffer for incoming data.
	boost::array<char, 8192> buffer_;

	/// The incoming request.
	HttpRequest request_;

	/// The parser for the incoming request.
	HttpRequestParser request_parser_;

	/// The reply to be sent back to the client.
	unique_ptr<HttpResponse> reply_;
};

}
}

#endif /* HTTPCONNECTION_H_ */
