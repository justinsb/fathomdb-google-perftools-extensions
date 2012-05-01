// See COPYRIGHT file for copyright information
#ifndef HTTPREQUEST_H_
#define HTTPREQUEST_H_

#include <boost/noncopyable.hpp>
#include <string>
#include <vector>
#include "HttpHeader.h"
#include <map>

namespace fathomdb {
namespace http {
using namespace std;

class ParsedUri {
public:
	string path;
	string query;
};

class HttpRequest: boost::noncopyable {
public:
	string method;
	string uri;
	string post_data;
	int http_version_major;
	int http_version_minor;
	vector<HttpHeader> headers;

	HttpRequest() {
	}

	string getRequestPath() const;

	string getQueryParameter(const string& key, const string& defaultValue) const;
	bool getQueryParameter(const string& key, string * dest) const;

private:
	ParsedUri parse() const;
	multimap<string, string> parseQueryString() const;

};

}
}

#endif /* HTTPREQUEST_H_ */
