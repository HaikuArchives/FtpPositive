#ifndef _FTPWINDOW_H_
#define _FTPWINDOW_H_


#include <Button.h>
#include <CheckBox.h>
#include <Directory.h>
#include <Entry.h>
#include <FilePanel.h>
#include <interface/StringView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <String.h>
#include <SupportDefs.h>
#include <TextControl.h>
#include <TextView.h>
#include <ToolBar.h>
#include <View.h>
#include <Window.h>

#include <MessageRunner.h>
#include "RemoteFileView.h"
#include "FTPLooper.h"
#include "ConfigFile.h"
#include "DirentParser.h"
#include "SimplePictureButton.h"
#include "EncoderAddon.h"
#include "EncoderAddonManager.h"
#include "StatusView.h"


enum {
	MSG_BOOKMARK_SELECTED = 'mbse',
	MSG_NEW_BOOKMARK = 'mnbk',
	MSG_SHOW_BOOKMARK = 'msbk',
	MSG_ENCODER_SELECTED = 'mesl',
	MSG_CANCEL = 'msca',
	MSG_ENTRY_DOUBLE_CLICKED = 'medc',
	MSG_REMOTE_PATH_CHANGED = 'rpch',
	MSG_BACKWARD_CLICKED = 'mgbk',
	MSG_FORWARD_CLICKED = 'mgfw',
	MSG_GOPARENT_CLICKED = 'mgpr',
	MSG_RELOAD_CLICKED = 'mrcl',
	MSG_REMOTE_FILE_DROPPED = 'mrfd',
	MSG_RENAME = 'mren',
	MSG_MKDIR = 'mmkd',
	MSG_COPYURL = 'mcpu',
	MSG_CHMOD = 'mchm',
	MSG_DELETE = 'mdel',
	MSG_MENU_OPEN = 'mopn',
	MSG_DOWNLOAD_CLICKED = 'mdlc',
	MSG_UPLOAD_CLICKED = 'mupc',
	MSG_READY= 'mrdy'
};


class TLogView : public BTextView
{
public:
	TLogView(const char *name);
	~TLogView();

	TLogView& operator<<(const char *string);
	TLogView& operator<<(int val);

protected:
	void InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs);
};

class TFTPWindow : public BWindow
{
public:
			TFTPWindow(BRect frame, const char *name);
			~TFTPWindow();

	void	FrameResized(float newWidth, float newHeight);
	void	MessageReceived(BMessage *msg);
	bool	QuitRequested();

private:
	TFtpLooper		*fFtpLooper;
	TConfigFile		*fBookmarkConfig;

	TLogView		*fLogView;
	TRemoteFileView *fRemoteFileView;
	BSplitView		*fSplitView;
	BToolBar		*mainToolBar;
	BTextControl	*fRemoteDirView;
	StatusView		*fItemCountView;
	BStringView		*fStatusView;
	BMenu			*fFileMenu;
	BMenu			*fConnectMenu;
	BMenu			*fCommandMenu;
	BMenu			*fEncodingMenu;
	BMenuItem		*fNoEncoder;
	BCheckBox		*fUseThisConnection;
	BPopUpMenu		*fPopUpMenu;

	BFilePanel		fOpenPanel;

	BString			fCurrentRemoteDir;
	BString			fLocalDir;

	BString			fHost;
	uint16			fPort;
	BString			fUsername;
	BString			fPassword;

	BList			fWalkHistory;
	int32			fCurrentWalkHistoryIndex;
	int32			fNavigating;

	void			ClearBookmarks();
	status_t		LoadBookmarks(BDirectory *dir, BMenu *menu);
	status_t 		AddBookmark(BEntry *entry, BMenu *menu);

	void 			FtpReportMsgIncoming(BMessage *msg);

	void 			AddRemoteFileItem(const char *name, int64 size,
						const char *date, const char *perm, const char *owner,
						const char *group);

	void 			SetBusy(bool busy);
	void 			Clear();

	void 			BookmarkMenuReload();
	void 			ShowBookmark() const;
	void 			EncoderChanged();
	void 			BookmarkSelected(const char *pathName);
	void 			RemoteFileDoubleClicked();

	status_t 		Connect(const char *remoteDir);
	status_t 		ReconnectIfDisconnected();
	void 			PasvList();
	void 			Chdir(const char *dir);
	void 			DirlistChanged(BMessage *msg);

	void 			RemotePathChanged();
	void 			BackwardClicked();
	void 			ForwardClicked();
	void 			GoParentClicked();
	void 			ReloadClicked();

	void 			Rename();
	void 			Mkdir();
	void 			CopyUrl();
	void 			Chmod();
	void 			Delete();

	void 			MenuOpen();

	void			RemoteFileDropped(BMessage *msg);
	void 			DragReply(BMessage *msg);
	void 			DownloadClicked();
	void 			Download(BMessage *msg, entry_ref *localDir);

	status_t 		ScanEntries(BDirectory *dir, BMessage *entries);
	void 			UploadClicked();
	void 			Upload(BMessage *msg);
};

#endif
