#include <stdio.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <Alert.h>
#include "ProgressWindow.h"
#include "FTPLooper.h"



// TProgressView

TProgressView::TProgressView(BRect frame, const char *name)
	:	BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_PULSE_NEEDED)
{
	SetViewColor(217, 217, 217);
	
	float right = Bounds().right - 5;
	
	fFileNameView = new BStringView(BRect(3, 2, right, 20),
		"FileNameView", "");
	AddChild(fFileNameView);
	
	fStatusBar = new BStatusBar(BRect(3, 22, right, 50),
		"StatusBar", "FileName", " / 0");
	fStatusBar->SetTrailingText("0");
	fStatusBar->SetBarHeight(13);
	AddChild(fStatusBar);
	
	fAvgStringView = new BStringView(BRect(5, 54, right / 2, 68), "AvgStringView", "999999.9KB/s");
	AddChild(fAvgStringView);
	
	fETAStringView = new BStringView(BRect(10 + right / 2, 54, right, 68), "ETAStringView", "99999:99");
	fETAStringView->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fETAStringView);
	
	fCancelButton = new BButton(BRect(right - 65, 69, right, 87),
		"CancelButton", "Cancel", new BMessage(MSG_TRANSFER_CANCEL));
	fCancelButton->ResizeTo(65, 20);
	AddChild(fCancelButton);
	
	fCurrentValue = 0;
}

TProgressView::~TProgressView()
{
}

void TProgressView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		default: BView::MessageReceived(msg);
	}
}

void TProgressView::Pulse()
{
	Update();
}

void TProgressView::SetTo(const char *fileName, int64 size, bool upload)
{
	fFileNameView->SetText(fileName);
	BString trailingLabel(" / ");
	trailingLabel << size;
	BString trailingText;
	trailingText << fStatusBar->CurrentValue();
	fCurrentValue = 0;
	fStatusBar->Reset("", trailingLabel.String());
	fStatusBar->Update(0, "", trailingText.String());
	fStatusBar->SetMaxValue(size);
	fAvgStringView->SetText("0 B/s");
	fETAStringView->SetText("∞");
	Invalidate();
	fBeginTime = system_time();
}

void TProgressView::Update()
{
	BString trailingText;
	trailingText << (int)fStatusBar->CurrentValue();
	float delta = fCurrentValue - fStatusBar->CurrentValue();
	fStatusBar->Update(delta, "", trailingText.String());
	
	BString strAvg, strEta;
	bigtime_t passed = system_time() - fBeginTime;
	
	float avg = 1000000 * fStatusBar->CurrentValue() / passed;
	uint32 eta = (uint32)((fStatusBar->MaxValue() - fStatusBar->CurrentValue()) / avg);
	
	if (fStatusBar->MaxValue() < fStatusBar->CurrentValue()) {
		strEta.SetTo("0:00");
	} else {
		uint32 sec = eta % 60, min = eta / 60;
		char t[100];
		sprintf(t, "%ld:%2.2ld", min, sec);
		strEta.SetTo(t);
	}
	
	if (avg < 1024) strAvg << avg << " B/s"; else
	if (avg < 1048576) strAvg << (avg / 1024) << " KB/s"; else
	if (avg < 1073741824) strAvg << (avg / 1048576) << " MB/s";
	
	fAvgStringView->SetText(strAvg.String());
	fETAStringView->SetText(strEta.String());
	
	Invalidate();
}

void TProgressView::SetValue(off_t currentValue)
{
	fCurrentValue = (float)currentValue;
}


// TProgressWindow

TProgressWindow::TProgressWindow(BRect frame, const char *title, volatile bool *abortFlag)
	:	BWindow(frame, title, B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
				B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	fProgressView = new TProgressView(Bounds(), "TransferView");
	AddChild(fProgressView);
	SetPulseRate(100000);
	fAbortFlag = abortFlag;
}

TProgressWindow::~TProgressWindow()
{
}

bool TProgressWindow::QuitRequested()
{
	if (CurrentMessage() == NULL) return true;
	if (!IsHidden()) Hide();
	return false;
}

bool TProgressWindow::Abort()
{
	BAlert *alert = new BAlert("FtpPositive", "Really cancel?",
							"Yes", "No", NULL,
							B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert->Go() == 0) {
		*fAbortFlag = true;
		return true;
	}
	return false;
}

void TProgressWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case FTP_DATA_RECEIVING: {
			int64 size;
			if (msg->FindInt64("transferred_size", &size) != B_OK) return;
			fProgressView->SetValue(size);
			break;
		}
		case MSG_TRANSFER_CANCEL: {
			Abort();
			break;
		}
		case FTP_LOG: {
			BString str, fileName;
			bool upload;
			int32 value;
			int64 size;
			if (msg->FindString("log", &str) == B_OK) fprintf(stderr, "%s", str.String());
			if (msg->FindInt32("value", &value) == B_OK) fProgressView->SetValue(value);
			msg->FindBool("upload", &upload);
			msg->FindInt64("size", &size);
			msg->FindString("name", &fileName);
			fProgressView->SetTo(fileName.String(), size, upload);
			break;
		}
		default: BWindow::MessageReceived(msg);
	}
}

void TProgressWindow::SetProgressInfo(const char *name, bool upload, off_t size)
{
	BMessage msg(FTP_LOG);
	msg.AddString("name", name);
	msg.AddBool("upload", true);
	msg.AddInt64("size", size);
	BMessenger(this).SendMessage(&msg);
}

void TProgressWindow::SetProgress(int64 trasferredSize)
{
	BMessage msg(FTP_DATA_RECEIVING);
	msg.AddInt64("transferred_size", trasferredSize);
	BMessenger(this).SendMessage(&msg);
}

void TProgressWindow::Clear()
{
	BMessage msg(FTP_LOG);
	msg.AddString("name", "");
	msg.AddBool("upload", true);
	msg.AddInt64("size", 0);
	BMessenger(this).SendMessage(&msg);
}
