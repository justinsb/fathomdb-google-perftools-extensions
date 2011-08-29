// See COPYRIGHT for copyright
#include "PerftoolsRequestHandler.h"

#include <fstream>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <glog/logging.h>

#include "google/malloc_extension.h"
#include "google/heap-profiler.h"
#include "google/profiler.h"

#include "../http/HttpResponse.h"
#include "../http/HttpRequest.h"
#include "../http/HttpException.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "AddressToLine.h"

using namespace std;
using boost::filesystem::path;
using fathomdb::perftools::AddressToLine;

namespace fathomdb {
namespace perftools {

PerftoolsRequestHandler::PerftoolsRequestHandler() {

}

PerftoolsRequestHandler::~PerftoolsRequestHandler() {
}

string readWholeFile(const path& filePath, int reserve) {
	ifstream ifs(filePath.string());
	string str;

	// If the kernel is dynamically generating it, we don't want to put more load onto the kernel
	// Also, tellg doesn't seem to work on /proc files
	if (reserve == 0) {
		ifs.seekg(0, ios::end);
		str.reserve(ifs.tellg());
		ifs.seekg(0, ios::beg);
	} else {
		str.reserve(reserve);
	}

	str.assign((istreambuf_iterator<char> (ifs)), istreambuf_iterator<char> ());

	return str;
}

static void appendMaps(string& content) {
	string maps = readWholeFile("/proc/self/maps", 128 * 1024);

	content.reserve(content.size() + 40 + maps.size());

	content.append("\nMAPPED_LIBRARIES:\n");
	content.append(maps);
}

void PerftoolsRequestHandler::handleSymbolRequest(const HttpRequest& request, HttpResponse& response) {
	//			This means that after the HTTP headers, pprof will pass in a list of hex addresses connected by +, like so:
	//
	//			   curl -d '0x0824d061+0x0824d1cf' http://remote_host:80/pprof/symbol
	//			The server should read the POST data, which will be in one line, and for each hex value, should write one line of output to the output stream, like so:

	vector<string> addresses;
	boost::algorithm::split(addresses, request.post_data, boost::algorithm::is_any_of("+"));

	AddressToLine addressToLine;

	vector<string> lines = addressToLine.mapAddressesToLines(addresses);

	ostringstream out;

	//	for (auto it = addresses.begin(); it != addresses.end(); it++) {
	//		string& address = *it;
	//		const char * start = address.c_str();
	//		if (start[0] == '0' && start[1] == 'x') {
	//			start += 2;
	//		}
	//		char * firstBad = 0;
	//
	//		uintptr_t addressVal = strtoull(start, &firstBad, 16);
	//		string procName;
	//
	//		if (*firstBad == 0) {
	//			procName = addressToLine.mapAddressToLine(addressVal);
	//		} else {
	//			procName = "invalid_address_";
	//			procName.append(address);
	//		}
	//		out << *it << "\t" << procName << "\n";
	//	}

	CHECK_EQ(lines.size(), addresses.size());

	for (size_t i = 0; i < lines.size(); i++) {
		out << addresses[i] <<"\t" << lines[i] << "\n";
	}

	response.content = out.str();

	//			<hex address><tab><function name>
	//			For instance:
	//
	//			0x08b2dabd    _Update
	//			The other reason this is the most difficult request to implement, is that the application will have to figure out for itself how to map from address to function name. One possibility is to run nm -C -n <program name> to get the mappings at program-compile-time. Another, at least on Linux, is to call out to addr2line for every pprof/symbol call, for instance addr2line -Cfse /proc//exe 0x12345678 0x876543210 (presumably with some caching!)

}

unique_ptr<HttpResponse> PerftoolsRequestHandler::handleRequest(const HttpRequest& request) {
	unique_ptr<HttpResponse> response(new HttpResponse());
	response->setContentType(HttpResponse::CONTENT_TYPE_TEXT);

	// Decode url to path.
	std::string requestPath = request.getRequestPath();

	LOG(INFO) << "Got request: " << request.method << " " << requestPath;

	if (requestPath == "/pprof/symbol") {
		if (request.method == "GET") {
			//			When the server receives a GET request for /pprof/symbol, it should return a line formatted like so:
			//			   num_symbols: ###
			//			where ### is the number of symbols found in the binary. (For now, the only important distinction is whether the value is 0, which it is for executables that lack debug information, or not-0).
			response->content = "num_symbols: 1";
			return response;
		}

		if (request.method == "POST") {
			handleSymbolRequest(request, *response);
			return response;
		}

		throw HttpException(HttpResponse::method_not_supported);
	}

	if (requestPath == "/pprof/cmdline") {
		string data = readWholeFile("/proc/self/cmdline", 32 * 1024);
		// Replace null characters with newlines
		boost::algorithm::replace_all(data, string(1, '\0'), "\n");

		swap(response->content, data);

		return response;
	}

	if (requestPath == "/pprof/heap") {
		response->content.reserve(1 << 20);

		// TODO: Could call GetHeapProfile first,
		// and only fall back to GetHeapSample if
		// the former returns an empty string?
		MallocExtension::instance()->GetHeapSample(&response->content);

		// It looks like this is already in the heap sample now??
		//		appendMaps(response->content);

		return response;
	}

	if (requestPath == "/pprof/growth") {
		response->content.reserve(1 << 20);

		MallocExtension::instance()->GetHeapGrowthStacks(&response->content);

		// It looks like this is already in the output now??
		//appendMaps(response->content);

		return response;
	}

	if (requestPath == "/pprof/heapstats") {
		// Yuk...
		size_t bufferSize = 1 << 20;
		unique_ptr<char[]> buffer(new char[bufferSize]);
		MallocExtension::instance()->GetStats(buffer.get(), bufferSize);

		// TODO: Loop, increasing buffer size if we under-estimated??
		response->content = buffer.get();

		return response;
	}

	if (requestPath == "/pprof/profile") {
		{
			ostringstream s;
			s << "/var/tmp/pprof.profile." << getpid();

			profilepath_ = s.str();
		}

		int n = 30;
		{
			string value = request.getQueryParameter("seconds", "");
			if (!value.empty()) {
				n = boost::lexical_cast<int>(value);
			}
		}

		LOG(WARNING) << "HTTP request to profile for " << n << " seconds";

		ProfilerStart(profilepath_.c_str());
		unique_ptr<HttpResponse> suspendResponse(new SuspendProcessing(n * 1000));
		return suspendResponse;
	}

	LOG(WARNING) << "Handler not found for request: " << request.method << " " << requestPath;

	throw HttpException(HttpResponse::not_found);
}

unique_ptr<HttpResponse> PerftoolsRequestHandler::resumeRequest(const HttpRequest& request, HttpResponse& previousResponse) {
	LOG(WARNING) << "Finishing profiling";

	ProfilerStop();
	ProfilerFlush();

	unique_ptr<HttpResponse> response(new HttpResponse());
	response->setContentType(HttpResponse::CONTENT_TYPE_TEXT);

	response->content = readWholeFile(profilepath_, 0);
	boost::filesystem::remove(profilepath_);

	appendMaps(response->content);

	return response;
}

}
}
