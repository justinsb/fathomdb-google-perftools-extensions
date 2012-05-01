// See COPYRIGHT file for copyright information
#ifndef HTTPHEADER_H_
#define HTTPHEADER_H_

#include <string>

namespace fathomdb {
namespace http {

using namespace std;

class HttpHeader {
public:
	string name;
	string value;

	HttpHeader() {
	}

	HttpHeader(const string& _name, const string& _value) :
		name(_name), value(_value) {
	}
};

}
}

#endif /* HTTPHEADER_H_ */
