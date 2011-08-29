// See COPYRIGHT for copyright
#ifndef HARDWAREPERFTOOLSEVENTSOURCE_H_
#define HARDWAREPERFTOOLSEVENTSOURCE_H_

#include <string>
#include <memory>

#include "google/profiler_extension.h"

namespace fathomdb {
namespace perftools {
namespace hardware {

using namespace std;

class HardwareEventManager;
class EventSet;

class HardwarePerftoolsEventSource: public ProfileEventSource {
public:
	HardwarePerftoolsEventSource(const string& event_spec, ::ProfileRecordCallback callback) :
		event_spec_(event_spec), callback_(callback), events_enabled_(false) {
	}

	void RegisterThread(int callback_count);

	void EnableEvents() {
		events_enabled_ = true;
	}
	void DisableEvents() {
		events_enabled_ = true;
	}

	void RegisteredCallback(int new_callback_count);
	void UnregisteredCallback(int new_callback_count);

	void Reset();

	EventSet& BuildEventSystem(const string& events);

private:
	//  SpinLock lock_;
	string event_spec_;
	ProfileRecordCallback callback_;

	unique_ptr<HardwareEventManager> event_system_;

	void StartBackgroundThread();
	void StopBackgroundThread();

	pthread_t thread_;
	bool thread_stop_;
	bool events_enabled_;

	static void * BackgroundThreadMain(void * arg);
};

}
}
}

#endif /* HARDWAREPERFTOOLSEVENTSOURCE_H_ */
