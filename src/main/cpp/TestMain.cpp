// See COPYRIGHT for copyright
#include "TestMain.h"

#include <stdint.h>
#include <iostream>
#include <memory>
#include <exception>

#include "fathomdb/http/HttpServer.h"

#include "fathomdb/perftools/PerftoolsRequestHandler.h"
#include "fathomdb/http/HttpRequestHandler.h"

using namespace std;

using fathomdb::perftools::PerftoolsRequestHandler;
using fathomdb::http::HttpServer;
using fathomdb::http::HttpRequestHandler;

int main_http() {
	string port("8088");
	string host("0.0.0.0");

	try {
		size_t num_threads = 4;

		shared_ptr<HttpServer> httpServer;
		{
			unique_ptr<HttpRequestHandler> requestHandler(new PerftoolsRequestHandler());
			httpServer.reset(new HttpServer(host, port, move(requestHandler), num_threads));
		}

		// Run the server until stopped.
		httpServer->run();
	} catch (exception& e) {
		std::cerr << "exception: " << e.what() << "\n";
	}

	return 0;
}

extern void TestHardwarePerformanceEvents();
extern void TestGoogleProfiler();

int main() {
//	TestHardwarePerformanceEvents();
	TestGoogleProfiler();

	return 0;
}
