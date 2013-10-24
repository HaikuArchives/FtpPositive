#ifndef _TCPSTREAM_H_
#define _TCPSTREAM_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <DataIO.h>
#include <SupportDefs.h>


typedef enum tcp_stream_mode_t {
	TCP_STREAM_DOWNLOAD,
	TCP_STREAM_UPLOAD,
} tcp_stream_mode_t;


class TTCPStream
{
public:
	TTCPStream(BDataIO *dataIO, struct sockaddr_in *addr,
		bigtime_t waitTime, uint32 buffSize, tcp_stream_mode_t tcpStreamMode);
	~TTCPStream();
	status_t Status() const;
	status_t Go();
	status_t Result(int64 *transferredSize) const;
	void Abort();
private:
	volatile bool fAborted;
	volatile off_t fTransferredSize;
	volatile status_t fStatus;
	
	static int32 ReceiveFunc(void *self);
	static int32 SendFunc(void *self);
	
	thread_id fThreadID;
	BDataIO *fDataIO;
	int fEndpoint;
	bigtime_t fWaitTime;
	
	void *fBuffer;
	uint32 fBufferSize;
};


#endif
