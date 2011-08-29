// See COPYRIGHT for copyright
#ifndef PERFTOOLSREQUESTHANDLER_H_
#define PERFTOOLSREQUESTHANDLER_H_

#include <string>
#include "../http/HttpRequestHandler.h"

namespace fathomdb {
namespace perftools {
using namespace std;
using namespace fathomdb::http;

class PerftoolsRequestHandler : public HttpRequestHandler {
	string profilepath_;

public:
	PerftoolsRequestHandler();
	virtual ~PerftoolsRequestHandler();

	unique_ptr<HttpResponse> handleRequest(const HttpRequest& request);
	unique_ptr<HttpResponse> resumeRequest(const HttpRequest& request, HttpResponse& previousResponse);

private:
	void handleSymbolRequest(const HttpRequest& request, HttpResponse& response);
};

}
}

#endif /* PERFTOOLSREQUESTHANDLER_H_ */
