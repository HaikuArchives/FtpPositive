#include <stdio.h>
#include "OAEdit.h"

#define OA_GONEXT_MSG 'ognm'
#define OA_GOPREV_MSG 'ogpm'


// TOAFilter

TOAFilter::TOAFilter(TOAEdit *owner):BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
	fOwner = owner;
	OAMessenger = new BMessenger(owner);
}

TOAFilter::~TOAFilter()
{
	delete OAMessenger;
}

//	BTextControl の子 BView である BTextView に送られる BMessage をフィルタする。
//	ENTERキーと↓、↑を横取りして、次または前の BView に MakeFocus する。
//	filter BMessage sent by BTextView in child BView of BTextControl
//	capture ENTER key, up arrow key, and down arrow key, then MakeFocus by next or previous BView
filter_result TOAFilter::Filter(BMessage *message, BHandler **handler)
{
	switch(message->what) {
		case B_KEY_DOWN:
			int8 kvalue;
			if (message->FindInt8("byte", 0, &kvalue)==B_OK) {
				switch(kvalue) {
					case B_ENTER:
						if (fOwner->EnableEnter()) OAMessenger->SendMessage(OA_GONEXT_MSG);
						break;
					case B_DOWN_ARROW:
						if (fOwner->EnableDownArrow()) OAMessenger->SendMessage(OA_GONEXT_MSG);
						break;
					case B_UP_ARROW:
						if (fOwner->EnableUpArrow()) OAMessenger->SendMessage(OA_GOPREV_MSG);
						break;
				}
			}
			break;
	}
	return B_DISPATCH_MESSAGE;
}


// TOAEdit

TOAEdit::TOAEdit(const char *name,
		const char *label,
		const char *text,
		BMessage *message,
		uint32 flags)
	:	BTextControl(name, label, text, message, flags)
{
	fEnableEnter = true;
	fEnableDownArrow = true;
	fEnableUpArrow = true;
	fInstantiatedFromArchive = false;
}

TOAEdit::TOAEdit(BMessage *archive)
	:	BTextControl(archive)
{
	if (archive->FindBool("oaedit:enable_enter", &fEnableEnter) != B_OK) fEnableEnter = true;
	if (archive->FindBool("oaedit:enable_downallow", &fEnableDownArrow) != B_OK) fEnableDownArrow = true;
	if (archive->FindBool("oaedit:enable_uparrow", &fEnableUpArrow) != B_OK) fEnableUpArrow = true;
	fInstantiatedFromArchive = true;
}

BArchivable *TOAEdit::Instantiate(BMessage *archive)
{
	return new TOAEdit(archive);
}

status_t TOAEdit::Archive(BMessage *archive, bool deep) const
{
	archive->AddBool("oaedit:enable_enter", fEnableEnter);
	archive->AddBool("oaedit:enable_downallow", fEnableDownArrow);
	archive->AddBool("oaedit:enable_uparrow", fEnableUpArrow);
	return BTextControl::Archive(archive, deep);
}

void TOAEdit::AttachedToWindow()
{
// 中にいるBTextViewにメッセージフィルタをかまして
// ENTERキー、および矢印キーを監視できるようにする。
// insert message filter to inner BTextView
// enable monitoring ENTER key and arrow keys
	TextView()->AddFilter(new TOAFilter(this));
	if (fInstantiatedFromArchive == false) {
		if (Parent() != NULL) {
			SetViewColor(Parent()->ViewColor());
		}
	}
}

void TOAEdit::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case OA_GONEXT_MSG:
			GoNext(true);
			break;
		case OA_GOPREV_MSG:
			GoNext(false);
			break;
	}
}

// フォーカス移動
// direction = true で次の BView、false で前の BView に移動する。
// move focus
// move BView after direction = true to BView before direction = false
void TOAEdit::GoNext(bool direction)
{
	BView *next = NULL;
	if (direction) {
		next = NextSibling();
	} else {
		next = PreviousSibling();
	}

	if (next!=NULL) {
		next->MakeFocus();
	}
}

bool TOAEdit::EnableEnter() const
{
	return fEnableEnter;
}

bool TOAEdit::EnableDownArrow() const
{
	return fEnableDownArrow;
}

bool TOAEdit::EnableUpArrow() const
{
	return fEnableUpArrow;
}

void TOAEdit::SetEnableEnter(bool enabled)
{
	fEnableEnter = enabled;
}

void TOAEdit::SetEnableDownArrow(bool enabled)
{
	fEnableDownArrow = enabled;
}

void TOAEdit::SetEnableUpArrow(bool enabled)
{
	fEnableUpArrow = enabled;
}
