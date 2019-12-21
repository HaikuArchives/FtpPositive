#ifndef _STREAM_BUFF_H
#define _STREAM_BUFF_H

#include <List.h>
#include <String.h>

enum {
	SB_UNKNOWN_TEXT_TYPE,	// CR,CRLF,LF - LFCR
	SB_LF_TEXT_TYPE,		// LF - UNIX/BeOS
	SB_CRLF_TEXT_TYPE,		// CRLF - DOS/Windows
	SB_CR_TEXT_TYPE,		// CR - MacOS(9.x)
	SB_BINARY_TYPE			//
};

#define SB_BUFFER_EMPTY -1


class TStreamBuff
{
private:
	int32 FTextType;
	int32 FFlags;
	BList FStreamBuffer;
	BString FPendingBuffer;
	BString FSeparator;
	
	void _AddStream(const char *input);
	void _AddStream2(const char *input);
	
public:
	TStreamBuff(int32 textType, int32 flags = 0);
	~TStreamBuff();
	void SetTo(int32 textType, int32 flags = 0);
	void AddStream(const char *input, uint32 len);
	int32 Get(BString *output);
	void MakeEmpty();
	
	int32 CountLines();
	const char *LineAt(int32 index);
	bool Pending();
	void PendingStream(BString *pending, bool remove = false);
};


#endif
