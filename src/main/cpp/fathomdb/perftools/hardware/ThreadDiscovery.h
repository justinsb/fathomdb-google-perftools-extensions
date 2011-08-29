// See COPYRIGHT for copyright
#ifndef THREADDISCOVERY_H_
#define THREADDISCOVERY_H_

#include <vector>
#include <sys/types.h>

namespace fathomdb {
namespace perftools {
namespace hardware {
using namespace std;

//class ThreadDiscovery {
//public:
//
//};

class LinuxThreadDiscovery {
public:
	static vector<pid_t> discoverThreads(pid_t pid);
};

}
}
}

#endif /* THREADDISCOVERY_H_ */
