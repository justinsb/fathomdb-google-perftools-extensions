// See COPYRIGHT for copyright
#include "HardwareEventManager.h"

using namespace std;

namespace fathomdb {
namespace perftools {
namespace hardware {

void HardwareEventManager::poll(EventSink& sink, int timeout) {
	int events = poll_list_.poll(timeout);
	if (events > 0) {
		auto keys = channels_.keys();
		for (auto it = keys.begin(); it != keys.end(); it++) {
			EventChannel& channel = channels_.getChannel(*it);
			channel.readEvents(sink);
		}
	}
}

EventSet& HardwareEventManager::addEventSet(unique_ptr<EventSet> && eventSetPtr) {
	event_sets_.push_back(move(eventSetPtr));
	EventSet& eventSet = *event_sets_.back();

	for (size_t i = 0; i < eventSet.size(); i++) {
		const Event& event = eventSet[i];

		// We separate out the channels so that multiple cpus/threads don't go into the same buffer
		EventChannelSet::key_t key(event.cpu(), event.tid());
		EventChannel& channel = channels_.getChannel(key);
		channel.add(event.fileDescriptor(), poll_list_);
	}
	return eventSet;
}

HardwareEventManager::HardwareEventManager(SampleFormat format) :
poll_list_(MAX_POLL), channels_(format), format_(format) {
}

}
}
}
