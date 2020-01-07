#include <stdio.h>
#include <UTF8.h>
#include <String.h>


extern "C" _EXPORT void fp_text_convert(const char *src, BString *dest, bool to_utf8)
{
	int32 srcLen = strlen(src);
	int32 destLen = srcLen * 4;
	char dstr[destLen];
	int32 state;
	status_t e;
	
	if (to_utf8) {
		e = convert_to_utf8(B_SJIS_CONVERSION, src, &srcLen, dstr, &destLen, &state);
		// 1A バグ回避
		// 1A avoid bug
		if (dstr[destLen - 1] == 0x1a) {
			dstr[destLen - 1] = 0;
			destLen--;
		}
	} else {
		e = convert_from_utf8(B_SJIS_CONVERSION, src, &srcLen, dstr, &destLen, &state);
	}
	
	if (e == B_OK) {
		dest->SetTo(dstr, destLen);
	} else {
		dest->SetTo(src);
	}
}
