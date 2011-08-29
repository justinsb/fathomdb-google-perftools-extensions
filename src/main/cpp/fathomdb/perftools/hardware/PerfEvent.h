// See COPYRIGHT for copyright
#ifndef PERFEVENT_H_
#define PERFEVENT_H_

#include "SampleFormat.h"

struct perf_event_header;

namespace fathomdb {
namespace perftools {
namespace hardware {

using namespace std;

class DecodedPerfEvent;

/**
 * PerfEvent is a lightweight wrapper around a perf_event.
 * It's just a pointer, so copy-by-value is OK.
 */
class PerfEvent {
public:
	PerfEvent(perf_event_header* header) :
		header_(header) {
	}

	void dump(SampleFormat flags);

	void decode(SampleFormat flags, DecodedPerfEvent& decoded);

private:
	// Lightweight... copy-by-value is OK
	perf_event_header * header_;

};

class DecodedPerfEvent {
public:
	uint64_t ip;
	uint32_t pid;
	uint32_t tid;
	uint64_t time;
	uint64_t addr;
	uint64_t sample_id;
	uint64_t stream_id;
	uint32_t cpu;
	uint32_t res;
	uint64_t period;

	uint64_t callchain_size;
	uint64_t * callchain;
};

}
}
}

#endif /* PERFEVENT_H_ */
