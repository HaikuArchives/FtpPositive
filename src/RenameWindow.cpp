#include <Application.h>
#include "RenameWindow.h"

enum {
	OK_CLICKED = 'okcl',
};

TRenameWindow::TRenameWindow(float x, float y,
			const char *winTitle, const char *caption, const char *oldName)
	:	BWindow(BRect(x, y, x + 340, y + 100), winTitle,
			B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE)
{
	BView *bgView = new BView(Bounds(), "", 0, 0);
	bgView->SetViewColor(217,217,217);
	AddChild(bgView);
	
	fTextControl = new BTextControl(BRect(15, 15, 320, 35), "NewName", caption, oldName, NULL);
	fTextControl->SetDivider(65);
	fTextControl->SetText(oldName);
	bgView->AddChild(fTextControl);
	fOKButton = new BButton(BRect(10, 70, 100, 90), "", "OK", new BMessage(OK_CLICKED));
	bgView->AddChild(fOKButton);
	fCancelButton = new BButton(BRect(230, 70, 320, 90), "", "Cancel", new BMessage(B_QUIT_REQUESTED));
	bgView->AddChild(fCancelButton);
	
	AddShortcut('W', 0, new BMessage(B_QUIT_REQUESTED));
	
	SetDefaultButton(fOKButton);
	fTextControl->MakeFocus(true);
	fStatus = B_BUSY;
}

TRenameWindow::~TRenameWindow()
{
}

bool TRenameWindow::QuitRequested()
{
	if (fStatus == B_BUSY) {
		fStatus = B_ERROR;
		return false;
	}
	return true;
}

void TRenameWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case OK_CLICKED:
			fStatus = B_OK;
			break;
		default: BWindow::MessageReceived(msg);
	}
}

bool TRenameWindow::Go(BString *newName)
{
	BWindow *window = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
	this->Show();
	while(this->fStatus == B_BUSY) {
		snooze(1000);
		window->UpdateIfNeeded();
	}
	newName->SetTo(fTextControl->Text());
	bool result = this->fStatus == B_OK;
	BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	return result;
}
