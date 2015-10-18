MAINTAINER=	spebsd@gmail.com

.PATH: ${.CURDIR}/toolkit
.PATH: ${.CURDIR}/src

CXX=		c++
PROG_CXX=	numb
SRCS=		log.cpp mystring.cpp streamer.cpp mutex.cpp semaphore.cpp thread.cpp objectaction.cpp server.cpp protectedmessagelist.cpp httpserver.cpp httpclientconnection.cpp httpcontext.cpp httpsession.cpp httphandler.cpp httpcontent.cpp streamcontent.cpp httpexchange.cpp cachemanager.cpp cachedisk.cpp httpconnection.cpp curl.cpp curlsession.cpp file.cpp cacheobject.cpp multicastserver.cpp hashtableelt.cpp hashtable.cpp hashalgorithm.cpp parser.cpp configuration.cpp keyhashtabletimeout.cpp cataloghashtabletimeout.cpp  administrationserver.cpp administrationserverconnection.cpp mp4streaming.cpp multicastservercatalog.cpp multicastpacketcatalog.cpp main.cpp monitoredhost.cpp mp4reader.cpp moov.cpp
# cachememory.cpp disktomemory.cpp cachememorygc.cpp hashalgorithm.cpp hashtable.cpp

CFLAGS=-Wall -ggdb -O2 -pipe -mmmx -msse -msse2 -msse3 -m3dnow -Wall -I/usr/local/include -DFreeBSD -DACCEPTFILTER -DPSEM # -DDEBUGSTREAMD -DDEBUGOUTPUT # -DDEBUGCATALOG -DSTDOUTDEBUG # -DSENDFILE # -DDEBUGSTREAMD -DSTDOUTDEBUG # -DDEBUG_MEMORY
#CFLAGS=-g -O3 -DPSEM -I/usr/local/include -DFreeBSD -DACCEPTFILTER # -DSENDFILE # -DDEBUGSTREAMD -DSTDOUTDEBUG # -DDEBUG_MEMORY
#CFLAGS+= -Wall -g -DPSEM -I/usr/local/include -DFreeBSD -DACCEPTFILTER -DSENDFILE # -DSTDOUTDEBUG -DDEBUGSTREAMD
#CFLAGS+= -DDEBUG_MEMORY -DPSEM -Wall -ggdb -I/usr/local/include -I/usr/local/include/libxml2 -I/usr/X11R6/include/gtk-2.0 -I/usr/X11R6/include/pango-1.0 -I/usr/local/include/atk-1.0
LDADD=	  -L/usr/local/lib -lthr -lcurl -lz -liconv -lssl -lcrypt -lcrypto
NO_MAN=	true

.include <bsd.prog.mk>
