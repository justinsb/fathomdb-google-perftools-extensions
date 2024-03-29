// See COPYRIGHT file for copyright information

#ifndef HTTPRESPONSE_H_
#define HTTPRESPONSE_H_

#include <string>
#include <vector>
#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/asio/buffer.hpp>
#include "HttpHeader.h"
#include <stdexcept>

namespace fathomdb {
namespace http {
using namespace std;

class HttpResponse: boost::noncopyable {
public:
	/// The status of the reply.
	enum status_type {
		ok = 200,
		created = 201,
		accepted = 202,
		no_content = 204,
		multiple_choices = 300,
		moved_permanently = 301,
		moved_temporarily = 302,
		not_modified = 304,
		bad_request = 400,
		unauthorized = 401,
		forbidden = 403,
		not_found = 404,
		method_not_supported = 405,
		internal_server_error = 500,
		not_implemented = 501,
		bad_gateway = 502,
		service_unavailable = 503
	} status;

	/// The headers to be included in the reply.
	vector<HttpHeader> headers;

	/// The content to be sent in the reply.
	string content;

	/// Convert the reply into a vector of buffers. The buffers do not own the
	/// underlying memory blocks, therefore the reply object must remain valid and
	/// not be changed until the write operation has completed.
	vector<boost::asio::const_buffer> to_buffers();

	/// Get a stock reply.
	static unique_ptr<HttpResponse> stock_reply(status_type status);

	HttpResponse() :
		status(status_type::ok) {
	}

	virtual ~HttpResponse() {
	}

	virtual bool isSpecial() const {
		return false;
	}

	virtual bool isSleep() const {
		return false;
	}

	virtual int sleepMilliseconds() const {
		throw invalid_argument("Not a sleep event");
	}

	static constexpr const char * CONTENT_TYPE_HTML = "text/html";
	static constexpr const char * CONTENT_TYPE_TEXT = "text/plain";

	void setContentType(const string& contentType);
	void setUniqueHeader(const string& name, const string& value);

	void finalize();
};

// A special Response requesting an async sleep
class SuspendProcessing: public HttpResponse {
	int sleepMilliseconds_;

public:
	SuspendProcessing(int sleepMilliseconds) :
		sleepMilliseconds_(sleepMilliseconds) {
	}

	bool isSpecial() const {
		return true;
	}

	bool isSleep() const {
		return true;
	}

	int sleepMilliseconds() const {
		return sleepMilliseconds_;
	}
};

}
}

#endif /* HTTPRESPONSE_H_ */
