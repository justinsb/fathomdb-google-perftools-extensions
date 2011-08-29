// See COPYRIGHT for copyright
#ifndef ADDRESSTOLINE_H_
#define ADDRESSTOLINE_H_

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <boost/array.hpp>
#include <vector>

namespace fathomdb {
namespace perftools {
using namespace std;

class AddressToLine {
	static const int buffer_size_ = 8192;
	boost::array<char, buffer_size_> buffer_;

public:
	AddressToLine();
	~AddressToLine();

	vector<string> mapAddressesToLines(const vector<string>& addresses);

private:
	string executePprof( const vector<string>& argv, const string& stdin);
};

}
}

#endif /* ADDRESSTOLINE_H_ */
