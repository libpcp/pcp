
#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#include "default_config.h"
#endif

#ifndef HAVE_GETTIMEOFDAY
#include <windows.h>
//#include <winsock2.h>
#include <time.h>
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else /* defined(_MSC_VER) || defined(_MSC_EXTENSIONS)*/
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif /* defined(_MSC_VER) || defined(_MSC_EXTENSIONS)*/

/* custom implementation of the gettimeofday function
    for Windows platform. */

struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag = 0;
  int tz_seconds = 0;
  int tz_daylight = 0;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10;  /*convert into microseconds*/
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    if (_get_timezone(&tz_seconds)) {
        return -1;
    }
    if (_get_daylight(&tz_daylight)) {
        return -1;
    }
    tz->tz_minuteswest = tz_seconds / 60;
    tz->tz_dsttime = tz_daylight;
  }

  return 0;
}

#endif //HAVE_GETTIMEOFDAY
