#ifndef _BOOKMARK_WINDOW_H_
#define _BOOKMARK_WINDOW_H_

#include <Button.h>
#include <Entry.h>
#include <String.h>
#include <Window.h>

#include "OAEdit.h"


class TBookmarkWindow : public BWindow
{
public:
				TBookmarkWindow(BRect frame, const char *title, const char *dir);
				~TBookmarkWindow();
	void 		MessageReceived(BMessage *msg);
	bool 		QuitRequested();
	bool 		Go(const char *pathName, BEntry *entry);
	
private:
	volatile status_t fStatus;
	TOAEdit 	*fBookmarkName;
	TOAEdit 	*fHostAddress;
	TOAEdit 	*fPort;
	TOAEdit 	*fUsername;
	TOAEdit 	*fPassword;
	TOAEdit 	*fEncoder;
	TOAEdit 	*fRemotePath;
	TOAEdit 	*fLocalPath;
	BButton 	*fSaveButton;
	BButton 	*fSaveAndConnectButton;
	BString 	fBookmarkDir;
	BEntry 		fEntry;
	bool 		fNewBookmark;
	
	status_t 	Save();
	void 		EditChanged();
	void 		Load(const char *pathName);
};


#endif
