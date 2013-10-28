#include <Application.h>
#include <Bitmap.h>
#include <Alert.h>
#include <Messenger.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <OS.h>
#include <Entry.h>
#include <Alert.h>
#include "FtpPositive.h"
#include "FTPWindow.h"
#include "BookmarkWindow.h"
#include "MimeDB.h"
#include "RenameWindow.h"
#include "ChmodWindow.h"

static const rgb_color kSelectionColor = {0xd6,0xe8,0xda,0xff};
static const rgb_color kBackgroundColor = {0xff,0xff,0xff,0xff};


// ----------------------------------- TLogView -------------------------------------

TLogView::TLogView(BRect rect, const char *name, BRect textRect)
	:	BTextView(rect, name, textRect, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW)
{
	this->MakeEditable(false);
	this->SetWordWrap(true);
	this->SetStylable(true);
}

TLogView::~TLogView()
{
}

void TLogView::FrameResized(float width, float height)
{
	BRect rect(this->TextRect());
	rect.right = width;
	this->SetTextRect(rect);
	this->BTextView::FrameResized(width, height);
}

void TLogView::InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs)
{
	BString str(text, length);
	str.ReplaceAll("\r\n", "\n");
	BTextView::InsertText(str.String(), str.Length(), offset, runs);
}

TLogView& TLogView::operator<<(const char *string)
{
	int32 len = this->TextLength();
	this->Select(len, len);
	this->Insert(string);
	this->ScrollToSelection();
	return *this;
}

TLogView& TLogView::operator<<(int val)
{
	BString s;
	s << val;
	return *this << s.String();
}


// ----------------------------------- TFTPWindow -------------------------------------

TFTPWindow::TFTPWindow(BRect frame, const char *name)
	:	BWindow(frame, name, B_DOCUMENT_WINDOW, 0)
{
	fFtpLooper = NULL;
	fBookmarkConfig = NULL;
	
	float minW, minH, maxW, maxH;
	this->GetSizeLimits(&minW, &maxW, &minH, &maxH);
	this->SetSizeLimits(280, maxW, 250, maxH);
	
	BRect rect;
	
	BView *bgview = new BView(Bounds(), "BGView",
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS);
	bgview->SetViewColor(217, 217, 217);
	this->AddChild(bgview);
	
	// Main Menu
	BMenuBar *mainMenu = new BMenuBar(BRect(), "MainMenuBar");
	bgview->AddChild(mainMenu);
	
	// Forward, Backward, Upfolder
	rect.left = 5;
	rect.right = rect.left + 24;
	rect.top = mainMenu->Frame().bottom + 5;
	rect.bottom = rect.top + 24;
	fBackward = new TSimplePictureButton(rect, "Backward", 
		"NAVIGATION_BACKWARD_UP", 'BMP ',
		"NAVIGATION_BACKWARD_DOWN", 'BMP ',
		"NAVIGATION_BACKWARD_DISABLED", 'BMP ',
		new BMessage(MSG_BACKWARD_CLICKED));
	bgview->AddChild(fBackward);
	rect.OffsetBy(25, 0);
	fForward = new TSimplePictureButton(rect, "Forward", 
		"NAVIGATION_FORWARD_UP", 'BMP ',
		"NAVIGATION_FORWARD_DOWN", 'BMP ',
		"NAVIGATION_FORWARD_DISABLED", 'BMP ',
		new BMessage(MSG_FORWARD_CLICKED));
	bgview->AddChild(fForward);
	rect.OffsetBy(25, 0);
	fGoparent = new TSimplePictureButton(rect, "GoParent", 
		"NAVIGATION_GOPARENT_UP", 'BMP ',
		"NAVIGATION_GOPARENT_DOWN", 'BMP ',
		"NAVIGATION_GOPARENT_DISABLED", 'BMP ',
		new BMessage(MSG_GOPARENT_CLICKED));
	bgview->AddChild(fGoparent);
	rect.OffsetBy(25, 0);
	fReload = new TSimplePictureButton(rect, "Reload", 
		"NAVIGATION_RELOAD_UP", 'BMP ',
		"NAVIGATION_RELOAD_DOWN", 'BMP ',
		"NAVIGATION_RELOAD_DISABLED", 'BMP ',
		new BMessage(MSG_RELOAD_CLICKED));
	bgview->AddChild(fReload);
	
	// Remote Path View
	rect.left = 110;
	rect.right = bgview->Bounds().right - 25;
	rect.top = mainMenu->Frame().bottom + 7;
	rect.bottom = rect.top + 12;
	fRemoteDirView = new BTextControl(rect, "RemoteDirView", "Remote Dir :", "",
		new BMessage(MSG_REMOTE_PATH_CHANGED), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fRemoteDirView->SetDivider(fRemoteDirView->StringWidth("Remote Dir :"));
	bgview->AddChild(fRemoteDirView);
	
	// Cancel Button
	rect.left = bgview->Bounds().right - 20;
	rect.right = rect.left + 17;
	rect.top = mainMenu->Frame().bottom + 7;
	rect.bottom = rect.top + 20;
	fCancelButton = new BButton(rect, "CancelButton", "X",
		new BMessage(MSG_CANCEL), B_FOLLOW_RIGHT);
	bgview->AddChild(fCancelButton);
	fCancelButton->ResizeTo(17, 18);
	
	// Remote File View
	TStringColumn *nameColumn = new TStringColumn("Name", 150, 60, INT_MAX, 0);
	BStringColumn *intNameColumn = new BStringColumn("Internal Name", 150, 60, INT_MAX, 0);
	rect.left = 5;
	rect.right = bgview->Bounds().right - 5;
	rect.top = fRemoteDirView->Frame().bottom + 7;
	rect.bottom = bgview->Bounds().bottom - 107;
	fRemoteFileView = new TRemoteFileView(rect, "RemoteFileView");
	fRemoteFileView->AddColumn(nameColumn, CLM_NAME);
	fRemoteFileView->AddColumn(intNameColumn, CLM_INTERNAL_NAME);
	fRemoteFileView->AddColumn(new BSizeColumn("Size", 80, 60, INT_MAX, B_ALIGN_RIGHT), CLM_SIZE);
	fRemoteFileView->AddColumn(new BStringColumn("Date", 100, 60, INT_MAX, 0), CLM_DATE);
	fRemoteFileView->AddColumn(new BStringColumn("Permission", 90, 60, INT_MAX, 0), CLM_PERMISSION);
	fRemoteFileView->AddColumn(new BStringColumn("Owner", 70, 60, INT_MAX, 0), CLM_OWNER);
	fRemoteFileView->AddColumn(new BStringColumn("Group", 70, 60, INT_MAX, 0), CLM_GROUP);
	fRemoteFileView->SetTarget(this);
	fRemoteFileView->SetInvocationMessage(new BMessage(MSG_ENTRY_DOUBLE_CLICKED));
	bgview->AddChild(fRemoteFileView);
	intNameColumn->SetVisible(false);
	fRemoteFileView->SetSortingEnabled(true);
	fRemoteFileView->SetSortColumn(nameColumn, false, true);
	fRemoteFileView->SetSelectionColor(kSelectionColor);
	fRemoteFileView->SetBackgroundColor(kBackgroundColor);
	fRemoteFileView->SetLatchWidth(24);
	
	fItemCountView = new BStringView(BRect(0, 0, 80, B_H_SCROLL_BAR_HEIGHT), "StatusView", "0 items", B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fRemoteFileView->AddStatusView(fItemCountView);
	fItemCountView->SetViewColor(217, 217, 217);
	
	// CheckBox (Use This Connection)
	rect.right = bgview->Bounds().right - B_V_SCROLL_BAR_WIDTH - 5;
	rect.left = rect.right - bgview->StringWidth("Use this connection") - 20;
	rect.top = bgview->Bounds().bottom - 19;
	rect.bottom = mainMenu->Bounds().bottom;
	fUseThisConnection = new BCheckBox(BRect(rect),
		"UseThisConnection", "Use this connection", NULL, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	bgview->AddChild(fUseThisConnection);
	fUseThisConnection->SetValue(1);
	
	// Log View
	rect.left = 5;
	rect.right = bgview->Bounds().right - B_V_SCROLL_BAR_WIDTH - 5;
	rect.top = bgview->Bounds().bottom - 95;
	rect.bottom = bgview->Bounds().bottom - 20;
	fLogView = new TLogView(rect, "LogView", BRect(2, 2, rect.Width(), rect.Height()));
	BScrollView *logScrollView = new BScrollView("logScrollView", fLogView,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW, false, true);
	bgview->AddChild(logScrollView);
	
	// Status View
	rect.left = 5;
	rect.right = fUseThisConnection->Frame().left - 1;
	rect.top = logScrollView->Frame().bottom + 1;
	rect.bottom = bgview->Bounds().bottom;
	fStatusView = new BStringView(rect, "StatusView", "", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	bgview->AddChild(fStatusView);
	
	// File Menu
	fFileMenu = new BMenu("File");
	mainMenu->AddItem(fFileMenu);
	fFileMenu->AddItem(new BMenuItem("About FtpPositive", new BMessage(B_ABOUT_REQUESTED)));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));
	
	// Connect Menu
	fConnectMenu = new BMenu("Connect");
	mainMenu->AddItem(fConnectMenu);
	fConnectMenu->AddItem(new BMenuItem("New Bookmark", new BMessage(MSG_NEW_BOOKMARK), 'N'));
	fConnectMenu->AddItem(new BMenuItem("Show Bookmarks", new BMessage(MSG_SHOW_BOOKMARK), 'B'));
	fConnectMenu->AddSeparatorItem();
	
	BDirectory dir(TFtpPositive::GetBookmarksDir());
	LoadBookmarks(&dir, fConnectMenu);
	
	// Command Menu
	fCommandMenu = new BMenu("Command");
	mainMenu->AddItem(fCommandMenu);
	fCommandMenu->AddItem(new BMenuItem("Download", new BMessage(MSG_DOWNLOAD_CLICKED), 'D'));
	fCommandMenu->AddItem(new BMenuItem("Upload", new BMessage(MSG_UPLOAD_CLICKED), 'U'));
	fCommandMenu->AddItem(new BMenuItem("Delete", new BMessage(MSG_DELETE), 'T'));
	fCommandMenu->AddItem(new BMenuItem("Rename/Move", new BMessage(MSG_RENAME), 'E'));
	fCommandMenu->AddItem(new BMenuItem("Create Directory", new BMessage(MSG_MKDIR), 'M'));
	fCommandMenu->AddItem(new BMenuItem("Change Permissions", new BMessage(MSG_CHMOD), 'J'));
	fCommandMenu->AddSeparatorItem();
	fCommandMenu->AddItem(new BMenuItem("Back", new BMessage(MSG_BACKWARD_CLICKED), B_LEFT_ARROW));
	fCommandMenu->AddItem(new BMenuItem("Forward", new BMessage(MSG_FORWARD_CLICKED), B_RIGHT_ARROW));
	fCommandMenu->AddItem(new BMenuItem("Parent Dir", new BMessage(MSG_GOPARENT_CLICKED), B_UP_ARROW));
	fCommandMenu->AddItem(new BMenuItem("Refresh", new BMessage(MSG_RELOAD_CLICKED), '.'));
	
	// Encoding Menu
	fEncodingMenu = new BMenu("Encoding");
	mainMenu->AddItem(fEncodingMenu);
	fEncodingMenu->SetRadioMode(true);
	fNoEncoder = new BMenuItem("None", new BMessage(MSG_ENCODER_SELECTED));
	fEncodingMenu->AddItem(fNoEncoder);
	fEncodingMenu->AddSeparatorItem();
	fNoEncoder->SetMarked(true);
	for (int32 i = 0; i < encoder_addon_manager->CountAddons(); i++) {
		fEncodingMenu->AddItem(new BMenuItem(encoder_addon_manager->NameAt(i), new BMessage(MSG_ENCODER_SELECTED)));
	}
	
	// PopupMenu
	fPopUpMenu = new BPopUpMenu("PopUpMenu");
	fPopUpMenu->SetRadioMode(false);
	fPopUpMenu->AddItem(new BMenuItem("Back", new BMessage(MSG_BACKWARD_CLICKED), B_LEFT_ARROW));
	fPopUpMenu->AddItem(new BMenuItem("Forward", new BMessage(MSG_FORWARD_CLICKED), B_RIGHT_ARROW));
	fPopUpMenu->AddItem(new BMenuItem("Parent Dir", new BMessage(MSG_GOPARENT_CLICKED), B_UP_ARROW));
	fPopUpMenu->AddItem(new BMenuItem("Refresh", new BMessage(MSG_RELOAD_CLICKED), '.'));
	fPopUpMenu->AddSeparatorItem();
	fPopUpMenu->AddItem(new BMenuItem("Download", new BMessage(MSG_DOWNLOAD_CLICKED), 'D'));
	fPopUpMenu->AddItem(new BMenuItem("Upload", new BMessage(MSG_UPLOAD_CLICKED), 'U'));
	fPopUpMenu->AddItem(new BMenuItem("Delete",  new BMessage(MSG_DELETE), 'T'));
	fPopUpMenu->AddItem(new BMenuItem("Rename/Move", new BMessage(MSG_RENAME), 'E'));
	fPopUpMenu->AddItem(new BMenuItem("Create Directory", new BMessage(MSG_MKDIR), 'M'));
	fPopUpMenu->AddItem(new BMenuItem("Change Permissions", new BMessage(MSG_CHMOD), 'J'));
	
	// 
	*fLogView << "Welcome to FtpPositive Version " VERSION "\n" COPY "\n";
	fRemoteFileView->MakeFocus(true);
	
	fStatusView->SetText("Idle.");
	fCommandMenu->SetEnabled(false);
	fPopUpMenu->SetEnabled(false);
	fUseThisConnection->SetEnabled(false);
	fRemoteDirView->SetEnabled(false);
	fBackward->SetEnabled(false);
	fForward->SetEnabled(false);
	fGoparent->SetEnabled(false);
	fReload->SetEnabled(false);
}


TFTPWindow::~TFTPWindow()
{
	BString s;
	s.SetTo(""); s << (int)Frame().left;   app_config->Write("frame_left",   s.String());
	s.SetTo(""); s << (int)Frame().top;    app_config->Write("frame_top",    s.String());
	s.SetTo(""); s << (int)Frame().right;  app_config->Write("frame_right",  s.String());
	s.SetTo(""); s << (int)Frame().bottom; app_config->Write("frame_bottom", s.String());
	Clear();
}

void TFTPWindow::Clear()
{
	fNavigating = 0;
	fCurrentWalkHistoryIndex = -1;
	for(int32 i = 0; i < fWalkHistory.CountItems(); i++) {
		BString *dir = (BString *)fWalkHistory.ItemAt(i);
		delete dir;
	}
	fWalkHistory.MakeEmpty();
	
	if (fBookmarkConfig) {
		delete fBookmarkConfig;
		fBookmarkConfig = NULL;
	}
	
	fRemoteFileView->Clear();
	fRemoteDirView->SetText("");
	fCurrentRemoteDir.SetTo("");
	fRemoteFileView->SetRemoteDir("");
	fItemCountView->SetText("0 items");
	this->SetTitle("FtpPositive");
	
	fStatusView->SetText("Suspended.");
}

bool TFTPWindow::QuitRequested()
{
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}

void TFTPWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case FTP_REPORT: FtpReportMsgIncoming(msg); break;
		case MSG_SHOW_BOOKMARK: ShowBookmark(); break;
		case MSG_NEW_BOOKMARK: BookmarkSelected(NULL); break;
		case MSG_BOOKMARK_SELECTED: {
			BString path;
			if (msg->FindString("entry", &path) != B_OK) return;
			BookmarkSelected(path.String());
			break;
		}
		case MSG_ENCODER_SELECTED: EncoderChanged(); break;
		case MSG_REMOTE_PATH_CHANGED: RemotePathChanged(); break;
		case MSG_CANCEL: if (fFtpLooper) fFtpLooper->Abort(); break;
		case FTP_LOG: {
			BString str;
			if (msg->FindString("log", &str) == B_OK) *fLogView << str.String();
			if (msg->FindString("ftp:statusMsg", &str) == B_OK) fStatusView->SetText(str.String());
			break;
		}
		case MSG_ENTRY_DOUBLE_CLICKED: RemoteFileDoubleClicked(); break;
		case B_ABOUT_REQUESTED:        be_app->AboutRequested(); break;
		case MSG_BACKWARD_CLICKED:     BackwardClicked(); break;
		case MSG_FORWARD_CLICKED:      ForwardClicked(); break;
		case MSG_GOPARENT_CLICKED:     GoParentClicked(); break;
		case MSG_RELOAD_CLICKED:       ReloadClicked(); break;
		case MSG_RENAME:               Rename(); break;
		case MSG_MKDIR:                Mkdir(); break;
		case MSG_CHMOD:                Chmod(); break;
		case MSG_DELETE:               Delete(); break;
		case B_SIMPLE_DATA:            RemoteFileDropped(msg); break;
		case B_REPLY:                  DragReply(msg); break;
		case MSG_MENU_OPEN:            MenuOpen(); break;
		case MSG_DOWNLOAD_CLICKED:     DownloadClicked(); break;
		case MSG_UPLOAD_CLICKED:       UploadClicked(); break;
		default: {
//			msg->PrintToStream();
			BWindow::MessageReceived(msg);
		}
	}
}

void TFTPWindow::FtpReportMsgIncoming(BMessage *msg)
{
	int32 requset;
	BString statusMsg, logMsg;
	bool busy;
	msg->FindInt32("ftp:requset", &requset);
	if (msg->FindString("ftp:statusMsg", &statusMsg) == B_OK) fStatusView->SetText(statusMsg.String());
	if (msg->FindString("ftp:logMsg", &logMsg) == B_OK) *fLogView << logMsg.String();
	if (msg->FindBool("ftp:busy", &busy) == B_OK) SetBusy(busy);
	switch(requset) {
		case REQ_KILL_ME: {
			if (fFtpLooper) {
				fFtpLooper->Abort();
				BMessenger(fFtpLooper).SendMessage(B_QUIT_REQUESTED);
				//->Lock();
//				fFtpLooper->Quit();
				fFtpLooper = NULL;
			}
			return;
		}
		case REQ_WALKED: {
			BString xpwd_result;
			if (msg->FindString("xpwd", &xpwd_result) == B_OK) {
				BString utf8remoteDir;
				encoder_addon_manager->ConvertToLocalName(xpwd_result.String(), &utf8remoteDir);
				fRemoteDirView->SetText(utf8remoteDir.String());
				fCurrentRemoteDir.SetTo(xpwd_result.String());
				fRemoteFileView->SetRemoteDir(xpwd_result.String());
				if (fNavigating) {
					// forward / backward
					if (fNavigating < 2) fCurrentWalkHistoryIndex += fNavigating;
					fNavigating = 0;
				} else {
					// It is engraved on the history.
					for(int32 i = fWalkHistory.CountItems() - 1; i > fCurrentWalkHistoryIndex; i--) {
						BString *dir = (BString *)fWalkHistory.ItemAt(i);
						delete dir;
						fWalkHistory.RemoveItem(i);
					}
					fWalkHistory.AddItem(new BString(xpwd_result.String()));
					fCurrentWalkHistoryIndex = fWalkHistory.CountItems() - 1;
				}
			}
			DirlistChanged(msg);
			return;
		}
	}
	return;
}

void TFTPWindow::AddRemoteFileItem(const char *name, uint32 size,
	const char *date, const char *perm, const char *owner, const char *group)
{
	// ディレクトリ、またはファイルタイプのアイコンイメージを取得
	BBitmap *icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);
	if (strlen(perm) > 0) {
		if (perm[0] == 'd') {
			app_mimedb->GetMimeIcon("application/x-vnd.Be-directory", icon, B_MINI_ICON);
		} else {
			app_mimedb->GetExtensionIcon(name, icon, B_MINI_ICON);
		}
	} else {
		app_mimedb->GetExtensionIcon(name, icon, B_MINI_ICON);
	}
	
	// ファイル名・オーナー名・グループ名文字コード変換
	BMenuItem *encMenuItem = fEncodingMenu->FindMarked();
	if (encMenuItem != NULL) encoder_addon_manager->SetEncoder(encMenuItem->Label());
	BString utf8name(name), utf8owner(owner), utf8group(group);
	encoder_addon_manager->ConvertToLocalName(name, &utf8name);
	encoder_addon_manager->ConvertToLocalName(owner, &utf8owner);
	encoder_addon_manager->ConvertToLocalName(group, &utf8group);
	
	// リスト表示
	TIconRow *row = new TIconRow(icon);
	row->SetField(new BStringField(utf8name.String()), CLM_NAME);
	row->SetField(new BStringField(name), CLM_INTERNAL_NAME);
	row->SetField(new BSizeField(size), CLM_SIZE);
	row->SetField(new BStringField(date), CLM_DATE);
	row->SetField(new BStringField(perm), CLM_PERMISSION);
	row->SetField(new BStringField(utf8owner.String()), CLM_OWNER);
	row->SetField(new BStringField(utf8group.String()), CLM_GROUP);
	fRemoteFileView->AddRow(row);
}

void TFTPWindow::SetBusy(bool busy)
{
	fRemoteDirView->SetEnabled(!busy);
	fConnectMenu->SetEnabled(!busy);
	fCommandMenu->SetEnabled(!busy);
	fPopUpMenu->SetEnabled(!busy);
	fEncodingMenu->SetEnabled(!busy);
	fCancelButton->SetEnabled(busy);
//	fUseThisConnection->SetEnabled(!busy);
	fBackward->SetEnabled(!busy);
	fForward->SetEnabled(!busy);
	fGoparent->SetEnabled(!busy);
	fReload->SetEnabled(!busy);
}

void TFTPWindow::ShowBookmark() const
{
	BEntry entry(TFtpPositive::GetBookmarksDir());
	if (entry.InitCheck() != B_OK) return;
	
	BMessage msg(B_REFS_RECEIVED);
	entry_ref ref;
	if (entry.GetRef(&ref) != B_OK) return;
	
	msg.AddRef("refs", &ref);
	BMessenger("application/x-vnd.Be-TRAK").SendMessage(&msg);
}

void TFTPWindow::ClearBookmarks()
{
	for(int32 i = fConnectMenu->CountItems() - 1 ; i >= 3; i--) {
		fConnectMenu->RemoveItem(i);
	}
}

status_t TFTPWindow::LoadBookmarks(BDirectory *dir, BMenu *menu)
{
	status_t e;
	BEntry entry;
	char leafName[B_FILE_NAME_LENGTH];
	if ((e = dir->InitCheck()) != B_OK) return e;
	
	while (dir->GetNextEntry(&entry) == B_OK) {
		entry.GetName(leafName);
		if (entry.IsDirectory()) {
			BDirectory ndir(dir, leafName);
			BMenu *subMenu = new BMenu(leafName);
			menu->AddItem(subMenu);
			LoadBookmarks(&ndir, subMenu);
		} else {
			AddBookmark(&entry, menu);
		}
	}
	
	return B_OK;
}

status_t TFTPWindow::AddBookmark(BEntry *entry, BMenu *menu)
{
	BPath path(entry);
	TConfigFile conf(path.Path());
	BString host;
	conf.Read("host", &host, "");
	if (host.Length() == 0) return B_ERROR;
	
	BMessage *msg = new BMessage(MSG_BOOKMARK_SELECTED);
	msg->AddString("entry", path.Path());
	BMenuItem *item = new BMenuItem(path.Leaf(), msg);
	menu->AddItem(item);
	
	return B_OK;
}

void TFTPWindow::EncoderChanged()
{
	BMenuItem *encMenuItem = fEncodingMenu->FindMarked();
	if (encMenuItem == NULL) return;
	
	fp_text_convert_t encoderAddonFunc = encoder_addon_manager->FuncAt(encMenuItem->Label());
	
	for(int32 i = 0; i < fRemoteFileView->CountRows(); i++) {
		BRow *row = fRemoteFileView->RowAt(i);
		BStringField *nameField = (BStringField *)row->GetField(CLM_NAME);		
		BStringField *intNameField = (BStringField *)row->GetField(CLM_INTERNAL_NAME);	
			
		// ファイル名文字コード変換
		BString utf8name(intNameField->String());
		if (encoderAddonFunc != NULL) encoderAddonFunc(intNameField->String(), &utf8name, true);
		nameField->SetString(utf8name.String());
	}
	fRemoteFileView->Refresh();
}

void TFTPWindow::BookmarkSelected(const char *pathName)
{
	BRect rect(Frame());
	BEntry entry;
	bool go = (new TBookmarkWindow(rect.left + 50, rect.top + 50, "New Bookmark", TFtpPositive::GetBookmarksDir().String()))->Go(pathName, &entry);
	ClearBookmarks();
	BDirectory dir(TFtpPositive::GetBookmarksDir());
	LoadBookmarks(&dir, fConnectMenu);
	if (!go) return;
	
	Clear();
	
	BPath path;
	entry.GetPath(&path);
	fBookmarkConfig = new TConfigFile(path.Path());
	if (fBookmarkConfig->Status() != B_OK) {
		BString msg("Bookmark load error.\n");
		msg << strerror(fBookmarkConfig->Status()) << "\n" << path.Path();
		(new BAlert("", msg.String(), "OK"))->Go();
		delete fBookmarkConfig;
		fBookmarkConfig = NULL;
		return;
	}
	
	this->SetTitle(path.Leaf());
	
	BString encoder, port, remotepath;
	fBookmarkConfig->Read("host", &fHost, "");
	fBookmarkConfig->Read("username", &fUsername, "");
	fBookmarkConfig->Read("password", &fPassword, "");
	fBookmarkConfig->Read("port", &port, "21");
	fBookmarkConfig->Read("encoder", &encoder, "");
	fBookmarkConfig->Read("remotepath", &remotepath, "");
	fBookmarkConfig->Read("localpath", &fLocalDir, TFtpPositive::GetDefaultLocalDir().String());
	
	fPort = atoi(port.String());
	
	fNoEncoder->SetMarked(true);
	if (encoder.Length() > 0) {
		BMenuItem *encItem = fEncodingMenu->FindItem(encoder.String());
		if (encItem) encItem->SetMarked(true);
	}
	
	Connect(remotepath.String());
}

status_t TFTPWindow::Connect(const char *remoteDir)
{
	status_t s;
	SetBusy(true);
	fStatusView->SetText("Connecting...");
	
	if (fFtpLooper != NULL) {
		fFtpLooper->Abort();
		fFtpLooper->Lock();
		fFtpLooper->Quit();
		fFtpLooper = NULL;
	}
	
	fFtpLooper = new TFtpLooper(this, fHost.String(), fPort, fUsername.String(), fPassword.String(),
		new TGenericDirentParser());
	fFtpLooper->Run();
	BMessage msg(FTP_CONNECT);
	msg.AddString("dir", remoteDir);
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return B_ERROR;
	}
	
	BString title(fHost.String());
	title << " - FtpPositive";
	this->SetTitle(title.String());
	
	return B_OK;
}

void TFTPWindow::PasvList()
{
	status_t s;
	if ((s = BMessenger(fFtpLooper).SendMessage(FTP_PASV_LIST)) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	fStatusView->SetText("Getting directory listing...");
	SetBusy(true);
}

void TFTPWindow::Chdir(const char *dir)
{
	if (ReconnectIfDisconnected() != B_OK) return;
	
	status_t s;
	BMessage msg(FTP_CHDIR);
	if ((s = msg.AddString("dir", dir)) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	SetBusy(true);
}

void TFTPWindow::DirlistChanged(BMessage *msg)
{
	fRemoteFileView->Clear();
	
	BString xpwd_result;
	if (msg->FindString("xpwd", &xpwd_result) != B_OK) {
	}
	
	int32 count;
	type_code type;
	
	msg->GetInfo("entry", &type, &count);
	for(int32 i = 0; i < count; i++) {
		BMessage entry;
		if (msg->FindMessage("entry", i, &entry) != B_OK) {
			*fLogView << "Internal Error in DirlistChanged(). msg->FindMessage() failed.\n";
			return;
		}
		BString path, date, permission, owner, group;
		int64 size;
		entry.FindString("path", &path);
		entry.FindString("date", &date);
		entry.FindString("permission", &permission);
		entry.FindString("owner", &owner);
		entry.FindString("group", &group);
		entry.FindInt64("size", &size);
		path.ReplaceFirst(xpwd_result.String(), "");
		AddRemoteFileItem(path.String(), size, date.String(), permission.String(), owner.String(), group.String());
	}
	
	BString status;
	status << count << " items";
	fItemCountView->SetText(status.String());
	
	BRow *top = fRemoteFileView->RowAt(0);
	fRemoteFileView->ScrollTo(top);
}

status_t TFTPWindow::ReconnectIfDisconnected()
{
	if (fFtpLooper == NULL) {
		*fLogView << "Reconnecting...\n";
		return Connect(fCurrentRemoteDir.String());
	}
	return B_OK;
}

void TFTPWindow::RemoteFileDoubleClicked()
{
	BRow *row = fRemoteFileView->CurrentSelection();
	if (row == NULL) return;
	
	BStringField *intNameField = (BStringField *)row->GetField(CLM_INTERNAL_NAME);
	BStringField *strpermField = (BStringField *)row->GetField(CLM_PERMISSION);
	
	if (strpermField->String()[0] == 'd') {
		// open directory
		this->Chdir(intNameField->String());
	} else {
		// open file
		
	}
}

void TFTPWindow::RemotePathChanged()
{
	BString remoteDir;
	encoder_addon_manager->ConvertToRemoteName(fRemoteDirView->Text(), &remoteDir);
	Chdir(remoteDir.String());
}

void TFTPWindow::BackwardClicked()
{
	BString *dir;
	dir = (BString *)fWalkHistory.ItemAt(fCurrentWalkHistoryIndex - 1);
	if (dir == NULL) return;
	fNavigating = -1;
	this->Chdir(dir->String());
}

void TFTPWindow::ForwardClicked()
{
	BString *dir;
	dir = (BString *)fWalkHistory.ItemAt(fCurrentWalkHistoryIndex + 1);
	if (dir == NULL) return;
	fNavigating = 1;
	this->Chdir(dir->String());
}

void TFTPWindow::GoParentClicked()
{
	this->Chdir("..");
}

void TFTPWindow::ReloadClicked()
{
	fNavigating = 2;
	this->Chdir(".");
}

void TFTPWindow::Rename()
{
	if (ReconnectIfDisconnected() != B_OK) return;
	
	status_t s;
	BString newName, remoteName;
	
	BRow *row = fRemoteFileView->CurrentSelection();
	if (row == NULL) return;
	
	BStringField *nameField = (BStringField *)row->GetField(CLM_NAME);
	BStringField *intNameField = (BStringField *)row->GetField(CLM_INTERNAL_NAME);
	
	// リネーム窓を表示
	BRect rect(Frame());
	if (!(new TRenameWindow(rect.left + 50, rect.top + 50, "Rename", "New Name:", nameField->String()))->Go(&newName)) {
		return;
	}
	if (strcmp(nameField->String(), newName.String()) == 0) return;
	
	// ファイル名変更コマンド送信
	BMenuItem *encMenuItem = fEncodingMenu->FindMarked();
	if (encMenuItem != NULL) encoder_addon_manager->SetEncoder(encMenuItem->Label());
	encoder_addon_manager->ConvertToRemoteName(newName.String(), &remoteName);
	
	BMessage msg(FTP_RENAME);
	if ((s = msg.AddString("from", intNameField->String())) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	if ((s = msg.AddString("to", remoteName.String())) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	
	SetBusy(true);
}

void TFTPWindow::Mkdir()
{
	status_t s;
	
	// 窓表示
	BRect rect(Frame());
	BString newName, remoteName;
	if (!(new TRenameWindow(rect.left + 50, rect.top + 50, "Create Directory", "New Name:", "NewDirectory"))->Go(&newName)) {
		return;
	}
	
	BMenuItem *encMenuItem = fEncodingMenu->FindMarked();
	if (encMenuItem != NULL) encoder_addon_manager->SetEncoder(encMenuItem->Label());
	encoder_addon_manager->ConvertToRemoteName(newName.String(), &remoteName);
	
	if (ReconnectIfDisconnected() != B_OK) return;
	
	BMessage msg(FTP_MKDIR);
	if ((s = msg.AddString("dir_name", remoteName.String())) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	
	SetBusy(true);
}

void TFTPWindow::Chmod()
{
	// 現在の permission を取得
	BRow *row = fRemoteFileView->CurrentSelection(NULL);
	if (row == NULL) return;
	BStringField *strpermField = (BStringField *)row->GetField(CLM_PERMISSION);
	uint32 mode = 0;
	if (strpermField->String()[1] != '-') mode |= 0400;
	if (strpermField->String()[2] != '-') mode |= 0200;
	if (strpermField->String()[3] != '-') mode |= 0100;
	if (strpermField->String()[4] != '-') mode |= 040;
	if (strpermField->String()[5] != '-') mode |= 020;
	if (strpermField->String()[6] != '-') mode |= 010;
	if (strpermField->String()[7] != '-') mode |= 04;
	if (strpermField->String()[8] != '-') mode |= 02;
	if (strpermField->String()[9] != '-') mode |= 01;
	
	// permission 変更窓を表示
	BRect rect(Frame());
	BString newMode;
	if (!(new TChmodWindow(rect.left + 50, rect.top + 50, "Change Permission"))->Go(mode, &newMode)) return;
	
	// permission 変更コマンドを送信
	if (ReconnectIfDisconnected() != B_OK) return;
	BMessage msg(FTP_SITE_2);
	status_t s;
	uint32 cnt = 0;
	row = NULL;
	while((row = fRemoteFileView->CurrentSelection(row))) {
		BStringField *intNameField = (BStringField *)row->GetField(CLM_INTERNAL_NAME);
		BString siteParam;
		siteParam << "chmod " << newMode << " " << intNameField->String();
		if ((s = msg.AddString("site_param", siteParam.String())) != B_OK) {
			*fLogView << "Error: " << strerror(s) << "\n";
			return;
		}
		cnt++;
	}
	if (cnt == 0) return;
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	SetBusy(true);
}

void TFTPWindow::Delete()
{
	BMessage entries;
	BRect rect;
	if (fRemoteFileView->GetSelectedEntries(&entries, &rect) == 0) return;
	
	if ((new BAlert("", "Really delete?", "Yes", "No", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1) return;
	status_t s;
	entries.what = FTP_DELETE;
	if ((s = BMessenger(fFtpLooper).SendMessage(&entries)) != B_OK) {
		*fLogView << "Error: " << strerror(s) << "\n";
		return;
	}
	SetBusy(true);
}

void TFTPWindow::MenuOpen()
{
	BPoint point;
	uint32 buttons;
	fRemoteFileView->GetMouse(&point, &buttons);
	fRemoteFileView->ConvertToScreen(&point);
	BMenuItem *selected = fPopUpMenu->Go(point, false, true, BRect(point.x, point.y, point.x, point.y));
	if (selected) BMessenger(this).SendMessage(selected->Command());
}

void TFTPWindow::DragReply(BMessage *msg)
{
	entry_ref local_dir_ref;
	BMessage previous;
	if (msg->FindRef("result", &local_dir_ref) != B_OK) return;
	BPath path(&local_dir_ref);
	if (strcmp(path.Path(), "/") == 0) return;
	if (msg->FindMessage("_previous_", &previous) != B_OK) return;
	
	Download(&previous, &local_dir_ref);
}

void TFTPWindow::DownloadClicked()
{
	BMessage entries;
	BRect rect;
	if (fRemoteFileView->GetSelectedEntries(&entries, &rect) == 0) return;
	
	entry_ref ref;
	BEntry entry(fLocalDir.String());
	if (entry.InitCheck() != B_OK) entry.SetTo(TFtpPositive::GetDefaultLocalDir().String());
	entry.GetRef(&ref);
	Download(&entries, &ref);
}

void TFTPWindow::Download(BMessage *entries, entry_ref *localDir)
{
	int32 count;
	type_code type;
	if (entries->GetInfo("remote_entry", &type, &count) != B_OK) return;
	BMessage dlmsg(FTP_DOWNLOAD);
	for(int32 i = 0; i < count; i++) {
		BMessage entry;
		if (entries->FindMessage("remote_entry", i, &entry) != B_OK) return;
		if (dlmsg.AddMessage("remote_entry", &entry) != B_OK) return;
	}
	if (dlmsg.AddRef("local_dir", localDir) != B_OK) return;
	if (dlmsg.AddString("remote_dir_name", fCurrentRemoteDir) != B_OK) return;
	
	BRect rect(Frame());
	dlmsg.AddRect("rect", rect);
	
	if (fUseThisConnection->Value()) {
		if (ReconnectIfDisconnected() != B_OK) return;
		status_t s;
		BMenuItem *encMenuItem = fEncodingMenu->FindMarked();
		if (encMenuItem != NULL) encoder_addon_manager->SetEncoder(encMenuItem->Label());
		dlmsg.AddBool("modal", true);
		if ((s = BMessenger(fFtpLooper).SendMessage(&dlmsg)) != B_OK) {
			*fLogView << "Error: " << strerror(s) << "\n";
			return;
		}
		return;
	}
}

void TFTPWindow::RemoteFileDropped(BMessage *msg)
{
	Upload(msg);
}

void TFTPWindow::UploadClicked()
{
}

void TFTPWindow::Upload(BMessage *msg)
{
	BRect rect(Frame());
	msg->AddRect("rect", rect);
	
	if (fUseThisConnection->Value()) {
		if (ReconnectIfDisconnected() != B_OK) return;
		msg->what = FTP_UPLOAD;
		msg->AddBool("modal", true);
		BMessenger(fFtpLooper).SendMessage(msg);
	}
}
