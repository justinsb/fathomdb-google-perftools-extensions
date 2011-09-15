// See COPYRIGHT for copyright
#include "HardwarePerftoolsEventSource.h"

#include <pthread.h>
#include <stdexcept>

#include <linux/perf_event.h>

#include <glog/logging.h>

#include "EventSink.h"
#include "HardwareEventManager.h"
#include <iostream>
#include <sys/syscall.h>

using namespace std;

namespace fathomdb {
namespace perftools {
namespace hardware {

#define FATAL(x) { throw invalid_argument(x); }

bool removeIfStartsWith(string& s, const string& find) {
	bool found = false;
	if (0 == s.compare(0, find.size(), find)) {
		found = true;
		s = s.substr(find.size());
	}
	return found;
}

/*static*/EventOptions EventOptions::parse(const string& spec, string& leftover) {
	leftover = spec;

	EventOptions options;

	while (true) {
		if (removeIfStartsWith(leftover, "backtrace:")) {
			options.backtrace = true;
		} else if (removeIfStartsWith(leftover, "nokernel:")) {
			options.exclude_kernel = true;
		} else {
			break;
		}
	}

	return options;
}

HardwarePerftoolsEventSource::HardwarePerftoolsEventSource(const string& event_spec, ::ProfileRecordCallback callback) :
	callback_(callback), events_enabled_(false) {
	options_ = EventOptions::parse(event_spec, event_spec_);
}

HardwareEventManager& HardwarePerftoolsEventSource::getEventManager() {
	if (!event_manager_) {
		// The system isn't yet bootstrapped enough for this to be safe :-(
		//		LOG(WARNING) << "Starting hardware event profiling.  Events = " << events;

		int format = 0;
		format |= PERF_SAMPLE_IP;
		format |= PERF_SAMPLE_TID;

		if (options_.backtrace) {
			format |= PERF_SAMPLE_CALLCHAIN;
		}

		//		format |= PERF_SAMPLE_TIME;
		//		format |= PERF_SAMPLE_PERIOD;
		//		format |= PERF_SAMPLE_CALLCHAIN;
		//	format |= PERF_SAMPLE_ID;
		//	format|= PERF_FORMAT_ID;

		SampleFormat sampleFormat(format);
		event_manager_.reset(new HardwareEventManager(sampleFormat));
	}

	return *event_manager_;
}

void HardwarePerftoolsEventSource::RegisterThread(int callback_count) {
	// We track threads automatically by hooking the pid
	//	pid_t tid = (pid_t) syscall(SYS_gettid);
	//	getEventManager().attachThread(tid);
}

void HardwarePerftoolsEventSource::StartBackgroundThread() {
	if (thread_) {
		FATAL("Background thread already running");
	}

	EventSet& eventSet = BuildEventSystem(event_spec_);

	thread_stop_ = false;

	pthread_t thread;
	if (pthread_create(&thread, nullptr, BackgroundThreadMain, this)) {
		FATAL("Cannot create timer thread");
	}

	thread_ = thread;

	eventSet.setEnabled(true);
}

EventSet& HardwarePerftoolsEventSource::BuildEventSystem(const string& events_arg) {
	// We gradually whittle down the events_arg as we parse it
	string events(events_arg);

	HardwareEventManager& eventManager = getEventManager();

	EventSetSpecifier eventSetSpec = EventSetSpecifier::parse(events);

	for (size_t j = 0; j < eventSetSpec.size(); j++) {
		EventSpecification& eventSpec = eventSetSpec[j];

		perf_event_attr& attr = eventSpec.attr();

		/* off by default        */
		attr.disabled = 1;
		/* next exec enables     */
		attr.enable_on_exec = 1;
		/* must always be on PMU */
		attr.pinned = 1;

		// This lets us profile with unpriviledged users
		// We only need this if /proc/sys/kernel/perf_event_paranoid == 2 (in single proc mode)
		// TODO: Detect when this is required?
		attr.exclude_kernel = options_.exclude_kernel ? 1 : 0;

		attr.sample_type = eventManager.format();

		//attr.freq = 1; /* use freq, not period  */
		//events_[i].hw_.sample_frequence = ???

		// trace fork/exit
		attr.task = 1;

		// inherit works in "one cpu, one pid" mode
		// inherit does not work in "all cpus, one tid" mode
		attr.inherit = 1;

		// TODO: Expose this in the event spec string??
		// (or compute a sensible default value??)
		//attr.sample_period = 100000;
		// By default, we ask the kernel to auto-tune to match our target
		// sample_freq is events per second i.e. Hz
		attr.freq = 1;
		attr.sample_freq = 1000;
	}

	unique_ptr<EventSet> eventSetPtr(new EventSet(eventSetSpec, CpuSet::buildEachCpu(), ThreadSet::buildSingleProcess(getpid())));

	EventSet& eventSet = eventManager.addEventSet(move(eventSetPtr));
	return eventSet;
}

class ProfilerEventSink: public EventSink {
public:
	ProfilerEventSink(SampleFormat format, ProfileRecordCallback callback) :
		EventSink(format), callback_(callback) {
	}

	virtual void HandleRecordSample(PerfEvent event) {
		DecodedPerfEvent decoded;
		event.decode(format(), decoded);

		if (decoded.callchain_size == 0) {
			uint64_t fake_backtrace[1];
			fake_backtrace[0] = decoded.ip;
			callback_(1, (void**) fake_backtrace, 1);
		} else {
			callback_(1, (void**) decoded.callchain, decoded.callchain_size);
		}
	}

private:
	ProfileRecordCallback callback_;
};

void * HardwarePerftoolsEventSource::BackgroundThreadMain(void * arg) {
	HardwarePerftoolsEventSource * instance = (HardwarePerftoolsEventSource*) arg;

	HardwareEventManager& eventManager = instance->getEventManager();

	ProfilerEventSink sink(eventManager.format(), instance->callback_);

	while (!instance->thread_stop_) {
		int timeout = 100;
		eventManager.poll(sink, timeout);
		//		LOG(INFO) << "Completed poll loop";
	}

	return 0;
}

void HardwarePerftoolsEventSource::StopBackgroundThread() {
	if (thread_) {
		thread_stop_ = true;
		if (pthread_join(thread_, NULL)) {
			LOG(FATAL) << "Cannot stop background thread " << errno;
		}
		thread_ = 0;
	}
	return;
}

void HardwarePerftoolsEventSource::Reset() {
	StopBackgroundThread();
}

void HardwarePerftoolsEventSource::RegisteredCallback(int new_callback_count) {
	// Start the timer if timer is shared and this is a first callback.
	if (new_callback_count == 1) {
		StartBackgroundThread();
	}
}

void HardwarePerftoolsEventSource::UnregisteredCallback(int new_callback_count) {
	if (new_callback_count == 0) {
		StopBackgroundThread();
	}
}

}
}
}

extern "C" {

using namespace fathomdb::perftools::hardware;

ProfileEventSource * google_perftools_extension_load_hardware(int32_t frequency, const char * extension_spec, ProfileRecordCallback callback) {
	string eventSpec(extension_spec);
	//LOG(WARNING) << "Doing hardware profiling with: " << eventSpec;
	return new HardwarePerftoolsEventSource(eventSpec, callback);
}

}
