// See COPYRIGHT for copyright
#ifndef EVENTPARSER_H_
#define EVENTPARSER_H_

#include <string>
#include <memory>

#include <linux/perf_event.h>

namespace fathomdb {
namespace perftools {
namespace hardware {

using namespace std;

class HardwareCacheEvent {
	// Hardware events are comprised of a type / op / result combo.

	// We normalize everything to lower case, and singular (because the events are singular).
	// The Linux "perf" code is a bit less rigid here
public:
	static HardwareCacheEvent parse(const string& s);

	string str();

	HardwareCacheEvent(perf_hw_cache_id cache, perf_hw_cache_op_id op = PERF_COUNT_HW_CACHE_OP_READ, perf_hw_cache_op_result_id result = PERF_COUNT_HW_CACHE_RESULT_ACCESS) :
		cache_(cache), op_(op), result_(result) {
	}

	void fillAttributes(perf_event_attr *attr);

private:
	static perf_hw_cache_op_result_id parseResult(const string& s);
	static perf_hw_cache_op_id parseOperation(const string& s);
	static perf_hw_cache_id parseCache(const string& s);

private:
	perf_hw_cache_id cache_;
	perf_hw_cache_op_id op_;
	perf_hw_cache_op_result_id result_;
};

class HardwareEvent {
	static const char * * names;
public:
	static unique_ptr<HardwareEvent> tryParse(const string& s);

	string str();

	HardwareEvent(perf_hw_id id) :
		id_(id) {
	}

	void fillAttributes(perf_event_attr *attr);

private:
	perf_hw_id id_;
};

class EventParser {
public:
	static void parseEvent(const string& eventName, perf_event_attr *attr);
};

}
}
}

#endif /* EVENTPARSER_H_ */
