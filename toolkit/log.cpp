/*
 * Cette classe permet de logguer des messages soit dans un fichier local soit dans syslogd.
 * facility peut etre: LOG_ERROR, LOG_CRIT etc... ou LOG_FILE si on loggue dans un fichier
 * mode vaut SYSLOG ou LOCALFILE
 * setOutput(...): Regle le mode de log de la chaine (fichier ou syslog)
 * getOutput()   : Recupere le mode dans lequel on se trouve
 * syslog(...)   : Loggue une chaine de caractere
 * closeLog()    : Ferme le log
 */

#include "log.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pthread.h>

LogError::LogError()
{
	mode = STDOUT;
	logOpt = 0;
	strcpy(ident, "UserLog");
	initMutexes();

	return;
}

LogError::LogError(const char *_ident, int _mode) {
	mode = _mode;
	initMutexes();
	if (mode & SYSLOG) {
		logOpt = 0;
		strncpy(ident, _ident, sizeof(ident)-1);
		ident[sizeof(ident)-1] = '\0';
		openlog(ident, LOG_NDELAY, LOG_USER);
		return;
	}
	if (mode & LOCALFILE)
		openFile(_ident);

	return;
}

LogError::~LogError()
{
	bzero(ident, sizeof(ident));
	if (mode & SYSLOG)
		closelog();
	if (mode & LOCALFILE)
		closeFile();
	mode = 0;
	localFile = 0;

  return;
}

void LogError::setOutput(char _mode)
{
	pthread_rwlock_wrlock(&lockmode);
	mode = _mode;
	pthread_rwlock_unlock(&lockmode);

	return;
}

char LogError::getOutput(void)
{
	char m;

	pthread_rwlock_rdlock(&lockmode);
	m = mode;
	pthread_rwlock_unlock(&lockmode);
	return m;
}

void LogError::setIdentity(const char *_ident)
{
	pthread_rwlock_wrlock(&lockidentity);
	strncpy(ident, _ident, sizeof(ident)-1);
	ident[sizeof(ident)-1] = '\0';
	pthread_rwlock_unlock(&lockidentity);

	return;
}

void LogError::initMutexes(void)
{
	pthread_rwlock_init(&locksyslog, NULL);
	pthread_rwlock_init(&lockmode, NULL);
	pthread_rwlock_init(&lockfacility, NULL);
	pthread_rwlock_init(&lockidentity, NULL);
	pthread_rwlock_init(&lockopen, NULL);
	pthread_rwlock_init(&lockclose, NULL);

	return;
}

void LogError::sysLog(int facility, const char *format, ...)
{
	va_list ap;
	char buf[LOGBUFFER];
	char buf2[LOGBUFFER];

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	if (! buf[0])
		return;
	if (mode & LOCALFILE) {
		// A modifier afin d'inclure la date et l'heure quand on loggue dans un fichier
		snprintf(buf2, sizeof(buf2), "%s\n", buf);
		pthread_rwlock_wrlock(&locksyslog);
		write(getLocalFile(), buf2, strlen(buf2));
		pthread_rwlock_unlock(&locksyslog);
		//fflush(getLocalFile());
		return;
	}
	if (mode & SYSLOG) {
		pthread_rwlock_wrlock(&locksyslog);
		syslog(facility, "%s", buf);
		pthread_rwlock_unlock(&locksyslog);
		return;
	}
	if (mode & STDOUT)
		fprintf(stderr, "%s\n", buf);
	

	return;
}

void LogError::sysLog(const char *identity, int facility, const char *format, ...) {
	va_list ap;
	char buf[LOGBUFFER];
	char buf2[LOGBUFFER];

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	if (! buf[0]) {
		systemLog->sysLog(ERROR, "cannot log a NULL string");
		return;
	}
	if (mode & LOCALFILE) {
		// A modifier afin d'inclure la date et l'heure quand on loggue dans un fichier
		snprintf(buf2, sizeof(buf2), "%s\n", buf);
		pthread_rwlock_wrlock(&locksyslog);
		write(getLocalFile(), buf2, strlen(buf2));
		pthread_rwlock_unlock(&locksyslog);
		//fflush(getLocalFile());
		return;
	}
	if (mode & SYSLOG) {
		pthread_rwlock_wrlock(&locksyslog);
		openlog(identity, LOG_NDELAY, LOG_USER);
		syslog(facility, "%s", buf);
		closelog();
		openlog(ident, LOG_NDELAY, LOG_USER);
		pthread_rwlock_unlock(&locksyslog);
		return;
	}
	if (mode & STDOUT)
		fprintf(stderr, "%s\n", buf);
	

	return;
}

void LogError::sysLog(const char *file, const char *function, unsigned int line, int facility, const char *format, ...)
{
	va_list ap;
	char buf[LOGBUFFER];
	char buf2[LOGBUFFER];

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	if (! buf[0]) {
		systemLog->sysLog(ERROR, "cannot log a NULL string");
		return;
	}
	// A modifier afin d'inclure la date et l'heure quand on loggue dans un fichier
	
	if (mode & LOCALFILE) {
		snprintf(buf2, sizeof(buf2), "%s %s:%d %s\n", file, function, line, buf);
		pthread_rwlock_wrlock(&locksyslog);
		write(getLocalFile(), buf2, strlen(buf2));
		pthread_rwlock_unlock(&locksyslog);
		//fflush(getLocalFile());
		return;
	}
	if (mode & SYSLOG) {
		snprintf(buf2, sizeof(buf2), "%s %s:%d %s\n", file, function, line, buf);
		pthread_rwlock_wrlock(&locksyslog);
		syslog(facility, "%s", buf2);
		pthread_rwlock_unlock(&locksyslog);
		return;
	}
	if (mode & STDOUT)
		fprintf(stderr, "%s\n", buf);

	return;
}

void LogError::openFile(const char *name)
{
	pthread_rwlock_wrlock(&lockopen);
	setLocalFile(open(name, O_APPEND | O_WRONLY | O_CREAT, 0644));
	pthread_rwlock_unlock(&lockopen);

	return;
}

void LogError::closeFile(void)
{	
	pthread_rwlock_wrlock(&lockclose);
	close(getLocalFile());
	pthread_rwlock_unlock(&lockclose);

	return;
}
