#ifndef _RENAME_WINDOW_H
#define _RENAME_WINDOW_H


#include <Button.h>
#include <String.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>


class TRenameWindow : public BWindow
{
public:
				TRenameWindow(BRect frame,
					const char *winTitle, const char *caption, const char *oldName);
				~TRenameWindow();
	bool 		QuitRequested();
	void 		MessageReceived(BMessage *msg);
	bool 		Go(BString *newName);
private:
	volatile status_t fStatus;
	BTextControl 	*fTextControl;
	BButton 		*fOKButton;
	BButton 		*fCancelButton;
};


#endif
