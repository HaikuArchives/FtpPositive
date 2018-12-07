#include <malloc.h>
#include "StringEx.h"

// TStringEx

TStringEx::TStringEx()
	:	BString()
{
}

TStringEx::TStringEx(const char *string)
	:	BString(string)
{
}

TStringEx::TStringEx(const char *string, int32 maxLength)
	:	BString(string, maxLength)
{
}

TStringEx::TStringEx(const BString &string)
	:	BString(string)
{
}

TStringEx::~TStringEx()
{
}

void TStringEx::TrimLeft()
{
	const char *str = this->String();
	int len = this->Length();
	int i;
	for(i=0; str[i]==' ' && i<len; i++);
	if (i<=0) return;
	
	char *blk = (char *)malloc(i+1);
	blk[i] = 0;
	for(; i>0; i--) blk[i-1]=' ';
	
	this->RemoveFirst(blk);
	free(blk);
}

void TStringEx::TrimRight()
{
	const char *str = this->String();
	int len = this->Length();
	int i, j = 0;
	for(i=len-1; i>=0 && str[i]==' '; i--) j++;
	if (j<=0) return;
	
	char *blk = (char *)malloc(j+1);
	blk[j] = 0;
	for(; j>0; j--) blk[j-1]=' ';
	
	this->RemoveLast(blk);
	free(blk);
}

void TStringEx::Trim()
{
	TrimLeft();
	TrimRight();
}
