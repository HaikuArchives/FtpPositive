#include <DataIO.h>
#include <OS.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "FTPClient.h"


TFTPClient::TFTPClient()
{
	fAbort = false;
	fControlEndpoint = -1;
	fWaitTime = 60000000;
	fReplyBuff = new TStreamBuff(SB_UNKNOWN_TEXT_TYPE);
	fSemID = -1;
	fReceiverThreadID = -1;
	
	fSemID = create_sem(1, "stream_buff");
	if (fSemID < 0) {
		fStatus = fSemID;
		fStrError.SetTo(strerror(fStatus));
		return;
	}
	
	// 受信スレッド生成
	fReceiverThreadID = spawn_thread(TFTPClient::ReceiverThread,
		"receiver_thread", B_NORMAL_PRIORITY, this);
	if (fReceiverThreadID < 0) {
		fStatus = fReceiverThreadID;
		fStrError.SetTo(strerror(fStatus));
		return;
	}
	
	fStatus = B_OK;
}

TFTPClient::~TFTPClient()
{
	this->Disconnect();
	if (fReceiverThreadID >= B_OK) kill_thread(fReceiverThreadID);
	delete fReplyBuff;
	if (fSemID >= B_OK) delete_sem(fSemID);
}

status_t TFTPClient::Status() const
{
	return fStatus;
}

const char *TFTPClient::StrError() const
{
	return fStrError.String();
}

status_t TFTPClient::Connect(const char *address, uint16 port)
{
	int err;
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_len = sizeof(struct sockaddr_in);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	if (inet_aton(address, &addr.sin_addr) == 0) {
		struct hostent *host = gethostbyname(address);
		if (!host) {
			err = h_errno;
			fStrError.SetTo(hstrerror(err));
			return err;
		}
		addr.sin_addr = *(struct in_addr *)host->h_addr;
	}
	
	fprintf(stderr, "Connecting %u.%u.%u.%u : %u\n",
		((uint8 *)&addr.sin_addr.s_addr)[0],
		((uint8 *)&addr.sin_addr.s_addr)[1],
		((uint8 *)&addr.sin_addr.s_addr)[2],
		((uint8 *)&addr.sin_addr.s_addr)[3],
		port);
	
	if (fControlEndpoint != -1) Disconnect();
	
	fControlEndpoint = socket(AF_INET, SOCK_STREAM, 0);
	if (fControlEndpoint == -1) {
		err = errno;
		fStrError.SetTo(strerror(err));
		return err;
	}
	
	// non-blocking mode
	if ((err = SetBlockingMode(false)) != B_OK) return err;
	
	// connect
	if (connect(fControlEndpoint, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		err = errno;
		fStrError.SetTo(strerror(err));
		if ((err != 0) && (err != EISCONN) && (err != EINPROGRESS) && (err != EALREADY)) return err;
	}
	
	// blocking mode
//	if ((err = SetBlockingMode(true)) != B_OK) return err;
//	snooze(500000);
	
	// 受信スレッド開始
	err = resume_thread(fReceiverThreadID);
	
	return err;
}

void TFTPClient::Disconnect()
{
	fStatus = EPIPE;
	fStrError.SetTo("Disconnected.");
	if (fControlEndpoint != 0) close(fControlEndpoint);
	fControlEndpoint = 0;
	fAbort = true;
}

status_t TFTPClient::SetBlockingMode(bool wouldBlock)
{
	int err, block;
	if (wouldBlock) {
		block = 0;
		if (setsockopt(fControlEndpoint, SOL_SOCKET, SO_NONBLOCK, &block, sizeof(block)) < 0) {
			err = errno;
			fStrError.SetTo(strerror(err));
			return err;
		}
	} else {
		block = 1;
		if (setsockopt(fControlEndpoint, SOL_SOCKET, SO_NONBLOCK, &block, sizeof(block)) < 0) {
			err = errno;
			fStrError.SetTo(strerror(err));
			return err;
		}
	}
	return B_OK;
}

// コマンド送信
status_t TFTPClient::SendCmd(const char *command, uint32 commandLen)
{
	fAbort = false;
	status_t e;
	ssize_t s;
	s = write(fControlEndpoint, command, commandLen);
	if (s == -1) {
		e = errno;
		fStrError.SetTo(strerror(e));
		return e;
	} else if (s == 0) {
		e = EPIPE;
		fStrError.SetTo(strerror(e));
		return e;
	}
	return B_OK;
}

// 受信文字列を一行ずつ取得
status_t TFTPClient::GetLastMessage(BString *str, int32 *reply, bool *isLast)
{
	*reply = 0;
	*isLast = false;
	
	status_t e = acquire_sem(fSemID);
	if (e != B_OK) return e;
	fReplyBuff->Get(str);
	release_sem(fSemID);
	
	if (str->Length() >= 4) {
		*reply = atoi(str->String());
		if (str->String()[3] == ' ') {
			*isLast = true;
			fStrReply.SetTo(str->String());
		}
	} else {
		*reply = -1;
	}
	return B_OK;
}

void TFTPClient::ClearMessages()
{
	acquire_sem(fSemID);
	fReplyBuff->MakeEmpty();
	release_sem(fSemID);
}

const char *TFTPClient::StrReply() const
{
	return fStrReply.String();
}

// 受信スレッド関数
int32 TFTPClient::ReceiverThread(void *self)
{
	TFTPClient *Self = (TFTPClient *)self;
	int32 recvSize;
	char recvBuff[1024];
	
	while(!Self->fAbort) {
		recvSize = read(Self->fControlEndpoint, recvBuff, sizeof(recvBuff));
		if (recvSize < 0) {
			// 受信データが無い or エラー
			int e = errno;
			if (e != EAGAIN) {
				// エラー
				Self->fStrError.SetTo(strerror(e));
				Self->fStatus = EPIPE;
			}
		} else if (recvSize == 0) {
			// 切断された
			Self->fStrError.SetTo("Disconnected by remote host.");
			Self->fStatus = EPIPE;
		} else {
			// 受信あり
			acquire_sem(Self->fSemID);
			Self->fReplyBuff->AddStream(recvBuff, recvSize);
			release_sem(Self->fSemID);
		}
		snooze(10000);
	}
	return 0;
}
