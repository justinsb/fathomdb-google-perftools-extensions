// See COPYRIGHT for copyright
#include "TestFunctions.h"

#include <string>
#include <algorithm>

#include <glog/logging.h>

#include "HardwareEventManager.h"
#include "SampleFormat.h"
#include "EventSink.h"

using namespace fathomdb::perftools::hardware;
using namespace std;

void TestHardwarePerformanceEvents() {
	string events("cpu-cycles,instructions");

	int format = 0;
	format |= PERF_SAMPLE_IP;
	format |= PERF_SAMPLE_TID;
	format |= PERF_SAMPLE_TIME;
	format |= PERF_SAMPLE_PERIOD;
	format |= PERF_SAMPLE_CALLCHAIN;
	//	format |= PERF_SAMPLE_ID;
	//	format|= PERF_FORMAT_ID;

	SampleFormat sampleFormat(format);
	HardwareEventManager eventSystem(sampleFormat);

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
		// attr.exclude_kernel = 1;

		attr.sample_type = eventSystem.format();

		//attr.freq = 1; /* use freq, not period  */
		//events_[i].hw_.sample_frequence = ???

		attr.freq = 0;
		attr.sample_period = 100000;
	}

	//	unique_ptr<EventSet> eventSetPtr(new EventSet(eventSetSpec, CpuSet::all()));
	unique_ptr<EventSet> eventSetPtr(new EventSet(eventSetSpec, CpuSet::buildEachCpu(), ThreadSet::buildSingleProcess(getpid())));

	EventSet& eventSet = eventSystem.addEventSet(move(eventSetPtr));

	eventSet.setEnabled(true);

	vector<int> dummy;
	for (int i = 0; i < 1000000; i++) {
		dummy.push_back(i);
	}

	DumpingEventSink sink(eventSystem.format());

	while (true) {
		int timeout = 100;
		eventSystem.poll(sink, timeout);
		LOG(INFO) << "Completed poll loop";

		sort(dummy.begin(), dummy.end());
	}
}

void TestGoogleProfiler() {
	for (int j = 0; j < 100; j++) {
		vector<int> dummy;
		for (int i = 0; i < 1000000; i++) {
			dummy.push_back(i);
		}

		sort(dummy.begin(), dummy.end());
		LOG(INFO) << "Completed sort " << j;
	}
}
