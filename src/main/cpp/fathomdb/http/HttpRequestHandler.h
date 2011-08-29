// See COPYRIGHT file for copyright information

#ifndef HTTPREQUESTHANDLER_H_
#define HTTPREQUESTHANDLER_H_

#include <boost/noncopyable.hpp>
#include <memory>
#include <stdexcept>

namespace fathomdb {
namespace http {
using namespace std;

class HttpRequest;
class HttpResponse;

class HttpRequestHandler: boost::noncopyable {
public:
	HttpRequestHandler() {}
	virtual ~HttpRequestHandler() {}

	virtual unique_ptr<HttpResponse> handleRequest(const HttpRequest& request) = 0;
	virtual unique_ptr<HttpResponse> resumeRequest(const HttpRequest& request, HttpResponse& previousResponse) {
		throw invalid_argument("resumeRequest not supported");
	}
};

}
}

#endif /* HTTPREQUESTHANDLER_H_ */
