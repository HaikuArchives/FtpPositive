#ifndef _CHMOD_WINDOW_H
#define _CHMOD_WINDOW_H

#include <Button.h>
#include <CheckBox.h>
#include <Window.h>
#include <View.h>

#include <interface/StringView.h>

class TChmodWindow : public BWindow
{
public:
				TChmodWindow(BRect frame, const char *title);
				~TChmodWindow();
	bool 		QuitRequested();
	void 		MessageReceived(BMessage *msg);
	bool 		Go(uint32 mode, BString *newMode);
private:
	volatile status_t fStatus;
	BCheckBox 	*fOwnerRead;
	BCheckBox 	*fOwnerWrite;
	BCheckBox 	*fOwnerExec;
	BCheckBox 	*fGroupRead;
	BCheckBox 	*fGroupWrite;
	BCheckBox 	*fGroupExec;
	BCheckBox 	*fOtherRead;
	BCheckBox 	*fOtherWrite;
	BCheckBox 	*fOtherExec;
	BButton 	*fOKButton;
	BButton 	*fCancelButton;
};


#endif
