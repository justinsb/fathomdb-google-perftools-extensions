// See COPYRIGHT file for copyright information
#ifndef HTTPEXCEPTION_H_
#define HTTPEXCEPTION_H_

#include <exception>
#include "HttpResponse.h"

using std::exception;

namespace fathomdb {
namespace http {

class HttpException: public exception {
	HttpResponse::status_type _statusType;

public:
	HttpException(HttpResponse::status_type statusType) :
		_statusType(statusType) {
	}

	virtual ~HttpException() throw () {
	}

	virtual const char* what() const throw () {
		return "HttpException";
	}

	HttpResponse::status_type statusCode() const {
		return _statusType;
	}
};

}
}

#endif /* HTTPEXCEPTION_H_ */
