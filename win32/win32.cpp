
#include <windows.h>
#include <time.h>

void sync(void) { return; }


#define TOFFSET (((LONGLONG)27111902 << 32) + (LONGLONG)3577643008)

static void ftconvert(const FILETIME *ft, struct timeval *ts);

int gettimeofday(struct timeval *tv, struct timezone *tz) {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ftconvert(&ft,tv);
	return 0;
}

static void ftconvert(const FILETIME *ft, struct timeval *ts) {
	ts->tv_sec = (int)((*(LONGLONG *)ft - TOFFSET) / 10000000);
	ts->tv_usec = (int)((*(LONGLONG *)ft - TOFFSET - ((LONGLONG)ts->tv_sec * (LONGLONG)10000000)) * 100) / 1000;
}

struct tm * newlocaltime(time_t *t) {
	if (*t < 0) {
		struct tm *temp;
		long tempnum = abs(_timezone) - (*t * -1);
		temp = localtime(&tempnum);
		if (!temp)
			return NULL;

		temp->tm_sec = temp->tm_sec - (abs(_timezone) % 60);
		temp->tm_min = temp->tm_min - (abs(_timezone) % 3600);
		temp->tm_hour = temp->tm_hour - (abs(_timezone) / 3600);
		return temp;
	} else {
		return localtime(t);
	}
}

