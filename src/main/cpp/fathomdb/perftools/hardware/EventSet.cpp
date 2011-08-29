// See COPYRIGHT for copyright
#include "EventSet.h"

#include <stdint.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <glog/logging.h>

#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/perf_event.h>
#include "EventParser.h"

using namespace std;

// TODO: Is this declared anywhere sensible??
#include <asm/unistd.h>
#include "ThreadDiscovery.h"
#include "EventSink.h"

static inline int sys_perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

namespace fathomdb {
namespace perftools {
namespace hardware {

FileDescriptorPollList::FileDescriptorPollList(int max) :
	pollfd_array_(0), count_(0), max_(max) {
	pollfd_array_ = new pollfd[max_];
	memset(pollfd_array_, 0, sizeof(pollfd) * max_);
}

void FileDescriptorPollList::add(int fd) {
	fcntl(fd, F_SETFL, O_NONBLOCK);

	if (count_ >= max_)
		throw invalid_argument("Declared maximum number of polled file descriptors reached");

	pollfd& p = pollfd_array_[count_];
	p.fd = fd;
	p.events = POLLIN;
	p.revents = 0;

	count_++;
}

int FileDescriptorPollList::poll(int timeout) {
	int ret = ::poll(pollfd_array_, count_, timeout);
	if (ret >= 0)
		return ret;

	// Error
	perror("Error doing event poll");
	return -1;
}

EventSet::~EventSet() {
}

Event::Event(const EventSpecification& specification, cpuid_t cpu, pid_t tid) :
	specification_(specification), cpu_(cpu), tid_(tid), fd_(-1) {
	open();
}

EventSpecification::EventSpecification(const perf_event_attr& attr, const string& name) :
	attr_(attr), name_(name) {
}

/*static*/vector<cpuid_t> CpuSet::getAllCpusSimple() {
	// The perf subsystem parses the list of CPUs that are actually online.
	// That's way too hard for now.
	int cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
	if (cpuCount < 0)
		throw invalid_argument("Cannot determine number of CPUs in system");

	vector<cpuid_t> cpus;
	for (int i = 0; i < cpuCount; i++) {
		cpus.push_back(i);
	}
	return cpus;
}

/*static*/CpuSet CpuSet::all() {
	static vector<cpuid_t> allCpus;

	if (allCpus.empty()) {
		// We know there's at least one CPU :-)
		vector<cpuid_t> cpus = getAllCpusSimple();
		allCpus.swap(cpus);
	}

	return CpuSet(allCpus);
}

/*static*/ThreadSet ThreadSet::findThreadsInProcess(pid_t pid) {
	vector<pid_t> threads = LinuxThreadDiscovery::discoverThreads(pid);
	return ThreadSet(move(threads));
}

EventChannel& EventChannelSet::getChannel(key_t key) {
	unique_ptr<EventChannel>& channel = channels_[key];
	if (!channel) {
		channel.reset(new EventChannel(format_));
		keys_.push_back(key);
	}
	return *channel;
}

void EventChannel::add(int fd, FileDescriptorPollList& pollList) {
	if (mmap_) {
		joinMmap(fd, mmap_fd_);
	} else {
		doMmap(fd);
		pollList.add(fd);
	}
}

Event& EventSet::operator[](int index) {
	DCHECK_LT((size_t) index, events_.size())
			;
	DCHECK_GE(index, 0)
			;
	return *events_[index];
}

void Event::setEnabled(bool enable) {
	if (enable) {
		int ret = ioctl(fd_, PERF_EVENT_IOC_ENABLE, 0);
		if (ret)
			err(1, "cannot enable event %s\n", name().c_str());
	} else {
		int ret = ioctl(fd_, PERF_EVENT_IOC_DISABLE, 0);
		if (ret)
			err(1, "cannot disable event %s\n", name().c_str());
	}
}

void Event::readEvent(void *buf, size_t size) {
	ssize_t ret = read(fd_, buf, size);
	if (ret != ((ssize_t) size)) {
		if (ret == -1) {
			err(1, "cannot read event %s\n", name().c_str());
		} else {
			warnx("could not read event %s\n", name().c_str());
		}
	}
}

void Event::open() {
	CHECK_EQ(fd_, -1)
		;

	fd_ = sys_perf_event_open(&specification_.attr(), tid_, cpu_, -1, 0);

	if (fd_ == -1) {
		err(1, "cannot attach event to CPU%d %s", cpu_, name().c_str());
		throw invalid_argument("Error attaching event");
	}
}

/*static*/EventSetSpecifier EventSetSpecifier::parse(const string& eventSpec) {
	vector<EventSpecification> events;

	vector<string> eventNames;
	boost::algorithm::split(eventNames, eventSpec, boost::algorithm::is_any_of(","));

	for (size_t i = 0; i < eventNames.size(); i++) {
		const string& eventName = eventNames[i];

		perf_event_attr attr;
		memset(&attr, 0, sizeof(perf_event_attr));
		attr.size = sizeof(attr);

		EventParser::parseEvent(eventName, &attr);
		//		LOG(INFO) << "Parsed " << eventName << " => " << attr.type << ":" << attr.config;

		events.push_back(EventSpecification(attr, eventName));
	}

	return EventSetSpecifier(move(events));
}

void EventSet::setEnabled(bool enable) {
	for (auto it = events_.begin(); it != events_.end(); it++) {
		(*it)->setEnabled(enable);
	}
}

EventSet::EventSet(const EventSetSpecifier& eventSetSpec, const CpuSet& cpus) :
	cpus_(cpus), threads_(ThreadSet::empty()) {
	buildEvents(eventSetSpec);
}

EventSet::EventSet(const EventSetSpecifier& eventSetSpec, const ThreadSet& threads) :
	cpus_(CpuSet::empty()), threads_(threads) {
	buildEvents(eventSetSpec);
}

void EventSet::buildEvents(const EventSetSpecifier& eventSetSpec) {
	for (size_t j = 0; j < eventSetSpec.size(); j++) {
		const EventSpecification& eventSpec = eventSetSpec[j];

		if (cpus_.size() != 0) {
			CHECK(threads_.size() == 0);
			for (size_t i = 0; i < cpus_.size(); i++) {
				unique_ptr<Event> event(new Event(eventSpec, cpus_[i], -1));
				events_.push_back(move(event));
			}
		} else {
			CHECK(cpus_.size() == 0);
			for (size_t i = 0; i < threads_.size(); i++) {
				unique_ptr<Event> event(new Event(eventSpec, -1, threads_[i]));
				events_.push_back(move(event));
			}
		}
	}
}

EventChannel::EventChannel(SampleFormat format, bool overwrite, int pages) :
	overwrite_(overwrite), mmap_(0), mmap_fd_(-1), last_read_offset_(0), format_(format) {
	buffer_size_ = PAGE_SIZE * pages;
	buffer_mask_ = buffer_size_ - 1;
}

void EventChannel::doMmap(int fd) {
	CHECK_GE(fd, 0)
		;

	int prot = PROT_READ;
	if (overwrite_)
		prot |= PROT_WRITE;

	/* One extra page for the header*/
	size_t mmapSize = buffer_size_ + PAGE_SIZE;
	mmap_ = mmap(NULL, mmapSize, prot, MAP_SHARED, fd, 0);
	if (mmap_ == MAP_FAILED) {
		throw invalid_argument("Failed to mmap event source");
	}

	mmap_fd_ = fd;
}

void EventChannel::joinMmap(int fd, int joinToFd) {
	CHECK_GE(fd, 0)
		;
	CHECK_GE(joinToFd, 0)
		;

	if (ioctl(fd, PERF_EVENT_IOC_SET_OUTPUT, joinToFd) != 0)
		throw invalid_argument("Failed to join event source to mmap");
}

void EventChannel::readEvents(EventSink& sink) {
	while (true) {
		perf_event_header * event = read();
		if (!event)
			break;

		//			ret = perf_session__parse_sample(self, event, &sample);
		//			if (ret) {
		//				pr_err("Can't parse sample, err = %d\n", ret);
		//				continue;
		//			}
		//
		//			if (event->header.type == PERF_RECORD_SAMPLE)
		//				perf_event__process_sample(event, &sample, self);
		//			else
		//				perf_event__process(event, &sample, self);
		switch (event->type) {
		/*
		 * If perf_event_attr.sample_id_all is set then all event types will
		 * have the sample_type selected fields related to where/when
		 * (identity) an event took place (TID, TIME, ID, CPU, STREAM_ID)
		 * described in PERF_RECORD_SAMPLE below, it will be stashed just after
		 * the perf_event_header and the fields already present for the existing
		 * fields, i.e. at the end of the payload. That way a newer perf.data
		 * file will be supported by older perf tools, with these new optional
		 * fields being ignored.
		 *
		 * The MMAP events record the PROT_EXEC mappings so that we can
		 * correlate userspace IPs to code. They have the following structure:
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *
		 *	u32				pid, tid;
		 *	u64				addr;
		 *	u64				len;
		 *	u64				pgoff;
		 *	char				filename[];
		 * };
		 */
		case PERF_RECORD_MMAP:
			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_MMAP";
			break;

		case PERF_RECORD_LOST:
			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_LOST";
			break;

		case PERF_RECORD_COMM:
			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_COMM";
			break;

		case PERF_RECORD_EXIT:
			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_EXIT";
			break;

		case PERF_RECORD_THROTTLE:
			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_THROTTLE";
			break;

		case PERF_RECORD_UNTHROTTLE:
			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_UNTHROTTLE";
			break;

		case PERF_RECORD_FORK:
			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_FORK";
			break;

		case PERF_RECORD_READ:
			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_READ";
			break;

		case PERF_RECORD_SAMPLE: {
			//			LOG(INFO) << "Got unhandled record of type: PERF_RECORD_SAMPLE";
			PerfEvent sample(event);
			sink.HandleRecordSample(sample);
			break;
		}

		default:
			LOG(INFO) << "Got unhandled record of unknown type " << event->type;
			break;
		}
	}
}

perf_event_header * EventChannel::read() {
	perf_event_mmap_page * page = (perf_event_mmap_page*) mmap_;

	perf_event_header *event = 0;

	uint64_t head = page->data_head;
	barrier();

	if (last_read_offset_ != head) {
		char * circularBuffer = ((char*) mmap_) + PAGE_SIZE;

		size_t begin = last_read_offset_ & buffer_mask_;

		event = (perf_event_header*) &circularBuffer[begin];
		size_t eventSize = event->size;

		if (begin + eventSize > buffer_size_) {
			// The events can wrap around in the circular buffer
			// We therefore have to copy the event

			event_scratch_space_.reserve(eventSize);
			char * scratch = &event_scratch_space_[0];
			event = (perf_event_header*) scratch;

			size_t tailSize = buffer_size_ - begin;

			memcpy(scratch, event, tailSize);
			scratch += tailSize;

			memcpy(scratch, circularBuffer, eventSize - tailSize);
		}

		last_read_offset_ += eventSize;

		if (!overwrite_) {
			// We need to signal that we've read here...
			throw invalid_argument("overwrite not fully supported");
		}
	}

	return event;
}

}
}
}

