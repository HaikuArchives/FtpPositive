#ifndef _FTPCLIENT_H
#define _FTPCLIENT_H

#include <OS.h>
#include <String.h>
#include <sys/time.h>
#include "StreamBuff.h"

class TFTPClient
{
public:
	TFTPClient();
	~TFTPClient();
	status_t Status() const;
	const char *StrError() const;
	const char *StrReply() const;
	
	status_t Connect(const char *address, uint16 port);
	void Disconnect();
	status_t SetBlockingMode(bool wouldBlock);
	
	status_t SendCmd(const char *command, uint32 commandLen);
	
	status_t GetLastMessage(BString *str, int32 *reply, bool *isLast);
	void ClearMessages();
	
private:
	status_t fStatus;
	volatile bool fAbort;
	
	static int32 ReceiverThread(void *self);
	thread_id fReceiverThreadID;
	
	BString fStrError;
	BString fStrReply;
	int fControlEndpoint;
	bigtime_t fWaitTime;
	
	TStreamBuff *fReplyBuff;
	sem_id fSemID;
};

#endif
