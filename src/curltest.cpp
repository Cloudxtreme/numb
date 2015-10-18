#include "../toolkit/curl.h"
#include "../toolkit/log.h"

LogError *systemLog;

int main(int argc, char **argv) {
	Curl *curl = new Curl();
	CurlSession *curlSession;


	systemLog = new LogError("curl");
 	systemLog->openFile("./curltest.log");
	systemLog->setOutput(LOCALFILE);
	systemLog->sysLog(ERROR, "create session");
	curlSession = curl->createSession();
	curl->fetchHttpUrl(curlSession, "blah.html", "http://www.google.com");

	return 0;
}
