#include "StreamBuff.h"


#define CR "\r"
#define LF "\n"


TStreamBuff::TStreamBuff(int32 textType, int32 flags = 0)
{
	SetTo(textType, flags);
}

TStreamBuff::~TStreamBuff()
{
	MakeEmpty();
}

void TStreamBuff::SetTo(int32 textType, int32 flags = 0)
{
	BString backup("");
	for(int i=0;FStreamBuffer.CountItems();i++) {
		backup.Append( *(BString *)FStreamBuffer.ItemAt(i) );
	}
	backup.Append(FPendingBuffer);

	FTextType = textType;
	FFlags = flags;
	MakeEmpty();
	switch(textType) {
		case SB_UNKNOWN_TEXT_TYPE:
			FSeparator.SetTo("");
			break;
		case SB_LF_TEXT_TYPE:
			FSeparator.SetTo(LF);
			break;
		case SB_CRLF_TEXT_TYPE:
			FSeparator.SetTo(CR LF);
			break;
		case SB_CR_TEXT_TYPE:
			FSeparator.SetTo(CR);
			break;
		case SB_BINARY_TYPE:
			FSeparator.SetTo("");
			break;
		default: FSeparator.SetTo("");
	}

	AddStream(backup.String(), backup.Length());
}

void TStreamBuff::_AddStream(const char *input)
{
	BString istr;
	int sp;
	BString _input(input);

	sp = _input.FindFirst(FSeparator);
	if (sp>-1) {
		BString *new_str = new BString("");
		_input.CopyInto(*new_str,0,sp);
		FStreamBuffer.AddItem(new_str);
		_input.CopyInto(istr, sp + FSeparator.Length(), _input.Length() );
		FPendingBuffer = istr;
		_AddStream(istr.String());
	} else {
		FPendingBuffer.SetTo(input);
	}
}


struct int_item {
	int value;
	int index;
};

int _int_item_sort(const void *item1, const void *item2)
{
	struct int_item **i1 = (struct int_item **)item1;
	struct int_item **i2 = (struct int_item **)item2;

	if (i1[0]->value > i2[0]->value) { return 1; } else
	if (i1[0]->value < i2[0]->value) { return -1; }
	return 0;
}

void TStreamBuff::_AddStream2(const char *input)
{
	BString istr;
	int sp, splen, idx;
	BString _input(input);

	if (_input.Length()<=0) return;

	int sp_cr = _input.FindFirst(CR);
	int sp_lf = _input.FindFirst(LF);
	int sp_crlf = _input.FindFirst(CR LF);

	static struct int_item st_cr;
	st_cr.value = sp_cr;
	st_cr.index = 0;
	
	static struct int_item st_lf;
	st_lf.value = sp_lf;
	st_lf.index = 1;

	static struct int_item st_crlf;
	st_crlf.value = sp_crlf;
	st_crlf.index = 2;

	BList list;
	list.AddItem(&st_cr);
	list.AddItem(&st_lf);
	list.AddItem(&st_crlf);
	list.SortItems(&_int_item_sort);

	sp = -1;
	idx = -1;
	for(int i=0; i<list.CountItems(); i++) {
		struct int_item *item = (struct int_item *)list.ItemAt(i);
		if (item->value > -1) {
			sp = item->value;
			idx = item->index;
			break;
		}
	}

	switch(idx) {
		case 0:
		case 1:
			splen = 1;
			break;
		case 2:
			splen = 2;
			break;
		default:
			splen = 0;
	}
	if (sp_crlf == sp) splen = 2;

	if (sp>-1) {
		BString *new_str = new BString("");
		_input.CopyInto(*new_str,0,sp);
		FStreamBuffer.AddItem(new_str);
		_input.CopyInto(istr, sp + splen, _input.Length() );
		FPendingBuffer = istr;
		_AddStream2(istr.String());
	} else {
		FPendingBuffer.SetTo(input);
	}
}

void TStreamBuff::AddStream(const char *input, uint32 len)
{
	BString str(FPendingBuffer);
	str.Append(input, len);

	switch(FTextType) {
		case SB_UNKNOWN_TEXT_TYPE:
			_AddStream2(str.String());
			break;
		case SB_LF_TEXT_TYPE:
		case SB_CRLF_TEXT_TYPE:
		case SB_CR_TEXT_TYPE:
			_AddStream(str.String());
			break;
		case SB_BINARY_TYPE:
			if (FStreamBuffer.CountItems()==0) {
				BString *new_str = new BString("");
				FStreamBuffer.AddItem(new_str);
				((BString *)(FStreamBuffer.ItemAt(0)))->Append(input);
			}
			break;
	}
}

int32 TStreamBuff::Get(BString *output)
{
	if (FStreamBuffer.CountItems() <= 0) {
		output->SetTo("");
		return SB_BUFFER_EMPTY;
	}
	output->SetTo(((BString *)FStreamBuffer.ItemAt(0))->String());
	delete (BString *)FStreamBuffer.ItemAt(0);
	FStreamBuffer.RemoveItems(0, 1);
	return FStreamBuffer.CountItems();
}

int32 TStreamBuff::CountLines()
{
	return FStreamBuffer.CountItems();
}

const char *TStreamBuff::LineAt(int32 index)
{
	if ((index < 0) || (index >= FStreamBuffer.CountItems())) return NULL;
	return ((BString *)FStreamBuffer.ItemAt(index))->String();
}

bool TStreamBuff::Pending()
{
	return FPendingBuffer.Length()>0;
}

void TStreamBuff::PendingStream(BString *pending, bool remove = false)
{
	pending->SetTo(FPendingBuffer);
	if (remove) {
		FPendingBuffer.SetTo("");
	}
}

void TStreamBuff::MakeEmpty()
{
	for(int i=0; i < FStreamBuffer.CountItems(); i++) {
		BString *str = (BString *)FStreamBuffer.ItemAt(i);
		delete str;
	}
	FStreamBuffer.MakeEmpty();
	FPendingBuffer.SetTo("");
}
