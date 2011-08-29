// See COPYRIGHT file for copyright information
#include "HttpRequest.h"

#include <iostream>
#include <sstream>
#include "HttpException.h"
#include <glog/logging.h>
#include <algorithm>
#include <map>

namespace fathomdb {
namespace http {

using namespace std;

string url_decode(const string& in) {
	string out;
	out.reserve(in.size());
	for (size_t i = 0; i < in.size(); ++i) {
		if (in[i] == '%') {
			if (i + 3 <= in.size()) {
				int value = 0;
				istringstream is(in.substr(i + 1, 2));
				if (is >> hex >> value) {
					out += static_cast<char> (value);
					i += 2;
				} else {
					throw invalid_argument("Invalid URI encoding");
				}
			} else {
				throw invalid_argument("Invalid URI encoding");
			}
		} else if (in[i] == '+') {
			out += ' ';
		} else {
			out += in[i];
		}
	}
	return out;
}

ParsedUri HttpRequest::parse() const {
	ParsedUri parsed;

	string::const_iterator queryIt = find(uri.begin(), uri.end(), '?');
	parsed.path = url_decode(string(uri.begin(), queryIt));

	if (queryIt != uri.end()) {
		++queryIt;
	}
	parsed.query.assign(queryIt, uri.end());

	return parsed;
}

multimap<string, string> HttpRequest::parseQueryString() const {
	ParsedUri parsed = parse();
	string& query = parsed.query;

	string::const_iterator pos = query.begin();
	string::const_iterator end = query.end();

	multimap<string, string> map;

	while (pos < end) {
		string::const_iterator sep = find(pos, end, '&');
		string::const_iterator eq = find(pos, sep, '=');

		CHECK(pos <= eq);

		string name(pos, eq);
		string value;

		if (eq == sep) {
			value.clear();
		} else {
			eq++;
			CHECK(eq <= sep);
			value.assign(eq, sep);
		}

		map.insert(make_pair(name, value));

		pos = sep;
	}

	return map;
}

string HttpRequest::getRequestPath() const {
	ParsedUri parsed = parse();
	return parsed.path;
}

string HttpRequest::getQueryParameter(const string& key, const string& defaultValue) const {
	ParsedUri parsed = parse();

	if (parsed.query.empty()) {
		return defaultValue;
	}

	multimap<string, string> map = parseQueryString();
	auto range = map.equal_range(key);
	int count = distance(range.first, range.second);
	if (count == 0) {
		return defaultValue;
	}

	if (count != 1) {
		LOG(WARNING) << "getQueryParameter only returning first value for " << key;
	}

	return range.first->second;
}

}
}
