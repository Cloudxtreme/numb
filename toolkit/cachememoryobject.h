#ifndef CACHEMEMORYOBJECT_H
#define CACHEMEMORYOBJECT_H

/* SAAAAAAAAALLLLLLLLEEEEEEEEEE !!!! :))) */
struct CacheMemoryObject {
	struct timeval timeVal;
	unsigned int objectSize;
	char *object;
};

#endif

