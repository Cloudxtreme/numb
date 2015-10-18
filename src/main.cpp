#include "streamer.h"
#include "../toolkit/log.h"

LogError *systemLog;
//const char *_malloc_options = "AX";

int main(int argc, char **argv) {
	Streamer *streamer = new Streamer();

	streamer->main(argc, argv);

	pthread_exit(NULL);
}
