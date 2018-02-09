#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Entry.h>
#include <File.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <StringForSize.h>

#include "ProgressWindow.h"
#include "FTPLooper.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProgressWindow"

extern const char* kAppName;


// TProgressView

TProgressView::TProgressView(const char *name)
	:	BView(name, B_PULSE_NEEDED | B_SUPPORTS_LAYOUT)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		
	BFont font(be_bold_font);
	fFileNameView = new BStringView("FileNameView", "");
	fFileNameView->SetFont(&font);
	fFileNameView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fStatusBar = new BStatusBar("StatusBar", "FileName", " / 0");
	fStatusBar->SetTrailingText("0");
	fStatusBar->SetBarHeight(font.Size() + 1);
	
	fAvgStringView = new BStringView("AvgStringView", "999999.9KB/s");
	fAvgStringView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fETALabelView = new BStringView("ETALabel", B_TRANSLATE("Time left:"));
	fETALabelView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fETAStringView = new BStringView("ETAStringView", "99999:99");
	fETAStringView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	
	fCancelButton = new BButton(
		"CancelButton", B_TRANSLATE("Cancel"), new BMessage(MSG_TRANSFER_CANCEL));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_ITEM_SPACING)
		.Add(fFileNameView)
		.Add(fStatusBar)
		.AddGroup(B_HORIZONTAL)
			.Add(fETALabelView)
			.Add(fETAStringView)
			.AddGlue(10)
			.Add(fCancelButton)
		.End();

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

	char fileSize[B_PATH_NAME_LENGTH];
	string_for_size(size, fileSize, sizeof(fileSize));
	trailingLabel.Append(fileSize);

	BString trailingText;
	char currentSize[B_PATH_NAME_LENGTH];
	string_for_size((int64)fStatusBar->CurrentValue(), currentSize, sizeof(currentSize));
	trailingText.Append(currentSize);

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
	char currentSize[B_PATH_NAME_LENGTH];
	string_for_size((int64)fStatusBar->CurrentValue(), currentSize, sizeof(currentSize));
	trailingText.Append(currentSize);

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
				B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fProgressView = new TProgressView("TransferView");
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fProgressView);

	SetPulseRate(100000);
	fAbortFlag = abortFlag;
}

TProgressWindow::~TProgressWindow()
{
}

bool TProgressWindow::QuitRequested()
{
	if (CurrentMessage() == NULL) return true;
	if (!IsHidden())
		Hide();
	return false;
}

bool TProgressWindow::Abort()
{
	BAlert *alert = new BAlert(kAppName,
		B_TRANSLATE("Do you want to abort the transfer?\n"),
		B_TRANSLATE("Abort"), B_TRANSLATE("Continue"), NULL,
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
