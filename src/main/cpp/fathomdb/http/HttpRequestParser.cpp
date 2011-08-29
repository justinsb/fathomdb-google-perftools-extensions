// See COPYRIGHT file for copyright information

#include "HttpRequestParser.h"
#include <boost/logic/tribool.hpp>
#include "HttpRequest.h"
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

namespace fathomdb {
namespace http {

using boost::logic::tribool;

HttpRequestParser::HttpRequestParser() :
	state_(method_start), postDataLength_(0) {
}

void HttpRequestParser::reset() {
	state_ = method_start;
}

tribool HttpRequestParser::consume(HttpRequest& req, char input) {
	switch (state_) {
	case method_start:
		if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
			return false;
		} else {
			state_ = method;
			req.method.push_back(input);
			return boost::indeterminate;
		}
	case method:
		if (input == ' ') {
			state_ = uri;
			return boost::indeterminate;
		} else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
			return false;
		} else {
			req.method.push_back(input);
			return boost::indeterminate;
		}
	case uri_start:
		if (is_ctl(input)) {
			return false;
		} else {
			state_ = uri;
			req.uri.push_back(input);
			return boost::indeterminate;
		}
	case uri:
		if (input == ' ') {
			state_ = http_version_h;
			return boost::indeterminate;
		} else if (is_ctl(input)) {
			return false;
		} else {
			req.uri.push_back(input);
			return boost::indeterminate;
		}
	case http_version_h:
		if (input == 'H') {
			state_ = http_version_t_1;
			return boost::indeterminate;
		} else {
			return false;
		}
	case http_version_t_1:
		if (input == 'T') {
			state_ = http_version_t_2;
			return boost::indeterminate;
		} else {
			return false;
		}
	case http_version_t_2:
		if (input == 'T') {
			state_ = http_version_p;
			return boost::indeterminate;
		} else {
			return false;
		}
	case http_version_p:
		if (input == 'P') {
			state_ = http_version_slash;
			return boost::indeterminate;
		} else {
			return false;
		}
	case http_version_slash:
		if (input == '/') {
			req.http_version_major = 0;
			req.http_version_minor = 0;
			state_ = http_version_major_start;
			return boost::indeterminate;
		} else {
			return false;
		}
	case http_version_major_start:
		if (is_digit(input)) {
			req.http_version_major = req.http_version_major * 10 + input - '0';
			state_ = http_version_major;
			return boost::indeterminate;
		} else {
			return false;
		}
	case http_version_major:
		if (input == '.') {
			state_ = http_version_minor_start;
			return boost::indeterminate;
		} else if (is_digit(input)) {
			req.http_version_major = req.http_version_major * 10 + input - '0';
			return boost::indeterminate;
		} else {
			return false;
		}
	case http_version_minor_start:
		if (is_digit(input)) {
			req.http_version_minor = req.http_version_minor * 10 + input - '0';
			state_ = http_version_minor;
			return boost::indeterminate;
		} else {
			return false;
		}
	case http_version_minor:
		if (input == '\r') {
			state_ = expecting_newline_1;
			return boost::indeterminate;
		} else if (is_digit(input)) {
			req.http_version_minor = req.http_version_minor * 10 + input - '0';
			return boost::indeterminate;
		} else {
			return false;
		}
	case expecting_newline_1:
		if (input == '\n') {
			state_ = header_line_start;
			return boost::indeterminate;
		} else {
			return false;
		}
	case header_line_start:
		if (input == '\r') {
			state_ = expecting_newline_3;
			return boost::indeterminate;
		} else if (!req.headers.empty() && (input == ' ' || input == '\t')) {
			state_ = header_lws;
			return boost::indeterminate;
		} else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
			return false;
		} else {
			req.headers.push_back(HttpHeader());
			req.headers.back().name.push_back(input);
			state_ = header_name;
			return boost::indeterminate;
		}
	case header_lws:
		if (input == '\r') {
			state_ = expecting_newline_2;
			return boost::indeterminate;
		} else if (input == ' ' || input == '\t') {
			return boost::indeterminate;
		} else if (is_ctl(input)) {
			return false;
		} else {
			state_ = header_value;
			req.headers.back().value.push_back(input);
			return boost::indeterminate;
		}
	case header_name:
		if (input == ':') {
			state_ = space_before_header_value;
			return boost::indeterminate;
		} else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
			return false;
		} else {
			req.headers.back().name.push_back(input);
			return boost::indeterminate;
		}
	case space_before_header_value:
		if (input == ' ') {
			state_ = header_value;
			return boost::indeterminate;
		} else {
			return false;
		}
	case header_value:
		if (input == '\r') {
			state_ = expecting_newline_2;
			return boost::indeterminate;
		} else if (is_ctl(input)) {
			return false;
		} else {
			req.headers.back().value.push_back(input);
			return boost::indeterminate;
		}
	case expecting_newline_2:
		if (input == '\n') {
			state_ = header_line_start;
			return boost::indeterminate;
		} else {
			return false;
		}
	case expecting_newline_3:
		if (input == '\n') {
			if ("POST" == req.method) {
				size_t contentLength = 0;
				bool found = false;
				for (auto it = req.headers.begin(); it != req.headers.end(); it++) {
					if ("Content-Length" == it->name) {
						try {
							contentLength = boost::lexical_cast<size_t>(it->value);
							found = true;
							break;
						} catch (boost::bad_lexical_cast& blc) {
							LOG(WARNING) << "Invalid Content-Length value in POST: " << it->value;
							return false;
						}
					}
				}
				if (!found) {
					LOG(WARNING) << "Content-Length header not found or negative value in POST: " << contentLength;
					return false;
				}
				postDataLength_ = contentLength;
				if (contentLength != 0) {
					req.post_data.reserve(contentLength);
					state_ = post_data;
					return boost::indeterminate;
				} else {
					// Don't wait for data that is never coming!
					return true;
				}
			}
			return true;
		}
	case post_data:
		req.post_data.push_back(input);
		if (req.post_data.size() == postDataLength_) {
			return true;
		}
		return boost::indeterminate;
	default:
		return false;
	}
}

bool HttpRequestParser::is_char(int c) {
	return c >= 0 && c <= 127;
}

bool HttpRequestParser::is_ctl(int c) {
	return (c >= 0 && c <= 31) || (c == 127);
}

bool HttpRequestParser::is_tspecial(int c) {
	switch (c) {
	case '(':
	case ')':
	case '<':
	case '>':
	case '@':
	case ',':
	case ';':
	case ':':
	case '\\':
	case '"':
	case '/':
	case '[':
	case ']':
	case '?':
	case '=':
	case '{':
	case '}':
	case ' ':
	case '\t':
		return true;
	default:
		return false;
	}
}

bool HttpRequestParser::is_digit(int c) {
	return c >= '0' && c <= '9';
}

}
}
