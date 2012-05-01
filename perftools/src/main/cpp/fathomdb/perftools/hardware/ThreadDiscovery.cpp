// See COPYRIGHT for copyright
#include "ThreadDiscovery.h"

#include <stdexcept>
#include <sstream>
#include <vector>

#include <dirent.h>
#include <sys/types.h>
#include <glog/logging.h>

using namespace std;

namespace fathomdb {
namespace perftools {
namespace hardware {

/*static*/ vector<pid_t> LinuxThreadDiscovery::discoverThreads(pid_t pid) {
	struct dirent **namelist = nullptr;

	string dirname;
	{
		ostringstream s;
		s << "/proc/" << pid << "/task";
		dirname = s.str();
	}

	vector<pid_t> threads;

	// Grab all directory entries; no sort no filter
	// We do this in one 'big' call because we're touching /proc
	// (though who knows how this is implemented under the covers)
	int n = scandir(dirname.c_str(), &namelist, nullptr, nullptr);
	if (n < 0) {
		perror("Error in scandir of /proc/<pid>/task");
		throw invalid_argument("Could not read threads from /proc/<pid>/task");
	} else {
		// We must free everything, no matter what happens

		threads.reserve(n); // We'll leak if this fails

		while (n--) {
			const char * dirname = namelist[n]->d_name;
			if (*dirname != '.') {
				char * endptr;
				long tid = strtol(dirname, &endptr, 10);
				if (*endptr != '\0') {
					LOG(WARNING) << "Unexpected format for dirname: " << dirname;
				}
				threads.push_back(tid);
			}
			free(namelist[n]);
		}
		free(namelist);
	}

	return threads;
}

}
}
}
