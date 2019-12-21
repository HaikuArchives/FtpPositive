#ifndef _STREAM_BUFF_H
#define _STREAM_BUFF_H

#include <List.h>
#include <String.h>

enum {
	SB_UNKNOWN_TEXT_TYPE,	// Automatically recognize CR, CRLF, LF. LFCR considers two line feeds.
	SB_LF_TEXT_TYPE,		// Treat LF as a line feed. UNIX / BeOS style
	SB_CRLF_TEXT_TYPE,		// Treat CRLF as a line feed. DOS / Windows style
	SB_CR_TEXT_TYPE,		// Treat CR as a line feed. MacOS (9.x or earlier) style
	SB_BINARY_TYPE			// Binary treatment without line feed code. Do not format.
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
