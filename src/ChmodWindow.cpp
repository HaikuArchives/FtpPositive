#include <stdlib.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <String.h>
#include <LayoutBuilder.h>
#include "ChmodWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ChmodWindow"

enum {
	OK_CLICKED = 'okcl',
};

TChmodWindow::TChmodWindow(float ix, float iy, const char *title)
	:	BWindow(BRect(ix, iy, 0,0), title,
			B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	
	fOwnerRead  = new BCheckBox("OwnerRead", "", NULL);
	fOwnerWrite = new BCheckBox("OwnerWrite", "", NULL);
	fOwnerExec  = new BCheckBox("OwnerExec", "", NULL);
	fGroupRead  = new BCheckBox("GroupRead", "", NULL);
	fGroupWrite = new BCheckBox("GroupWrite", "", NULL);
	fGroupExec  = new BCheckBox("GroupROwner", "", NULL);
	fOtherRead  = new BCheckBox("OtherRead", "", NULL);
	fOtherWrite = new BCheckBox("OtherWrite", "", NULL);
	fOtherExec  = new BCheckBox("OtherExec", "", NULL);
	fOKButton = new BButton("", B_TRANSLATE("Change"), new BMessage(OK_CLICKED));
	fCancelButton = new BButton("", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));
	BLayoutBuilder::Group<>(this,B_VERTICAL,B_USE_ITEM_SPACING)
		.SetInsets(B_USE_ITEM_SPACING)
		.AddGrid(B_USE_ITEM_SPACING,B_USE_ITEM_SPACING)
			.Add(new BStringView("", B_TRANSLATE("Read:")),0,1)
			.Add(new BStringView("", B_TRANSLATE("Write:")),0,2)
			.Add(new BStringView("", B_TRANSLATE("Execute:")),0,3)
			.Add(new BStringView("", B_TRANSLATE("Owner")),1,0)
			.Add(new BStringView("", B_TRANSLATE("Group")),2,0)
			.Add(new BStringView("", B_TRANSLATE("Other")),3,0)
			.Add(fOwnerRead,1,1)
			.Add(fOwnerWrite,1,2)
			.Add(fOwnerExec,1,3)
			.Add(fGroupRead,2,1)
			.Add(fGroupWrite,2,2)
			.Add(fGroupExec,2,3)
			.Add(fOtherRead,3,1)
			.Add(fOtherWrite,3,2)
			.Add(fOtherExec,3,3)
		.End()
		.AddStrut(B_USE_BIG_SPACING)
		.AddGroup(B_HORIZONTAL,B_USE_ITEM_SPACING)
			.Add(fCancelButton)
			.Add(fOKButton)
		.End()
		.View()->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	AddShortcut('W', 0, new BMessage(B_QUIT_REQUESTED));
	
	fStatus = B_BUSY;
}

TChmodWindow::~TChmodWindow()
{
}

bool TChmodWindow::QuitRequested()
{
	if (fStatus == B_BUSY) {
		fStatus = B_ERROR;
		return false;
	}
	return true;
}

void TChmodWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case OK_CLICKED:
			fStatus = B_OK;
			break;
		default: BWindow::MessageReceived(msg);
	}
}

bool TChmodWindow::Go(uint32 mode, BString *newMode)
{
	BWindow *window = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
	
	fOwnerRead->SetValue(mode & 0400);
	fOwnerWrite->SetValue(mode & 0200);
	fOwnerExec->SetValue(mode & 0100);
	fGroupRead->SetValue(mode & 040);
	fGroupWrite->SetValue(mode & 020);
	fGroupExec->SetValue(mode & 010);
	fOtherRead->SetValue(mode & 04);
	fOtherWrite->SetValue(mode & 02);
	fOtherExec->SetValue(mode & 01);
	
	this->Show();
	while(this->fStatus == B_BUSY) {
		snooze(1000);
		window->UpdateIfNeeded();
	}
	
	uint32 ow = 0, gr = 0, ot = 0;
	if (fOwnerRead->Value()) ow |= 04;
	if (fOwnerWrite->Value()) ow |= 02;
	if (fOwnerExec->Value()) ow |= 01;
	if (fGroupRead->Value()) gr |= 04;
	if (fGroupWrite->Value()) gr |= 02;
	if (fGroupExec->Value()) gr |= 01;
	if (fOtherRead->Value()) ot |= 04;
	if (fOtherWrite->Value()) ot |= 02;
	if (fOtherExec->Value()) ot |= 01;
	newMode->SetTo("");
	*newMode << ow << gr << ot;
	
	bool result = this->fStatus == B_OK;
	BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	return result;
}
