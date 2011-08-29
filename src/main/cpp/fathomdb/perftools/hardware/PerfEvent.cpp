// See COPYRIGHT for copyright
#include "PerfEvent.h"
#include <glog/logging.h>
#include <stdint.h>
#include <sstream>

using namespace std;

namespace fathomdb {
namespace perftools {
namespace hardware {

/*
 * struct {
 *	struct perf_event_header	header;
 *
 *	{ u64			ip;	  } && PERF_SAMPLE_IP
 *	{ u32			pid, tid; } && PERF_SAMPLE_TID
 *	{ u64			time;     } && PERF_SAMPLE_TIME
 *	{ u64			addr;     } && PERF_SAMPLE_ADDR
 *	{ u64			id;	  } && PERF_SAMPLE_ID
 *	{ u64			stream_id;} && PERF_SAMPLE_STREAM_ID
 *	{ u32			cpu, res; } && PERF_SAMPLE_CPU
 *	{ u64			period;   } && PERF_SAMPLE_PERIOD
 *
 *	{ struct read_format	values;	  } && PERF_SAMPLE_READ
 *
 *	{ u64			nr,
 *	  u64			ips[nr];  } && PERF_SAMPLE_CALLCHAIN
 *
 *	#
 *	# The RAW record below is opaque data wrt the ABI
 *	#
 *	# That is, the ABI doesn't make any promises wrt to
 *	# the stability of its content, it may vary depending
 *	# on event, hardware, kernel version and phase of
 *	# the moon.
 *	#
 *	# In other words, PERF_SAMPLE_RAW contents are not an ABI.
 *	#
 *
 *	{ u32			size;
 *	  char                  data[size];}&& PERF_SAMPLE_RAW
 * };
 */

void PerfEvent::dump(SampleFormat flags) {
	DecodedPerfEvent event;
	decode(flags, event);

	ostringstream s;

	uint8_t * p = (uint8_t *) header_;
	p += sizeof(perf_event_header);

	s << "PERF_RECORD_SAMPLE";
	// *	{ u64			ip;	  } && PERF_SAMPLE_IP
	if (flags.checkFlag(PERF_SAMPLE_IP)) {
		s << " ip: " << hex << event.ip;
	}

	// *	{ u32			pid, tid; } && PERF_SAMPLE_TID
	if (flags.checkFlag(PERF_SAMPLE_TID)) {
		s << " pid: " << dec << event.pid;
		s << " tid: " << event.tid;
	}

	// *	{ u64			time;     } && PERF_SAMPLE_TIME
	if (flags.checkFlag(PERF_SAMPLE_TIME)) {
		s << " time: " << dec << event.time;
	}

	//*	{ u64			addr;     } && PERF_SAMPLE_ADDR
	if (flags.checkFlag(PERF_SAMPLE_ADDR)) {
		s << " addr: " << hex << event.addr;
	}

	//*	{ u64			id;	  } && PERF_SAMPLE_ID
	if (flags.checkFlag(PERF_SAMPLE_ID)) {
		s << " id: " << hex << event.sample_id;
	}

	//*	{ u64			stream_id;} && PERF_SAMPLE_STREAM_ID
	if (flags.checkFlag(PERF_SAMPLE_STREAM_ID)) {
		s << " stream: " << hex << event.stream_id;
	}

	//*	{	u32 cpu, res;}&& PERF_SAMPLE_CPU
	if (flags.checkFlag(PERF_SAMPLE_CPU)) {
		s << " cpu: " << dec << event.cpu;
		s << " res: " << event.res;
	}

	//	* {	u64 period;}&& PERF_SAMPLE_PERIOD
	if (flags.checkFlag(PERF_SAMPLE_PERIOD)) {
		s << " period: " << dec << event.period;
	}

	//	* {	struct read_format values;}&& PERF_SAMPLE_READ
	// TODO: ????

	//	* {	u64 nr,
	//		* u64 ips[nr];}&& PERF_SAMPLE_CALLCHAIN
	if (flags.checkFlag(PERF_SAMPLE_CALLCHAIN)) {
		// Without -fno-omit-frame-pointer, you're not going to get more than the top item
		// This is really painful because the kernel & libraries aren't compiled with -fno-omit-frame-pointer

		s << " callchain:" << hex;
		for (uint64_t i = 0; i < event.callchain_size; i++) {
			// There seems to always be a fake entry first: fffffffffffffe00
			s << " " << event.callchain[i];
		}
	}

	LOG(WARNING) << "Record: " << s.str();
}

void PerfEvent::decode(SampleFormat flags, DecodedPerfEvent& decoded) {
	uint8_t * p = (uint8_t *) header_;
	p += sizeof(perf_event_header);

	// *	{ u64			ip;	  } && PERF_SAMPLE_IP
	if (flags.checkFlag(PERF_SAMPLE_IP)) {
		decoded.ip = *((uint64_t*) p);
		p += 8;
	}

	// *	{ u32			pid, tid; } && PERF_SAMPLE_TID
	if (flags.checkFlag(PERF_SAMPLE_TID)) {
		decoded.pid = *((uint32_t*) p);
		p += 4;
		decoded.tid = *((uint32_t*) p);
		p += 4;
	}

	// *	{ u64			time;     } && PERF_SAMPLE_TIME
	if (flags.checkFlag(PERF_SAMPLE_TIME)) {
		decoded.time = *((uint64_t*) p);
		p += 8;
	}

	//*	{ u64			addr;     } && PERF_SAMPLE_ADDR
	if (flags.checkFlag(PERF_SAMPLE_ADDR)) {
		decoded.addr = *((uint64_t*) p);
		p += 8;
	}

	//*	{ u64			id;	  } && PERF_SAMPLE_ID
	if (flags.checkFlag(PERF_SAMPLE_ID)) {
		decoded.sample_id = *((uint64_t*) p);
		p += 8;
	}

	//*	{ u64			stream_id;} && PERF_SAMPLE_STREAM_ID
	if (flags.checkFlag(PERF_SAMPLE_STREAM_ID)) {
		decoded.stream_id = *((uint64_t*) p);
		p += 8;
	}

	//*	{	u32 cpu, res;}&& PERF_SAMPLE_CPU
	if (flags.checkFlag(PERF_SAMPLE_CPU)) {
		decoded.cpu = *((uint32_t*) p);
		p += 4;
		decoded.res = *((uint32_t*) p);
		p += 4;
	}

	//	* {	u64 period;}&& PERF_SAMPLE_PERIOD
	if (flags.checkFlag(PERF_SAMPLE_PERIOD)) {
		decoded.period = *((uint64_t*) p);
		p += 8;
	}

	//	* {	struct read_format values;}&& PERF_SAMPLE_READ
	// TODO: ????

	//	* {	u64 nr,
	//		* u64 ips[nr];}&& PERF_SAMPLE_CALLCHAIN

	// It can be painful if we don't zero this one out
	decoded.callchain_size = 0;

	if (flags.checkFlag(PERF_SAMPLE_CALLCHAIN)) {
		// Without -fno-omit-frame-pointer, you're not going to get more than the top item
		// This is really painful because the kernel & libraries aren't compiled with -fno-omit-frame-pointer

		decoded.callchain_size = *((uint64_t*) p);
		p += 8;
		if (decoded.callchain_size != 0) {
			decoded.callchain = ((uint64_t*) p);

			p += sizeof(uint64_t) * decoded.callchain_size;
		}
	}
}

}
}
}
