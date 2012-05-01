// See COPYRIGHT file for copyright information
#include "HttpConnection.h"

#include <vector>
#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>

#include "HttpRequestHandler.h"
#include "HttpException.h"
#include <glog/logging.h>
#include <boost/asio.hpp>

namespace fathomdb {
namespace http {

HttpConnection::HttpConnection(shared_ptr<HttpServer> server, boost::asio::io_service& io_service, HttpRequestHandler& handler) :
	server_(server), strand_(io_service), socket_(io_service), request_handler_(handler) {
}

boost::asio::ip::tcp::socket& HttpConnection::socket() {
	return socket_;
}

template<class T> T * get_pointer(std::shared_ptr<T> const& p) {
	return p.get();
}

void HttpConnection::start() {
	socket_.async_read_some(boost::asio::buffer(buffer_), strand_.wrap(boost::bind(&HttpConnection::handle_read, shared_from_this(), boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)));
}

// Helper class that handles an (async) sleep during an HTTP call, used by profiler
class SleepHandler {
	typedef boost::asio::deadline_timer timer_t;
	timer_t timer_;

public:
	SleepHandler(boost::asio::io_service& io) :
		timer_(io) {
	}

	template<class WaitHandler>
	void async_wait(const timer_t::duration_type& delay, BOOST_ASIO_MOVE_ARG(WaitHandler) handler) {
		timer_.expires_from_now(delay);
		timer_.async_wait(handler);
	}
};

void HttpConnection::handle_read(const boost::system::error_code& e, size_t bytes_transferred) {
	if (!e) {
		boost::tribool result;
		boost::tie(result, boost::tuples::ignore) = request_parser_.parse(request_, buffer_.data(), buffer_.data() + bytes_transferred);

		if (result || !result) {
			if (result) {
				buildReply(false);
			}
			if (!result) {
				reply_ = HttpResponse::stock_reply(HttpResponse::status_type::bad_request);
			}

			sendReply();
		} else {
			socket_.async_read_some(boost::asio::buffer(buffer_), strand_.wrap(boost::bind(&HttpConnection::handle_read, shared_from_this(), boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)));
		}
	}

	// If an error occurs then no new asynchronous operations are started. This
	// means that all shared_ptr references to the connection object will
	// disappear and the object will be destroyed automatically after this
	// handler returns. The connection class's destructor closes the socket.
}

void HttpConnection::handle_write(const boost::system::error_code& e) {
	if (!e) {
		// Initiate graceful connection closure.
		boost::system::error_code ignored_ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
	}

	// No new asynchronous operations are started. This means that all shared_ptr
	// references to the connection object will disappear and the object will be
	// destroyed automatically after this handler returns. The connection class's
	// destructor closes the socket.
}

void HttpConnection::buildReply(bool continuation) {
	try {
		if (continuation) {
			reply_ = request_handler_.resumeRequest(request_, *reply_);
		} else {
			reply_ = request_handler_.handleRequest(request_);
		}
	} catch (HttpException& e) {
		reply_ = HttpResponse::stock_reply(e.statusCode());
	} catch (exception& e) {
		LOG(WARNING) << "Unexpected internal error: " << e.what();
		reply_ = HttpResponse::stock_reply(HttpResponse::status_type::internal_server_error);
	} catch (...) {
		LOG(WARNING) << "Unknown internal error";
		reply_ = HttpResponse::stock_reply(HttpResponse::status_type::internal_server_error);
	}
}

void HttpConnection::sendReply() {
	if (reply_->isSpecial()) {
		if (reply_->isSleep()) {
			shared_ptr<SleepHandler> sleepHandler(new SleepHandler(strand_.get_io_service()));

			sleepHandler->async_wait(boost::posix_time::milliseconds(reply_->sleepMilliseconds()), strand_.wrap(boost::bind(&HttpConnection::handleWaitComplete, shared_from_this(),
					boost::asio::placeholders::error, sleepHandler)));
		} else {
			CHECK(false)<< "Unhandled special reply";
		}
	}
	else
	{
		reply_->finalize();
		boost::asio::async_write(socket_, reply_->to_buffers(), strand_.wrap(boost::bind(&HttpConnection::handle_write, shared_from_this(), boost::asio::placeholders::error)));
	}
}

void HttpConnection::handleWaitComplete(const boost::system::error_code& e, shared_ptr<SleepHandler> sleepHandler) {
	if (e) {
		LOG(WARNING) << "Error in async wait";
		reply_ = HttpResponse::stock_reply(HttpResponse::status_type::internal_server_error);
	}
	else {
		buildReply(true);

		sendReply();
	}
}

}
}
