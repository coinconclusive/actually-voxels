#include <av/av.hh>
#include <sys/stat.h>

namespace av::fs {
	size_t GetFileSize(const char *fname) {
		struct stat st;
		stat(fname, &st);
		return st.st_size;
	}

	size_t GetFileSize(FILE *fptr) {
		struct stat st;
		fstat(fileno(fptr), &st);
		return st.st_size;
	}
}
