#ifndef _REMOTE_FILE_VIEW_H_
#define _REMOTE_FILE_VIEW_H_

#include <Bitmap.h>
#include <MessageRunner.h>
#include <ColumnListView.h>
#include "ColumnTypes.h"


enum {
	CLM_NAME,
	CLM_INTERNAL_NAME,
	CLM_SIZE,
	CLM_DATE,
	CLM_PERMISSION,
	CLM_OWNER,
	CLM_GROUP,
};

#define MSG_ISWORD_RESET 'rest'


class TStringColumn : public BStringColumn
{
public:
	TStringColumn(const char* title, float width, float minWidth,
							 float maxWidth, uint32 truncate, alignment align = B_ALIGN_LEFT);
	~TStringColumn();
	int CompareFields(BField *field1, BField *field2, BRow *row1, BRow *row2);
};

class TIconRow : public BRow
{
public:
	TIconRow(BBitmap *icon) : BRow() { fIcon = icon; } ;
	~TIconRow() { if (fIcon) delete fIcon; };
	BBitmap *Icon() const { return fIcon; };
private:
	BBitmap *fIcon;
};

class TRemoteFileView : public BColumnListView
{
public:
	TRemoteFileView(const char *name);
	~TRemoteFileView();
	void MessageReceived(BMessage *msg);
	void MessageDropped(BMessage *msg, BPoint point);
	void KeyDown(const char *bytes, int32 numBytes);
	void MouseDown(BPoint point);
	uint32 GetSelectedEntries(BMessage *entries, BRect *rect);
	bool InitiateDrag(BPoint, bool);
	void DrawLatch(BView *view, BRect rect, LatchType type, BRow *row);
	void SetRemoteDir(const char *remoteDir);
	const char *RemoteDir() const;
	void SearchAndSelect(const char *word);
private:
	BString fCurrentRemoteDir;
	BString fISWord;
	BMessageRunner *fISReseter;
};


#endif
