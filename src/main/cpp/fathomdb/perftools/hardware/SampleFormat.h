// See COPYRIGHT for copyright
#ifndef SAMPLEFORMAT_H_
#define SAMPLEFORMAT_H_

#include <stdint.h>
#include <linux/perf_event.h>

namespace fathomdb {
namespace perftools {
namespace hardware {

using namespace std;

/**
 * Abstracts some problems dealing with the enums vs ints
 */
class SampleFormat {
public:
	SampleFormat(uint64_t format) :
		format_(format) {
	}

	bool checkFlag(perf_event_sample_format flag) const {
		return format_ & flag;
	}

	operator uint64_t() const {
		return format_;
	}
private:
	uint64_t format_;
};

}
}
}
#endif /* SAMPLEFORMAT_H_ */
