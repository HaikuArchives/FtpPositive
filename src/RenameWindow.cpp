#include <Application.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>

#include "RenameWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RenameWindow"

enum {
	OK_CLICKED = 'okcl',
};

TRenameWindow::TRenameWindow(BRect frame,
			const char *winTitle, const char *caption, const char *oldName)
	:	BWindow(BRect(0, 0, 400, 0), winTitle, B_DOCUMENT_WINDOW_LOOK,  B_MODAL_APP_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	frame.OffsetBy(50.0, 50.0);
	MoveTo(frame.LeftTop());

	fTextControl = new BTextControl("NewName", caption, oldName, NULL);
	fTextControl->SetText(oldName);

	fOKButton = new BButton("", B_TRANSLATE("OK"), new BMessage(OK_CLICKED));
	fCancelButton = new BButton("", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(this,B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_ITEM_SPACING)
			.Add(fTextControl)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_ITEM_SPACING, 0, B_USE_ITEM_SPACING, B_USE_ITEM_SPACING)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fOKButton)
			.AddGlue()
		.End()
		.View()->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	
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
