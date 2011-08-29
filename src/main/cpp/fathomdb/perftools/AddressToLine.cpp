// See COPYRIGHT for copyright
#include "AddressToLine.h"
#include <iostream>
#include <sstream>
#include <glog/logging.h>
#include <unistd.h>
#include <boost/filesystem/path.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <boost/algorithm/string.hpp>

using namespace std;
using boost::filesystem::path;

namespace fathomdb {
namespace perftools {

extern string readWholeFile(const path& filePath, int reserve);

AddressToLine::AddressToLine() {
}

AddressToLine::~AddressToLine() {
}

enum PIPE_FILE_DESCRIPTERS {
	READ_FD = 0, WRITE_FD = 1
};

char **to_argv(const vector<string>& args) {
	size_t size = args.size();
	char **buf = new char*[size + 1];
	for (size_t i = 0; i < size; ++i) {
		buf[i] = new char[args[i].size() + 1];
		strcpy(buf[i], args[i].c_str());
	}
	buf[size] = NULL;
	return buf;
}

string AddressToLine::executePprof(const vector<string>& argv, const string& stdin) {
	string stdout;

	int parentToChild[2];
	int childToParent[2];
	pid_t pid;
	ssize_t readResult;
	int status;

	CHECK_EQ(0, pipe(parentToChild))
		;
	CHECK_EQ(0, pipe(childToParent))
		;

	switch (pid = fork()) {
	case -1: {
		perror("fork failed");
		throw invalid_argument("fork failed");
	}

	case 0: {
		CHECK_NE(-1, dup2(parentToChild[READ_FD], STDIN_FILENO))
			;
		CHECK_NE(-1, dup2(childToParent[WRITE_FD], STDOUT_FILENO))
			;
		CHECK_NE(-1, dup2(childToParent[WRITE_FD], STDERR_FILENO))
			;
		CHECK_EQ(0, close(parentToChild[WRITE_FD]))
			;
		CHECK_EQ(0, close(childToParent[READ_FD]))
			;

		char ** argv_cstr = to_argv(argv);
		execvp(argv_cstr[0], argv_cstr);

		perror("subprocess execution failed");
		CHECK(false)<< "subprocess execution failed";
	}
}

/*Parent*/
LOG(INFO) << "symbol mapping process starting (" << pid << ")";

CHECK_EQ(0, close(parentToChild[READ_FD]));
CHECK_EQ(0, close(childToParent[WRITE_FD]));

ssize_t done = 0;
ssize_t total = stdin.size();
const char * start = stdin.c_str();
while (done < total) {
	ssize_t wrote = write(parentToChild[WRITE_FD], start + done, total - done);
	if (wrote == -1) {
		perror("write to child failed");
		throw invalid_argument("Write to child failed");
	}
	done += wrote;
}

CHECK_EQ(0, close(parentToChild[WRITE_FD]));

bool exited = false;
while (!exited) {
	switch (readResult = read(childToParent[READ_FD], &buffer_[0], buffer_size_)) {
		case 0: { // End-of-File, or non-blocking read. */
			//				cout << "End of file reached..." << endl << "Data received was (" << dataReadFromChild.size() << "):" << endl << dataReadFromChild << endl;

			if (pid != waitpid(pid, &status, 0)) {
				perror("error waiting for subprocess to end");
				throw invalid_argument("error waiting for subprocess to end");
			}

			exited =true;

			if (WEXITSTATUS(status) != 0) {
				LOG(WARNING) << "subprocess returned error code " << WEXITSTATUS(status);
				throw invalid_argument("subprocess did not complete successfully");
			}

			LOG(INFO) << "symbol mapping process completed";

			//				cout << endl << "Child exit status is:  " << WEXITSTATUS(status) << endl << endl;
			//				exit(0);
			break;
		}

		case -1: {
			if ((errno == EINTR) || (errno == EAGAIN)) {
				errno = 0;
				break;
			} else {
				perror("Read from subprocess failed");
				throw invalid_argument("read from subprocess failed");
			}
		}

		default: {
			stdout.append(&buffer_[0], readResult);
			//			LOG(INFO) << "Read from child " << stdout;
			break;
		}
	}
}

return stdout;
}

vector<string> AddressToLine::mapAddressesToLines(const vector<string>& addresses) {
string stdin;

{
	ostringstream s;
	s << readWholeFile("/proc/self/maps", 128 * 1024);
	s << "\n";
	copy(addresses.begin(), addresses.end(), ostream_iterator<string>(s,"\n"));
	stdin = s.str();
}

string exe;
{
	ssize_t length = readlink("/proc/self/exe", &buffer_[0], buffer_size_);
	if (length >= buffer_size_) {
		throw invalid_argument("Path length was too long");
	}
	exe.assign(&buffer_[0], length);
}

string pprofCommand = "/usr/local/bin/pprof";

vector<string> argv;
{
	argv.push_back(pprofCommand);
	argv.push_back("--symbols");
	argv.push_back(exe);
}

//LOG(WARNING) << "Running command: " << argv[0];

string stdout = executePprof(argv, stdin);

vector<string> lines;
boost::algorithm::split(lines, stdout, boost::algorithm::is_any_of("\n"));

if (boost::algorithm::starts_with(lines[0], "Using local file ")) {
	lines.erase(lines.begin());
}

for (size_t i= 0; i < lines.size(); i++) {
	if (lines[i] == "") {
		lines.erase(lines.begin() + i);
		i--;
	}
}

if (lines.size() != addresses.size()) {
	throw invalid_argument("lines and addresses arrays were not of same size");
}

return lines;
}

}
}
