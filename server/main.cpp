#include "WebServer.h"

#include <cstdio>
#include <csignal>


WebServer webServer("0.0.0.0", 8080);

void signalHandler(int signum) {
	printf("Terminating Server\n");
	webServer.stop();
}

int main(int argc, char const *argv[]) {
	
	signal(SIGINT, signalHandler);

	if (webServer.init() != 0)
		return -1;

	if(webServer.run() < 0) return -1;

	return 0;
}