#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <Entry.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <MessageFormat.h>
#include <Messenger.h>
#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "BookmarkWindow.h"
#include "ChmodWindow.h"
#include "FtpPositive.h"
#include "FTPWindow.h"
#include "MimeDB.h"
#include "RenameWindow.h"
#include "SizeColumn.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FTPWindow"

static const char* kZeroItems = B_TRANSLATE("no items");
static const char* kNewBookmark = B_TRANSLATE("New bookmark");
static const char* kCreateDirectory = B_TRANSLATE("Create directory");
static const char* kNewName = B_TRANSLATE("New name:");
static const char* kNewDirectory = B_TRANSLATE("New directory:");
static const char* kError = B_TRANSLATE("Error");


// ----------------------------------- TLogView -------------------------------------

TLogView::TLogView(const char *name)
	:	BTextView(name, B_WILL_DRAW)
{
	this->MakeEditable(false);
	this->SetWordWrap(true);
	this->SetStylable(true);
}

TLogView::~TLogView()
{
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


static void
FileMenu(BMenu* menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Download"), new BMessage(MSG_DOWNLOAD_CLICKED), 'D'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Upload"), new BMessage(MSG_UPLOAD_CLICKED), 'U'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Delete"), new BMessage(MSG_DELETE), 'T'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Rename/Move"), new BMessage(MSG_RENAME), 'E'));
	menu->AddItem(new BMenuItem(kCreateDirectory, new BMessage(MSG_MKDIR), 'M'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Copy FTP URL"), new BMessage(MSG_COPYURL), 'C'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Change permissions"), new BMessage(MSG_CHMOD), 'J'));
}

static void
NaviMenu(BMenu* menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Back"), new BMessage(MSG_BACKWARD_CLICKED), B_LEFT_ARROW));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Forward"), new BMessage(MSG_FORWARD_CLICKED), B_RIGHT_ARROW));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Parent dir"), new BMessage(MSG_GOPARENT_CLICKED), B_UP_ARROW));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Refresh"), new BMessage(MSG_RELOAD_CLICKED), 'R'));
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

	fOpenPanel.SetTarget(BMessenger(this));
	fOpenPanel.SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Upload"));
	fOpenPanel.Window()->SetTitle(B_TRANSLATE("FtpPositive: Upload"));

	BRect rect;
	
	// Main Menu
	BMenuBar *mainMenu = new BMenuBar("MainMenuBar");
	
	// Tool Bar
	mainToolBar = new BToolBar(B_HORIZONTAL);
	
	// Forward, Backward, Upfolder
	rect.left = 0;
	rect.right = 0;
	rect.top = 1;
	rect.bottom = 1;
	
	mainToolBar->AddAction(new BMessage(MSG_BACKWARD_CLICKED),this,TSimplePictureButton::ResVectorToBitmap("NAVIGATION_BACKWARD"),"Backward","",false);
	mainToolBar->AddAction(new BMessage(MSG_FORWARD_CLICKED),this,TSimplePictureButton::ResVectorToBitmap("NAVIGATION_FORWARD"),"Forward","",false);
	mainToolBar->AddAction(new BMessage(MSG_GOPARENT_CLICKED),this,TSimplePictureButton::ResVectorToBitmap("NAVIGATION_GOPARENT"), "Go to Parent","",false);
	mainToolBar->AddAction(new BMessage(MSG_RELOAD_CLICKED),this,TSimplePictureButton::ResVectorToBitmap("NAVIGATION_RELOAD"),"Reload","",false);
	mainToolBar->AddAction(new BMessage(MSG_CANCEL),this,TSimplePictureButton::ResVectorToBitmap("NAVIGATION_STOP"),"Cancel","",false);
	// Remote Path View
	const char* label = B_TRANSLATE("Remote:");
	fRemoteDirView = new BTextControl("RemoteDirView", label, "",
		new BMessage(MSG_REMOTE_PATH_CHANGED));
	fRemoteDirView->SetDivider(fRemoteDirView->StringWidth(label));
	mainToolBar->GetLayout()->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(B_USE_BIG_SPACING));
	mainToolBar->AddView(fRemoteDirView);
	mainToolBar->GetLayout()->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(5));

	// Remote File View
	BFont font;
	TStringColumn *nameColumn = new TStringColumn(B_TRANSLATE("Name"), 150, 60, INT_MAX, 0);
	BStringColumn *intNameColumn = new BStringColumn(B_TRANSLATE("Internal name"), 150, 60, INT_MAX, 0);
	fRemoteFileView = new TRemoteFileView("RemoteFileView");
	fRemoteFileView->AddColumn(nameColumn, CLM_NAME);
	fRemoteFileView->AddColumn(intNameColumn, CLM_INTERNAL_NAME);
	fRemoteFileView->AddColumn(new SizeColumn(B_TRANSLATE("Size"), 80, 60, INT_MAX, B_ALIGN_RIGHT), CLM_SIZE);
	fRemoteFileView->AddColumn(new BStringColumn(B_TRANSLATE("Date"), 100, 60, INT_MAX, 0), CLM_DATE);
	fRemoteFileView->AddColumn(new BStringColumn(B_TRANSLATE("Permission"), 90, 60, INT_MAX, 0), CLM_PERMISSION);
	fRemoteFileView->AddColumn(new BStringColumn(B_TRANSLATE("Owner"), 70, 60, INT_MAX, 0), CLM_OWNER);
	fRemoteFileView->AddColumn(new BStringColumn(B_TRANSLATE("Group"), 70, 60, INT_MAX, 0), CLM_GROUP);
	fRemoteFileView->SetTarget(this);
	fRemoteFileView->SetInvocationMessage(new BMessage(MSG_ENTRY_DOUBLE_CLICKED));
	intNameColumn->SetVisible(false);
	fRemoteFileView->SetSortingEnabled(true);
	fRemoteFileView->SetSortColumn(nameColumn, false, true);
	fRemoteFileView->SetLatchWidth(font.Size()*2);

	BScrollBar* scrollBar = (BScrollBar*)fRemoteFileView->FindView("horizontal_scroll_bar");
	fItemCountView = new StatusView(scrollBar);
	fItemCountView->SetText(kZeroItems);
	fRemoteFileView->AddStatusView(fItemCountView);

	// CheckBox (Use This Connection)
	const char* str = B_TRANSLATE("Use this connection");
	fUseThisConnection = new BCheckBox("UseThisConnection", str, NULL, 0);
	fUseThisConnection->SetValue(1);
	
	// Log View
	fLogView = new TLogView("LogView");
	BScrollView *logScrollView = new BScrollView("logScrollView", fLogView,
		B_WILL_DRAW, false, true, B_PLAIN_BORDER);
	
	// Status View
	fStatusView = new BStringView("StatusView", "");
	
	BLayoutBuilder::Group<>(this,B_VERTICAL,0)
		.Add(mainMenu)
		.Add(mainToolBar)
		.AddGroup(B_VERTICAL,0)
			.SetInsets(B_USE_ITEM_SPACING, 0, B_USE_ITEM_SPACING, 0)
			.AddSplit(B_VERTICAL, 0)
				.Add(fRemoteFileView)
				.Add(logScrollView)
			.End()
			.AddGroup(B_HORIZONTAL, 0)
			.SetInsets(0, 0, B_USE_ITEM_SPACING, 0)
				.Add(fStatusView,1)
				.AddGlue(2)
				.Add(fUseThisConnection,1)
			.End()
		.End();
	;
	// File Menu
	fFileMenu = new BMenu(B_TRANSLATE("File"));
	mainMenu->AddItem(fFileMenu);
	fFileMenu->AddItem(new BMenuItem(B_TRANSLATE("About FtpPositive"), new BMessage(B_ABOUT_REQUESTED)));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED), 'W'));
	
	// Connect Menu
	fConnectMenu = new BMenu(B_TRANSLATE("Connect"));
	mainMenu->AddItem(fConnectMenu);
	fConnectMenu->AddItem(new BMenuItem(kNewBookmark, new BMessage(MSG_NEW_BOOKMARK), 'N'));
	fConnectMenu->AddItem(new BMenuItem(B_TRANSLATE("Show bookmarks"), new BMessage(MSG_SHOW_BOOKMARK), 'B'));
	fConnectMenu->AddSeparatorItem();
	
	BDirectory dir(TFtpPositive::GetBookmarksDir());
	LoadBookmarks(&dir, fConnectMenu);
	
	// Command Menu
	fCommandMenu = new BMenu(B_TRANSLATE("Command"));
	mainMenu->AddItem(fCommandMenu);
	FileMenu(fCommandMenu);
	fCommandMenu->AddSeparatorItem();
	NaviMenu(fCommandMenu);
	
	// Encoding Menu
	fEncodingMenu = new BMenu(B_TRANSLATE("Encoding"));
	mainMenu->AddItem(fEncodingMenu);
	fEncodingMenu->SetRadioMode(true);
	fNoEncoder = new BMenuItem(B_TRANSLATE("None"), new BMessage(MSG_ENCODER_SELECTED));
	fEncodingMenu->AddItem(fNoEncoder);
	fEncodingMenu->AddSeparatorItem();
	fNoEncoder->SetMarked(true);
	for (int32 i = 0; i < encoder_addon_manager->CountAddons(); i++) {
		fEncodingMenu->AddItem(new BMenuItem(encoder_addon_manager->NameAt(i), new BMessage(MSG_ENCODER_SELECTED)));
	}
	
	// PopupMenu
	fPopUpMenu = new BPopUpMenu("PopUpMenu");
	fPopUpMenu->SetRadioMode(false);
	NaviMenu(fPopUpMenu);
	fPopUpMenu->AddSeparatorItem();
	FileMenu(fPopUpMenu);
	// 


	fRemoteFileView->MakeFocus(true);
	fLogView->SetTextRect(BRect(5,5,-1,-1));
	fStatusView->SetText(B_TRANSLATE("Idle."));
	fCommandMenu->SetEnabled(false);
	fPopUpMenu->SetEnabled(false);
	fUseThisConnection->SetEnabled(false);
	fRemoteDirView->SetEnabled(false);
	mainToolBar->FindButton(MSG_BACKWARD_CLICKED)->SetEnabled(false);
	mainToolBar->FindButton(MSG_FORWARD_CLICKED)->SetEnabled(false);
	mainToolBar->FindButton(MSG_GOPARENT_CLICKED)->SetEnabled(false);
	mainToolBar->FindButton(MSG_RELOAD_CLICKED)->SetEnabled(false);
	mainToolBar->FindButton(MSG_CANCEL)->Hide();
	PostMessage(new BMessage(MSG_READY));
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
	fItemCountView->SetText(kZeroItems);
	this->SetTitle(kAppName);
	
	fStatusView->SetText(B_TRANSLATE("Suspended."));
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
		case MSG_COPYURL:              CopyUrl(); break;
		case MSG_CHMOD:                Chmod(); break;
		case MSG_DELETE:               Delete(); break;
		case B_SIMPLE_DATA:            RemoteFileDropped(msg); break;
		case B_REPLY:                  DragReply(msg); break;
		case MSG_MENU_OPEN:            MenuOpen(); break;
		case MSG_DOWNLOAD_CLICKED:     DownloadClicked(); break;
		case MSG_UPLOAD_CLICKED:       UploadClicked(); break;
		case B_REFS_RECEIVED:          Upload(msg); break;
		case MSG_READY:	{
							*fLogView << B_TRANSLATE("Welcome to FtpPositive")
							<< " " << VERSION
							<< "\n" << COPY;
							break;
		}
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

void TFTPWindow::AddRemoteFileItem(const char *name, int64 size,
	const char *date, const char *perm, const char *owner, const char *group)
{
	// ディレクトリ、またはファイルタイプのアイコンイメージを取得
	bool isDirectory = false;
	static const float icon_size = BRow().Height() - 2;

	BBitmap *icon = new BBitmap(BRect(0, 0, icon_size - 1, icon_size - 1), B_RGBA32);
	if (strlen(perm) > 0) {
		if (perm[0] == 'd') {
			app_mimedb->GetMimeIcon("application/x-vnd.Be-directory", icon, B_MINI_ICON);
			isDirectory = true;
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
	row->SetField(new BSizeField(isDirectory ? -1 : size), CLM_SIZE);
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
	mainToolBar->FindButton(MSG_CANCEL)->SetEnabled(busy);
	fUseThisConnection->SetEnabled(!busy);
	mainToolBar->FindButton(MSG_BACKWARD_CLICKED)->SetEnabled(!busy);
	mainToolBar->FindButton(MSG_FORWARD_CLICKED)->SetEnabled(!busy);
	mainToolBar->FindButton(MSG_GOPARENT_CLICKED)->SetEnabled(!busy);
	mainToolBar->FindButton(MSG_RELOAD_CLICKED)->SetEnabled(!busy);
	if (busy) {
		mainToolBar->FindButton(MSG_CANCEL)->Show();
		mainToolBar->FindButton(MSG_RELOAD_CLICKED)->Hide();
	} else {
		mainToolBar->FindButton(MSG_CANCEL)->Hide();
		mainToolBar->FindButton(MSG_RELOAD_CLICKED)->Show();
	}
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
	bool go = (new TBookmarkWindow(rect.left + 50, rect.top + 50, kNewBookmark, TFtpPositive::GetBookmarksDir().String()))->Go(pathName, &entry);
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
		(new BAlert("", msg.String(), B_TRANSLATE("OK")))->Go();
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
	fStatusView->SetText(B_TRANSLATE("Connecting..."));
	
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
		*fLogView << kError << ": " << strerror(s) << "\n";
		return B_ERROR;
	}
	
	BString title(fHost.String());
	title << " - " << kAppName;
	this->SetTitle(title.String());
	
	return B_OK;
}

void TFTPWindow::PasvList()
{
	status_t s;
	if ((s = BMessenger(fFtpLooper).SendMessage(FTP_PASV_LIST)) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
		return;
	}
	fStatusView->SetText(B_TRANSLATE("Getting directory listing..."));
	SetBusy(true);
}

void TFTPWindow::Chdir(const char *dir)
{
	if (ReconnectIfDisconnected() != B_OK) return;
	
	status_t s;
	BMessage msg(FTP_CHDIR);
	if ((s = msg.AddString("dir", dir)) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
		return;
	}
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
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

	BString itemsNumber;
	static BMessageFormat formatItems(B_TRANSLATE("{0, plural,"
		"=0{no items}"
		"=1{1 item}"
		"other{# items}}" ));
	formatItems.Format(itemsNumber, count);
	fItemCountView->SetText(itemsNumber.String());

	BRow *top = fRemoteFileView->RowAt(0);
	fRemoteFileView->ScrollTo(top);
}

status_t TFTPWindow::ReconnectIfDisconnected()
{
	if (fFtpLooper == NULL) {
		*fLogView << B_TRANSLATE("Reconnecting") << "...\n";
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
	if (!(new TRenameWindow(rect.left + 50, rect.top + 50,
		B_TRANSLATE("Rename"), kNewName, nameField->String()))->Go(&newName)) {
		return;
	}
	if (strcmp(nameField->String(), newName.String()) == 0) return;
	
	// ファイル名変更コマンド送信
	BMenuItem *encMenuItem = fEncodingMenu->FindMarked();
	if (encMenuItem != NULL) encoder_addon_manager->SetEncoder(encMenuItem->Label());
	encoder_addon_manager->ConvertToRemoteName(newName.String(), &remoteName);
	
	BMessage msg(FTP_RENAME);
	if ((s = msg.AddString("from", intNameField->String())) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
		return;
	}
	if ((s = msg.AddString("to", remoteName.String())) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
		return;
	}
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
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
	if (!(new TRenameWindow(rect.left + 50, rect.top + 50, kCreateDirectory,
		kNewDirectory, "NewDirectory"))->Go(&newName)) {
		return;
	}
	
	BMenuItem *encMenuItem = fEncodingMenu->FindMarked();
	if (encMenuItem != NULL) encoder_addon_manager->SetEncoder(encMenuItem->Label());
	encoder_addon_manager->ConvertToRemoteName(newName.String(), &remoteName);
	
	if (ReconnectIfDisconnected() != B_OK) return;
	
	BMessage msg(FTP_MKDIR);
	if ((s = msg.AddString("dir_name", remoteName.String())) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
		return;
	}
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
		return;
	}
	
	SetBusy(true);
}

void TFTPWindow::CopyUrl()
{
	BRow *row = fRemoteFileView->CurrentSelection();
	if (row == NULL) return;

	const char *intName
		= ((BStringField *)row->GetField(CLM_INTERNAL_NAME))->String();

	BString url;
	if (fCurrentRemoteDir.Length() > 0 &&
		fCurrentRemoteDir[fCurrentRemoteDir.Length() - 1] == '/') {

		url << "ftp://" << fHost << ":" << fPort
			<< fCurrentRemoteDir << intName;
	} else {
		url << "ftp://" << fHost << ":" << fPort
			<< fCurrentRemoteDir << "/" << intName;
	}

	BMessage *clip = (BMessage *)NULL;
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		if (clip = be_clipboard->Data()) {
			clip->AddData("text/plain", B_MIME_TYPE,
				url.String(), url.Length());
			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}
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
	if (!(new TChmodWindow(rect.left + 50, rect.top + 50, B_TRANSLATE("Change permission")))->Go(mode, &newMode))
		return;
	
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
			*fLogView << kError << ": " << strerror(s) << "\n";
			return;
		}
		cnt++;
	}
	if (cnt == 0) return;
	if ((s = BMessenger(fFtpLooper).SendMessage(&msg)) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
		return;
	}
	SetBusy(true);
}

void TFTPWindow::Delete()
{
	BMessage entries;
	BRect rect;
	if (fRemoteFileView->GetSelectedEntries(&entries, &rect) == 0) return;
	
	if ((new BAlert("", B_TRANSLATE("Really delete?"), B_TRANSLATE("Yes"),
		B_TRANSLATE("No"), NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1)
		return;
	status_t s;
	entries.what = FTP_DELETE;
	if ((s = BMessenger(fFtpLooper).SendMessage(&entries)) != B_OK) {
		*fLogView << kError << ": " << strerror(s) << "\n";
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
			*fLogView << kError << ": " << strerror(s) << "\n";
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
	fOpenPanel.Show();
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


// HACK TO minimize glitches in ColumnListView
// The menubar should still have problems
// remove when #3037 in haiku is fixed.
void
TFTPWindow::FrameResized(float newWidth, float newHeight)
{
		fRemoteFileView->Hide();
		BWindow::FrameResized(newWidth, newHeight);
		fRemoteFileView->Show();
}
