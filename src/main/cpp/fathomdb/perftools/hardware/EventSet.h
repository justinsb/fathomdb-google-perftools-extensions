// See COPYRIGHT for copyright
#ifndef EVENTSET_H_
#define EVENTSET_H_

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <utility>

#include <linux/perf_event.h>
#include "SampleFormat.h"

struct pollfd;

namespace fathomdb {
namespace perftools {
namespace hardware {
class EventSink;

using namespace std;

typedef int cpuid_t;


class EventSpecification {
public:
	string name() const {
		return name_;
	}

	perf_event_attr& attr() {
		return attr_;
	}

	EventSpecification(const perf_event_attr& attr, const string& name);

private:
	perf_event_attr attr_;
	string name_;
};

class Event {
public:
	void readEvent(void * buffer, size_t size);

	Event(const EventSpecification& specification, int cpu, pid_t tid);

	string name() const {
		return specification_.name();
	}

	int fileDescriptor() const {
		return fd_;
	}

	cpuid_t cpu() const {
		return cpu_;
	}

	pid_t tid() const {
		return tid_;
	}

	void setEnabled(bool enable);
private:
	EventSpecification specification_;
	cpuid_t cpu_;
	pid_t tid_;
	int fd_;

private:
	friend class EventSet;
	void open();
};

class EventChannel {
	static const int DEFAULT_PAGE_COUNT = 128;
	static const int PAGE_SIZE = 4096;

	/**
	 * Data is stored in a circular buffer, which we mmap.
	 * We can choose the size of the buffer to mmap.
	 * We map an additional page, which is is the 'header'
	 * The header is at the start of that page, and is of type perf_event_mmap_page.
	 * More details are in the declaration of perf_event_mmap_page.
	 */

public:
	EventChannel(SampleFormat format, bool overwrite = true, int pages = DEFAULT_PAGE_COUNT);

	void add(int fd, class FileDescriptorPollList& pollList);

	int readEvents(EventSink& sink);

private:
	void doMmap(int fd);

	void joinMmap(int fd, int joinToFd);

	void barrier() {
		// perf_event says we need an "rmb" after reading
		// (read memory barrier, I guess)

		// TODO: we're doing a full memory barrier here, we only need a read
		__sync_synchronize();
	}

	perf_event_header * read();

private:
	bool overwrite_;
	void * mmap_;
	int mmap_fd_;
	uint64_t last_read_offset_;

	size_t buffer_size_;
	size_t buffer_mask_;

	SampleFormat format_;

	string event_scratch_space_;
};

class EventChannelSet {
public:
	typedef pair<cpuid_t, pid_t> key_t;

public:
	EventChannelSet(SampleFormat format) :
		format_(format) {
	}

	EventChannel& getChannel(key_t key);

	const vector<key_t>& keys() const {
		return keys_;
	}

private:
	// TODO: Use AssocVector?
	// Note: We switched to map because hash wasn't defined on the pair (?)
	map<key_t, unique_ptr<EventChannel> > channels_;

	vector<key_t> keys_;
	SampleFormat format_;
};

class CpuSet {
public:
	static CpuSet buildEachCpu();
	static CpuSet buildWildcard();

	const vector<cpuid_t>& cpus() const {
		return cpus_;
	}

	size_t size() const {
		return cpus_.size();
	}

	cpuid_t operator[](int index) const {
		return cpus_[index];
	}

private:
	CpuSet(const vector<cpuid_t>& cpus) :
		cpus_(cpus) {
	}

	CpuSet() {}

	static vector<cpuid_t> getAllCpusSimple();
private:
	vector<cpuid_t> cpus_;
};


class ThreadSet {
public:
	ThreadSet() {}

	ThreadSet(const vector<pid_t>& threads) :
		threads_(threads) {}

	ThreadSet(vector<pid_t>&& threads) :
		threads_(move(threads)) {}

	const vector<pid_t>& threads() const {
		return threads_;
	}

	size_t size() const {
		return threads_.size();
	}

	int operator[](int index) const {
		return threads_[index];
	}

//	static ThreadSet findThreadsInProcess(pid_t pid);
	static ThreadSet buildSingleProcess(pid_t pid);
	static ThreadSet buildWildcard();

private:
	vector<pid_t> threads_;
};

class EventSetSpecifier {
public:
	EventSetSpecifier(const vector<EventSpecification>& events) :
		events_(events) {
	}

	EventSetSpecifier(vector<EventSpecification> && events) :
	events_(move(events)) {
	}

	size_t size() const {
		return events_.size();
	}

	const EventSpecification& operator[](int index) const {
		return events_[index];
	}

	EventSpecification& operator[](int index) {
		return events_[index];
	}

	static EventSetSpecifier parse(const string& eventSpec);

private:
	vector<EventSpecification> events_;
};

class EventSet {
	vector<unique_ptr<Event> > events_;
	CpuSet cpus_;
	ThreadSet threads_;
	EventSetSpecifier event_spec_;

public:
	EventSet(const EventSetSpecifier& specifier, const CpuSet& cpus, const ThreadSet& threads);
	~EventSet();

	Event& operator[](int index);
	size_t size() const {
		return events_.size();
	}

	void setEnabled(bool enable);

//	Event& attachThread(pid_t tid);

private:
	void buildEvents(const EventSetSpecifier& eventSetSpec);
};

class FileDescriptorPollList {
public:
	FileDescriptorPollList(int max);
	void add(int fd);

	int poll(int timeout = 100);

private:
	pollfd * pollfd_array_;
	int count_;
	int max_;
};

}
}
}

#endif /* EVENTSET_H_ */
