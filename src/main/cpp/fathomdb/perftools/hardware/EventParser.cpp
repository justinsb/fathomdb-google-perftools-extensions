// See COPYRIGHT for copyright
#include "EventParser.h"
#include <stdexcept>
#include <string>
#include <glog/logging.h>

using namespace std;

namespace fathomdb {
namespace perftools {
namespace hardware {

void HardwareCacheEvent::fillAttributes(perf_event_attr *attr) {
	CHECK_EQ(attr->size, sizeof(perf_event_attr))
		;

	attr->type = PERF_TYPE_HW_CACHE;
	attr->config = cache_ | (op_ << 8) | (result_ << 16);
}

/*static*/HardwareCacheEvent HardwareCacheEvent::parse(const string& s) {
	string cache;
	string op;
	string result;

	const char * p = s.c_str();
	while (*p) {
		char c = *p;
		p++;

		if (c == '-') {
			// Sadly, l1-icache and l1-dcache "break the dash"
			// If we're in this case, just skip over the dash
			if (cache == "l1")
				continue;
			else
				break;
		}

		cache.append(1, tolower(c));
	}

	while (*p) {
		char c = *p;
		p++;

		if (c == '-') {
			break;
		}

		op.append(1, tolower(c));
	}

	while (*p) {
		char c = *p;
		p++;

		if (c == '-') {
			break;
		}

		result.append(1, tolower(c));
	}

	return HardwareCacheEvent(parseCache(cache), parseOperation(op), parseResult(result));
}

/*static*/perf_hw_cache_op_result_id HardwareCacheEvent::parseResult(const string& s) {
	if (s == "" || s == "access" || s == "accesses") {
		return PERF_COUNT_HW_CACHE_RESULT_ACCESS;
	}

	if (s == "miss" || s == "misses") {
		return PERF_COUNT_HW_CACHE_RESULT_MISS;
	}

	string message("Unknown hardware cache operation result: ");
	message.append(s);
	throw invalid_argument(message);
}

/*static*/perf_hw_cache_op_id HardwareCacheEvent::parseOperation(const string& s) {
	if (s == "load" || s == "loads" || s == "read" || s == "reads" || s == "") {
		return PERF_COUNT_HW_CACHE_OP_READ;
	}

	if (s == "stores" || s == "stores" || s == "write" || s == "writes") {
		return PERF_COUNT_HW_CACHE_OP_WRITE;
	}

	if (s == "prefetch" || s == "prefetches") {
		return PERF_COUNT_HW_CACHE_OP_PREFETCH;
	}

	string message("Unknown hardware cache operation: ");
	message.append(s);
	throw invalid_argument(message);
}

/*static*/perf_hw_cache_id HardwareCacheEvent::parseCache(const string& s) {
	/*
	 * Generalized hardware cache events:
	 *
	 *       { L1-D, L1-I, LLC, ITLB, DTLB, BPU } x
	 *       { read, write, prefetch } x
	 *       { accesses, misses }
	 */

	if (s == "l1-dcache" || s == "l1-d" || s == "l1dcache" || s == "l1d") {
		return PERF_COUNT_HW_CACHE_L1D;
	}

	if (s == "l1-icache" || s == "l1-i" || s == "l1icache" || s == "l1i") {
		return PERF_COUNT_HW_CACHE_L1I;
	}

	if (s == "llc") {
		return PERF_COUNT_HW_CACHE_LL;
	}

	if (s == "dtlb") {
		return PERF_COUNT_HW_CACHE_DTLB;
	}

	if (s == "itlb") {
		return PERF_COUNT_HW_CACHE_ITLB;
	}

	if (s == "branch" || s == "bpu") {
		return PERF_COUNT_HW_CACHE_BPU;
	}

	string message("Unknown hardware cache: ");
	message.append(s);
	throw invalid_argument(message);
}

string HardwareCacheEvent::str() {
	string s;

	switch (cache_) {
	case PERF_COUNT_HW_CACHE_L1D:
		s.append("l1-dcache");
		break;

	case PERF_COUNT_HW_CACHE_L1I:
		s.append("l1-icache");
		break;

	case PERF_COUNT_HW_CACHE_LL:
		s.append("llc");
		break;

	case PERF_COUNT_HW_CACHE_DTLB:
		s.append("dtlb");
		break;

	case PERF_COUNT_HW_CACHE_ITLB:
		s.append("itlb");
		break;

	case PERF_COUNT_HW_CACHE_BPU:
		s.append("branch");
		break;

	default:
		throw invalid_argument("Unknown hardware cache");
	}

	switch (op_) {
	case PERF_COUNT_HW_CACHE_OP_READ:
		s.append("-load");
		break;

	case PERF_COUNT_HW_CACHE_OP_WRITE:
		s.append("-store");
		break;

	case PERF_COUNT_HW_CACHE_OP_PREFETCH:
		s.append("-prefetch");
		break;
	default:
		throw invalid_argument("Unknown hardware cache event");
	}

	switch (result_) {
	case PERF_COUNT_HW_CACHE_RESULT_ACCESS:
		s.append("-access");
		break;

	case PERF_COUNT_HW_CACHE_RESULT_MISS:
		s.append("-miss");
		break;

	default:
		throw invalid_argument("Unknown hardware cache event result");
	}
	return s;
}

void HardwareEvent::fillAttributes(perf_event_attr *attr) {
	CHECK_EQ(attr->size, sizeof(perf_event_attr))
		;

	attr->type = PERF_TYPE_HARDWARE;
	attr->config = id_;
}

/*static*/unique_ptr<HardwareEvent> HardwareEvent::tryParse(const string& s) {
	// PERF_COUNT_HW_CPU_CYCLES
	if (s == "cpu-cycles" || s == "cycles") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_CPU_CYCLES));
		return event;
	}

	// PERF_COUNT_HW_INSTRUCTIONS
	if (s == "instructions") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_INSTRUCTIONS));
		return event;
	}

	// PERF_COUNT_HW_CACHE_REFERENCES
	if (s == "cache-references") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_CACHE_REFERENCES));
		return event;
	}

	// PERF_COUNT_HW_CACHE_MISSES
	if (s == "cache-misses") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_CACHE_MISSES));
		return event;
	}

	// PERF_COUNT_HW_BRANCH_INSTRUCTIONS
	if (s == "branch-instructions" || s == "branches") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_BRANCH_INSTRUCTIONS));
		return event;
	}

	// PERF_COUNT_HW_BRANCH_MISSES
	if (s == "branch-misses") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_BRANCH_MISSES));
		return event;
	}

	// PERF_COUNT_HW_BUS_CYCLES
	if (s == "bus-cycles") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_BUS_CYCLES));
		return event;
	}

	// PERF_COUNT_HW_STALLED_CYCLES_FRONTEND
	if (s == "stalled-cycles-backend" || s == "idle-cycles-frontend") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_STALLED_CYCLES_FRONTEND));
		return event;
	}

	// PERF_COUNT_HW_STALLED_CYCLES_BACKEND
	if (s == "stalled-cycles-backend" || s == "idle-cycles-backend") {
		unique_ptr<HardwareEvent> event(new HardwareEvent(PERF_COUNT_HW_STALLED_CYCLES_BACKEND));
		return event;
	}

	return nullptr;
}

/*static*/unique_ptr<SoftwareEvent> SoftwareEvent::tryParse(const string& s) {
	//	cpu-clock                                          [Software event]
	//	  task-clock                                         [Software event]
	//	  page-faults OR faults                              [Software event]
	//	  minor-faults                                       [Software event]
	//	  major-faults                                       [Software event]
	//	  context-switches OR cs                             [Software event]
	//	  cpu-migrations OR migrations                       [Software event]
	//	  alignment-faults                                   [Software event]
	//	  emulation-faults                                   [Software event]


	if (s == "cpu-clock") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_CPU_CLOCK));
		return event;
	}

	if (s == "task-clock") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_TASK_CLOCK));
		return event;
	}

	if (s == "page-faults" || s == "faults") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_PAGE_FAULTS));
		return event;
	}

	if (s == "context-switches" || s == "cs") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_CONTEXT_SWITCHES));
		return event;
	}

	if (s == "cpu-migrations" || s == "migrations") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_CPU_MIGRATIONS));
		return event;
	}

	if (s == "minor-faults") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_PAGE_FAULTS_MIN));
		return event;
	}

	if (s == "major-faults") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_PAGE_FAULTS_MAJ));
		return event;
	}

	if (s == "alignment-faults") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_ALIGNMENT_FAULTS));
		return event;
	}

	if (s == "emulation-faults") {
		unique_ptr<SoftwareEvent> event(new SoftwareEvent(PERF_COUNT_SW_EMULATION_FAULTS));
		return event;
	}

	return nullptr;
}

void SoftwareEvent::fillAttributes(perf_event_attr *attr) {
	CHECK_EQ(attr->size, sizeof(perf_event_attr))
		;

	attr->type = PERF_TYPE_SOFTWARE;
	attr->config = id_;
}

void EventParser::parseEvent(const string& eventName, perf_event_attr *attr) {
	{
		unique_ptr<HardwareEvent> event = HardwareEvent::tryParse(eventName);
		if (event) {
			event->fillAttributes(attr);
			return;
		}
	}

	{
		unique_ptr<SoftwareEvent> event = SoftwareEvent::tryParse(eventName);
		if (event) {
			event->fillAttributes(attr);
			return;
		}
	}

	{
		HardwareCacheEvent event = HardwareCacheEvent::parse(eventName);
		event.fillAttributes(attr);
	}
}

/*
 class PfmParser {
 static void init() {
 int err;
 if (0 != (err = pfm_initialize())) {
 throw invalid_argument("Error initializing libpfm");
 }
 }
 static void parseEvent() {
 int ret = pfm_get_perf_event_encoding(eventName.c_str(), PFM_PLM3, &attr, NULL, NULL);
 if (ret != PFM_SUCCESS) {
 warnx("event %s: %s\n", eventName.c_str(), pfm_strerror(ret));
 }
 }
 };
 */

}
}
}
