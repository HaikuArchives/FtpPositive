#ifndef _OAEDIT_H
#define _OAEDIT_H


#include <TextControl.h>
#include <MessageFilter.h>


class TOAEdit;


class TOAFilter : public BMessageFilter
{
public:
	TOAFilter(TOAEdit *owner);
	~TOAFilter();
	filter_result Filter(BMessage *message, BHandler **handler);
private:
	BMessenger *OAMessenger;
	TOAEdit *fOwner;
};

class TOAEdit : public BTextControl
{
public:
	TOAEdit(const char *name,
		const char *label,
		const char *text,
		BMessage *message,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	TOAEdit(BMessage *archive);
	static BArchivable *Instantiate(BMessage *archive);
	status_t Archive(BMessage *archive, bool deep = true) const;
	void MessageReceived(BMessage *msg);
	void AttachedToWindow();
	void GoNext(bool direction);
	bool EnableEnter() const;
	bool EnableDownArrow() const;
	bool EnableUpArrow() const;
	void SetEnableEnter(bool enabled);
	void SetEnableDownArrow(bool enabled);
	void SetEnableUpArrow(bool enabled);
private:
	bool fEnableEnter;
	bool fEnableDownArrow;
	bool fEnableUpArrow;
	bool fInstantiatedFromArchive;
};

#endif
