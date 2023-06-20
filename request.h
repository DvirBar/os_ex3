#ifndef __REQUEST_H__

struct ThreadStats_t{
    int tid;
    int reqCount;
    int staticReqCount;
    int dynamicReqCount;
};

typedef  struct ThreadStats_t* ThreadStats;

struct Stats_t {
    struct timeval arrivalTime;
    struct timeval dispatchInterval;
};

typedef struct Stats_t* Stats;

void requestHandle(int fd, Stats stats, ThreadStats tstats);

#endif
