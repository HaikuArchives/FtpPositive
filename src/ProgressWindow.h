#ifndef _PROGRESS_WINDOW_H_
#define _PROGRESS_WINDOW_H_

#include <Window.h>
#include <View.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <StatusBar.h>
#include <Button.h>
#include <interface/StringView.h>
#include <String.h>
#include <List.h>
#include <Rect.h>


#define PROGRESS_WINDOW_TITLE B_TRANSLATE("Transfer")
#define MSG_TRANSFER_CANCEL 'abor'


class TProgressItem
{
public:
	TProgressItem(bool upload, off_t remoteSize) {
		fUpload = upload;
		fRemoteSize = remoteSize;
	};
	~TProgressItem() {
	};
	bool IsUpload() const { return fUpload; };
	off_t RemoteSize() const { return fRemoteSize; };
private:
	bool fUpload;
	off_t fRemoteSize;
};

class TProgressView : public BView
{
public:
	TProgressView(BRect frame, const char *name);
	~TProgressView();
	void MessageReceived(BMessage *msg);
	void Pulse();
	
	void SetTo(const char *fileName, off_t size, bool upload);
	void SetValue(off_t currentValue);
	void Update();
	
	BStatusBar *StatusBar() const { return fStatusBar; };
	
private:
	BStringView *fFileNameView;
	BStatusBar *fStatusBar;
	BStringView *fAvgStringView;
	BStringView *fETAStringView;
	BButton *fCancelButton;
	float fCurrentValue;
	bigtime_t fBeginTime;
};

class TProgressWindow : public BWindow
{
public:
	TProgressWindow(BRect frame, const char *title, volatile bool *abortFlag);
	~TProgressWindow();
	
	bool QuitRequested();
	void MessageReceived(BMessage *msg);
	bool Abort();
	
	void SetProgressInfo(const char *name, bool upload, off_t size);
	void SetProgress(int64 trasferredSize);
	void Clear();
	
private:
	TProgressView *fProgressView;
	volatile bool *fAbortFlag;
};

#endif
