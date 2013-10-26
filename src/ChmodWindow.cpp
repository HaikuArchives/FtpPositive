#include <stdlib.h>
#include <String.h>
#include "ChmodWindow.h"

enum {
	OK_CLICKED = 'okcl',
};

TChmodWindow::TChmodWindow(float ix, float iy, const char *title)
	:	BWindow(BRect(ix, iy, ix + 275, iy + 170), title,
			B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE)
{
	BView *bgView = new BView(Bounds(), "", 0, 0);
	bgView->SetViewColor(217,217,217);
	AddChild(bgView);
	
	float r = 40, w = 70, x = 100, ow = 80, gr = 140, ot = 200, xd = 15, yd = 15;
	bgView->AddChild(new BStringView(BRect(10, r, ow - 2, r + yd + 3), "", "Read:"));
	bgView->AddChild(new BStringView(BRect(10, w, ow - 2, w + yd + 3), "", "Write:"));
	bgView->AddChild(new BStringView(BRect(10, x, ow - 2, x + yd + 3), "", "Execute:"));
	bgView->AddChild(new BStringView(BRect(ow - 10, r - 20, ow + 30, r - 3), "", "Owner"));
	bgView->AddChild(new BStringView(BRect(gr - 10, r - 20, gr + 30, r - 3), "", "Group"));
	bgView->AddChild(new BStringView(BRect(ot - 10, r - 20, ot + 30, r - 3), "", "Other"));
	fOwnerRead  = new BCheckBox(BRect(ow, r, ow + xd, r + yd), "OwnerRead", "", NULL);
	fOwnerWrite = new BCheckBox(BRect(ow, w, ow + xd, w + yd), "OwnerWrite", "", NULL);
	fOwnerExec  = new BCheckBox(BRect(ow, x, ow + xd, x + yd), "OwnerExec", "", NULL);
	fGroupRead  = new BCheckBox(BRect(gr, r, gr + xd, r + yd), "GroupRead", "", NULL);
	fGroupWrite = new BCheckBox(BRect(gr, w, gr + xd, w + yd), "GroupWrite", "", NULL);
	fGroupExec  = new BCheckBox(BRect(gr, x, gr + xd, x + yd), "GroupROwner", "", NULL);
	fOtherRead  = new BCheckBox(BRect(ot, r, ot + xd, r + yd), "OtherRead", "", NULL);
	fOtherWrite = new BCheckBox(BRect(ot, w, ot + xd, w + yd), "OtherWrite", "", NULL);
	fOtherExec  = new BCheckBox(BRect(ot, x, ot + xd, x + yd), "OtherExec", "", NULL);
	bgView->AddChild(fOwnerRead);
	bgView->AddChild(fOwnerWrite);
	bgView->AddChild(fOwnerExec);
	bgView->AddChild(fGroupRead);
	bgView->AddChild(fGroupWrite);
	bgView->AddChild(fGroupExec);
	bgView->AddChild(fOtherRead);
	bgView->AddChild(fOtherWrite);
	bgView->AddChild(fOtherExec);
	fOKButton = new BButton(BRect(10, 140, 100, 160), "", "Change", new BMessage(OK_CLICKED));
	bgView->AddChild(fOKButton);
	fCancelButton = new BButton(BRect(170, 140, 260, 160), "", "Cancel", new BMessage(B_QUIT_REQUESTED));
	bgView->AddChild(fCancelButton);
	
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
