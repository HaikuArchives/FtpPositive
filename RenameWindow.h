#ifndef _RENAME_WINDOW_H
#define _RENAME_WINDOW_H


#include <Window.h>
#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include <String.h>


class TRenameWindow : public BWindow
{
public:
	TRenameWindow(float x, float y,
		const char *winTitle, const char *caption, const char *oldName);
	~TRenameWindow();
	bool QuitRequested();
	void MessageReceived(BMessage *msg);
	bool Go(BString *newName);
private:
	volatile status_t fStatus;
	BTextControl *fTextControl;
	BButton *fOKButton;
	BButton *fCancelButton;
};


#endif
