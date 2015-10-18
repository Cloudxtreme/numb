#ifndef MONITOREDHOST_H
#define MONITOREDHOST_H

#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>

class MonitoredHost {
public:
	MonitoredHost();
	~MonitoredHost();

	in_addr_t ipAddress;
	time_t timeout;
};

#endif

