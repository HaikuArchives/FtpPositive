#ifndef _STREAM_BUFF_H
#define _STREAM_BUFF_H

#include <List.h>
#include <String.h>

enum {
	SB_UNKNOWN_TEXT_TYPE,	// CR,CRLF,LFを自動認識する。LFCRは２改行とみなす。
	SB_LF_TEXT_TYPE,		// LFを改行として扱う。UNIX/BeOS風
	SB_CRLF_TEXT_TYPE,		// CRLFを改行として扱う。DOS/Windows風
	SB_CR_TEXT_TYPE,		// CRを改行として扱う。MacOS(9.x以前)風
	SB_BINARY_TYPE		// 改行コードなしのバイナリ扱い。整形しない。
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
