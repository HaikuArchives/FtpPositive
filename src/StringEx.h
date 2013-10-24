#ifndef _STRINGEX_H
#define _STRINGEX_H

#include <String.h>

class TStringEx : public BString
{
public:
	TStringEx();
	TStringEx(const char *string);
	TStringEx(const char *string, int32 maxLength);
	TStringEx(const BString &string);
	~TStringEx();
	
	void TrimLeft();
	void TrimRight();
	void Trim();
};

#endif
