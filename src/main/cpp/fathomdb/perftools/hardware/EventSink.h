// See COPYRIGHT for copyright
#ifndef EVENTSINK_H_
#define EVENTSINK_H_

#include "SampleFormat.h"
#include "PerfEvent.h"

namespace fathomdb {
namespace perftools {
namespace hardware {

using namespace std;

class EventSink {
public:
	EventSink(SampleFormat format) :
		format_(format) {
	}

	virtual ~EventSink() {
	}

	virtual void HandleRecordSample(PerfEvent event) {
	}

	SampleFormat format() const {
		return format_;
	}

private:
	SampleFormat format_;
};

class DumpingEventSink: public EventSink {
public:
	DumpingEventSink(SampleFormat format) :
		EventSink(format) {
	}

	virtual void HandleRecordSample(PerfEvent event) {
		event.dump(format());
	}
};

}
}
}

#endif /* EVENTSINK_H_ */
