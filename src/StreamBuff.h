#ifndef _STREAM_BUFF_H
#define _STREAM_BUFF_H

#include <List.h>
#include <String.h>

enum {
	SB_UNKNOWN_TEXT_TYPE,	// automatically recognize CR, CRLF, and LF; regard LFCR as 2 line breaks
	SB_LF_TEXT_TYPE,		// handle LF as line break; UNIX/BeOS style
	SB_CRLF_TEXT_TYPE,		// handle CRLF as line break; DOS/Windows style
	SB_CR_TEXT_TYPE,		// handle CR as line break; MacOS (before 9.x) style
	SB_BINARY_TYPE		// handle no line break as binary file; no style
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
