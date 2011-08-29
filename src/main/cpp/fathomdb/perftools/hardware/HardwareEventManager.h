// See COPYRIGHT for copyright
#ifndef HARDWAREEVENTMANAGER_H_
#define HARDWAREEVENTMANAGER_H_

#include <memory>

#include "SampleFormat.h"
#include "EventSet.h"

namespace fathomdb {
namespace perftools {
namespace hardware {
class EventSink;
class EventSet;

using namespace std;

/**
 * Facade that simplifies working with all the underlying event system
 */
class HardwareEventManager {
	static const int MAX_POLL = 1024;

public:
	HardwareEventManager(SampleFormat sampleFormat);

	EventSet& addEventSet(unique_ptr<EventSet> && eventSet);

	void poll(EventSink& sink, int timeout = 100);

	SampleFormat format() const {
		return format_;
	}

private:
	vector<unique_ptr<EventSet> > event_sets_;
	FileDescriptorPollList poll_list_;

	/* Currently all event sets share a single set of mmaps (channels) */
	EventChannelSet channels_;

	SampleFormat format_;
};

}
}
}

#endif /* HARDWAREEVENTMANAGER_H_ */
