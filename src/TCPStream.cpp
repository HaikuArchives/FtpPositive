#include <OS.h>
#include <string.h>
#include <sys/select.h>
#include "TCPStream.h"


TTCPStream::TTCPStream(BDataIO *dataIO, struct sockaddr_in *addr,
	bigtime_t waitTime, uint32 buffSize, tcp_stream_mode_t tcpStreamMode)
{
	int block;
	fTransferredSize = 0;
	fDataIO = dataIO;
	fWaitTime = waitTime;
	fEndpoint = socket(AF_INET, SOCK_STREAM, 0);
	if (fEndpoint == -1) {
		fStatus = errno;
		return;
	}
	
	// allocation of sending / receiving buffer
	if ((fBuffer = malloc(buffSize)) == NULL) {
		fStatus = ENOMEM;
		return;
	}
	fBufferSize = buffSize;
	
	// non-blocking mode
	block = 1;
	if (setsockopt(fEndpoint, SOL_SOCKET, SO_NONBLOCK, &block, sizeof(block)) < 0) {
		fStatus = errno;
		return;
	}
	
	// connect
	if (connect(fEndpoint, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) != 0) {
		fStatus = errno;
		if ((fStatus != 0) && (fStatus != EISCONN) && (fStatus != EINPROGRESS) && (fStatus != EALREADY)) {
			return;
		}
	}
	
	fAborted = false;
	
	switch (tcpStreamMode) {
		case TCP_STREAM_DOWNLOAD:
			// start receiving thread
			fThreadID = spawn_thread(TTCPStream::ReceiveFunc, "TCPStream", B_NORMAL_PRIORITY, this);
			if (fThreadID < 0) {
				fStatus = fThreadID;
				return;
			}
			break;
		case TCP_STREAM_UPLOAD:
			// blocking mode
			block = 0;
			if (setsockopt(fEndpoint, SOL_SOCKET, SO_NONBLOCK, &block, sizeof(block)) < 0) {
				fStatus = errno;
				return;
			}
			// start sending thread
			fThreadID = spawn_thread(TTCPStream::SendFunc, "TCPStream", B_NORMAL_PRIORITY, this);
			if (fThreadID < 0) {
				fStatus = fThreadID;
				return;
			}
			break;
	}
	
	fStatus = B_OK;
}

TTCPStream::~TTCPStream()
{
	if (fEndpoint != -1) close(fEndpoint);
	free(fBuffer);
}

status_t TTCPStream::Status() const
{
	return fStatus;
}

status_t TTCPStream::Result(int64 *transferredSize) const
{
	*transferredSize = fTransferredSize;
	return fStatus;
}

status_t TTCPStream::Go()
{
	fAborted = false;
	fStatus = EINPROGRESS;
	status_t s = resume_thread(fThreadID);
	if (s == B_OK) return B_OK;
	fStatus = s;
	return s;
}

int32 TTCPStream::ReceiveFunc(void *self)
{
	TTCPStream *Self = (TTCPStream *)self;
	int32 recvSize;
	Self->fTransferredSize = 0;
	bigtime_t sleeptime = 1000;
	
	bigtime_t limit = system_time() + Self->fWaitTime;
	while (!Self->fAborted) {
		recvSize = read(Self->fEndpoint, Self->fBuffer, Self->fBufferSize);
//		printf("%d\n", (int)recvSize);
		if (recvSize < 0) {
			// no data received or error
			int e = errno;
			if (e != EAGAIN) {
				// error
				Self->fStatus = e;
				break;
			}
			if (system_time() > limit) {
				// Timeout!
				Self->fStatus = B_TIMED_OUT;
				break;
			}
		} else if (recvSize == 0) {
			// disconnected
			Self->fStatus = B_OK;
			break;
		} else {	
			// received
			limit = system_time() + Self->fWaitTime;
			Self->fTransferredSize += Self->fDataIO->Write(Self->fBuffer, recvSize);
/*			
			if (d > recvSize) {
				sleeptime += 1;
			} else {
				sleeptime -= 1;
			}
			printf("size=%d sleep=%d\n", (int)recvSize, (int)sleeptime);
*/			
		}
		snooze(sleeptime);
	}
	if (Self->fAborted) fprintf(stderr, "ReceiveFunc: Transfer aborted.\n");
	return 0;
}

int32 TTCPStream::SendFunc(void *self)
{
	ssize_t blen, slen;
	size_t wsize;
	char *p;
	TTCPStream *Self = (TTCPStream *)self;
	Self->fTransferredSize = 0;
	while (!Self->fAborted) {
		p = (char *)Self->fBuffer;
		blen = Self->fDataIO->Read(Self->fBuffer, Self->fBufferSize);
		if (blen <= 0) {
			// end of sending
			Self->fStatus = B_OK;
			break;
		}
		wsize = blen;
		while(wsize > 0) {
			slen = write(Self->fEndpoint, p, wsize);
			if (slen > 0) {
				// successfully sent
				wsize -= slen;
				Self->fTransferredSize += slen;
				p += slen;
			} else if (slen == -1) {
				// error
				fprintf(stderr, "error(1)\n");
				Self->fStatus = B_IO_ERROR;
				break;
			} else {
				// impossible
				fprintf(stderr, "error(2)\n");
				Self->fStatus = B_IO_ERROR;
				break;
			}
		}
		
	}
	if (Self->fAborted) fprintf(stderr, "SendFunc: Transfer aborted.\n");
	close(Self->fEndpoint);
	Self->fEndpoint = -1;
	return 0;
}

void TTCPStream::Abort()
{
	status_t exit_status;
	fAborted = true;
	wait_for_thread(fThreadID, &exit_status);
}
