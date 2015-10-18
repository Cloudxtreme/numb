

#include "server.h"
#include <pthread.h>
#include <netinet/tcp.h>
#include <fcntl.h>

Server::Server(int type, int protocol, int port, int maxqueue) {
	openSocket(type, protocol, port, maxqueue);
	numberOfConnections = 0;
  
	return;
}

Server::Server(int port, int maxqueue) {
	openSocket(SOCK_STREAM, IPPROTO_IP, port, maxqueue);
	numberOfConnections = 0;

	return;
}

Server::Server(int type, int protocol, int port) {
	openSocket(type, protocol, port, 0);
	numberOfConnections = 0;

	return;
}

Server::Server(int port) {
	openSocket(SOCK_DGRAM, IPPROTO_IP, port, 0);
	numberOfConnections = 0;

	return;
}

Server::~Server(void) {
	closeSocket(sd);

	return;
}

int Server::openSocket(int type, int protocol, int port, int maxqueue) {
	int vrai = 1;
	struct sockaddr_in addr;

	sd = -1;
	if (maxqueue)
		this->maxqueue = maxqueue;
	bzero(&addr, sizeof(addr));
	sd = socket(PF_INET, type, protocol);
	if (sd < 0) {
		systemLog->sysLog(ERROR, "impossible to open a socket for hearing connections... cancel: %s", strerror(errno));
		return -1;
	}
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &vrai, sizeof(vrai));
	setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &vrai, sizeof(vrai));
	setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &vrai, sizeof(vrai));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		systemLog->sysLog(ERROR, "impossible to bind on the socket... cancel: %s", strerror(errno));
		return -1;
	}

	return -1;
}

int Server::setSendBuffer(int bufferSize) {
	int returnCode;

	returnCode = setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] cannot set the send buffer on socket: %s", sd, strerror(errno));
		return -1;
	}

	return 0;
}

int Server::getSendBuffer() {
	int bufferSize;
	int returnCode;
	socklen_t bufferLen;

	returnCode = getsockopt(sd, SOL_SOCKET, SO_SNDBUF, &bufferSize, &bufferLen);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] cannot get send buffer on socket: %s", sd, strerror(errno));
		return -1;
	}

	return bufferSize;
}

int Server::setRecvBuffer(int bufferSize) {
	int returnCode;

	returnCode = setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] cannot set the receive buffer on socket: %s", sd, strerror(errno));
		return -1;
	}

	return 0;
}

#ifdef ACCEPTFILTER
int Server::setAcceptFilterHttp(void) {
	struct accept_filter_arg afa;
	int returnCode;
	
	bzero(&afa, sizeof(afa));
	strcpy(afa.af_name, "httpready");
	returnCode = setsockopt(sd, SOL_SOCKET, SO_ACCEPTFILTER, &afa, sizeof(afa));
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] cannot set accept filter HTTP on socket: %s", sd, strerror(errno));
		return -1;
	}

	return 0;
}
#else
int Server::setAcceptFilterHttp(void) {
	systemLog->sysLog(ERROR, "[%d] accept filter HTTP is not available on this OS (SO_ACCEPTFILTER)", sd);

	return 0;
}
#endif

int Server::setMinimumToSend(int bufferSize) {
	int returnCode;

	returnCode = setsockopt(sd, SOL_SOCKET, SO_SNDLOWAT, &bufferSize, sizeof(bufferSize));
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] cannot set the minimum data to send on socket: %s", sd, strerror(errno));
		return -1;
	}
	sendLoWat = bufferSize;

	return 0;
}

int Server::setMinimumToSend(int sdclient, int bufferSize) {
	int returnCode;

	returnCode = setsockopt(sdclient, SOL_SOCKET, SO_SNDLOWAT, &bufferSize, sizeof(bufferSize));
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] cannot set the minimum data to send on socket: %s", sdclient, strerror(errno));
		return -1;
	}
	sendLoWat = bufferSize;

	return 0;
}

void Server::setNumberOfConnections(int _numberOfConnections) {
	numberOfConnections = _numberOfConnections;
}

int Server::getNumberOfConnections(void) {
	return numberOfConnections;
}

void Server::addAConnection(void) {
	numberOfConnections++;

	return;
}

void Server::deleteAConnection(void) {
	numberOfConnections--;

	return;
}

int Server::closeSocket(int sdclient) {
	close(sdclient);
	deleteAConnection();

	return 0;
}

int Server::listenSocket() {
#ifdef DEBUGSERVER
	systemLog->sysLog(DEBUG, "listen on server socket %d maxqueue", maxqueue);
#endif
	if (listen(sd, maxqueue) < 0) {
		systemLog->sysLog(LOG_INFO, "cannot listen on the socket: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int Server::acceptSocket(struct sockaddr_in *sa) {
	socklen_t sasize = sizeof(struct sockaddr_in);
	int sdclient;

	if (sd < 0) {
		systemLog->sysLog(ERROR, "you must call the Server::Server constructor before using Server::Listen");
		return -1;
	}
	sdclient = accept(sd, (struct sockaddr *)sa, &sasize);
	if (sdclient < 0) {
		if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
			return -1;
		systemLog->sysLog(ERROR, "error during accept of the connection: %s %d", strerror(errno), sdclient);
		return -2;
	}
#ifdef DEBUGSERVER
	systemLog->sysLog(LOG_INFO, "connection accepted from < %s:%u >", inet_ntoa(sa->sin_addr), sa->sin_port);
#endif
	addAConnection();

	return sdclient;
}

int Server::waitMessage(int sdc, int timeout)
{
	struct pollfd pfd;

	pfd.fd      = sdc;
	pfd.events  = POLLIN;
	pfd.revents = 0;

	return poll(&pfd, 1, timeout);
}

int Server::keepaliveSocket()
{
	const int val = 1;

	if (sd < 0) {
		systemLog->sysLog(ERROR, "you must call the Server::Server constructor before trying to use Server::Listen");
		return -1;
	}
	if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) < 0) {
		systemLog->sysLog(ERROR, "impossible to set KEEPALIVE option on the socket: %s", strerror(errno));
		return -1;
	}

	return 0;
}

socketMsg *Server::recvMessage(int sdc) {
	socketMsg *smsg = (socketMsg *)malloc(sizeof(socketMsg));
	socklen_t slen = sizeof(smsg->saddr);

	if (! smsg) {
		systemLog->sysLog(CRITICAL, "cannot malloc smsg struct: %s", strerror(errno));
		return NULL;
	}
	bzero(smsg, sizeof(*smsg));
	if (sdc < 0) {
		systemLog->sysLog(ERROR, "the socket descriptor is less than 0, so it's impossible to receive a message");
		if (smsg)
			free(smsg);
		return NULL;
	}
	smsg->brecv = recvfrom(sdc, smsg->recvmsg, MAXMSGSIZE, 0, (struct sockaddr *)&smsg->saddr, &slen);
	if (smsg->brecv <= 0) {
		systemLog->sysLog(ERROR, "an error has occured while reading a message on the socket: %s", strerror(errno));
		if (smsg)
			free(smsg);
		return NULL;
	}
	if (smsg->brecv == MAXMSGSIZE) {
		systemLog->sysLog(ERROR, "the reception buffer is completly full, the maximum size is %d", MAXMSGSIZE);
		systemLog->sysLog(ERROR, "you can grow this buffer in the file server.h, the option name is MAXMSGSIZE");
	}

	return smsg;
}

void Server::libereSocketMessage(socketMsg *smsg)
{
	if (smsg)
		free(smsg);
	smsg = NULL;

	return;
}

int Server::sendMessage(int sdc, socketMsg *smsg)
{
	if (sdc < 0) {
		systemLog->sysLog(ERROR, "the socket descriptor is less than 0, so you can't send a message");
		return -1;
	}
	if (! smsg->sendmsg || ! smsg->sendmsg[0]) {
		systemLog->sysLog(ERROR, "sendmsg is not initialized, you can't use Server::sendMessage()");
		return -1;
	}
	smsg->bsent = sendto(sdc, smsg->sendmsg, smsg->len, 0, (struct sockaddr *)&smsg->saddr, sizeof(smsg->saddr));
	if (smsg->bsent <= 0) {
		systemLog->sysLog(ERROR, "an error has occured while sending a message on the socket: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int Server::setNonBlocking() {
	if (sd < 0) {
		systemLog->sysLog(ERROR, "you must call the Server::Server constructor before trying to use Server::Listen");
		return -1;
	}
	if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
		systemLog->sysLog(ERROR, "impossible to set NON BLOCKING on the socket: %s", strerror(errno));
		return -1;
	}

	return 0;
}
