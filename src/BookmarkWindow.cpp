#include <View.h>
#include <String.h>
#include <Alert.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Messenger.h>
#include <File.h>
#include <Path.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ConfigFile.h"
#include "BookmarkWindow.h"
#include "FTPWindow.h"
#include "FtpPositive.h"

enum {
	CONNECT_CLICKED = 'svcn',
	SAVE_CLICKED = 'save',
	EDIT_CHANGED = 'edit',
};

static const char* kCancel = B_TRANSLATE("Cancel");
static const char* kOK = B_TRANSLATE("OK");

TBookmarkWindow::TBookmarkWindow(float x, float y, const char *title, const char *dir)
	:	BWindow(BRect(x, y, 0,0), title, B_FLOATING_WINDOW_LOOK,
				B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS )
{
	fBookmarkDir.SetTo(dir);
	
	fNewBookmark = true;
	
	float lx = 10, rx = Bounds().right - 10, div = 70;
	
	fBookmarkName = new TOAEdit("BookmarkName", B_TRANSLATE("Name:"), B_TRANSLATE("New bookmark"), new BMessage(EDIT_CHANGED));
	fHostAddress = new TOAEdit("HostAddress", B_TRANSLATE("Address:"), "", new BMessage(EDIT_CHANGED));
	fPort = new TOAEdit("Port", B_TRANSLATE("Port:"), "21", new BMessage(EDIT_CHANGED));
	fUsername = new TOAEdit("Username", B_TRANSLATE("User name:"), "", new BMessage(EDIT_CHANGED));
	fPassword = new TOAEdit("Password", B_TRANSLATE("Password:"), "", new BMessage(EDIT_CHANGED));
	fEncoder = new TOAEdit("Encoder", B_TRANSLATE("Encoder:"), "", NULL);
	fRemotePath = new TOAEdit("RemotePath", B_TRANSLATE("Remote path:"), "", NULL);
	fLocalPath = new TOAEdit("LocalPath", B_TRANSLATE("Local path:"), TFtpPositive::GetDefaultLocalDir().String(), NULL);
	
	fBookmarkName->SetDivider(div);
	fHostAddress->SetDivider(div);
	fPort->SetDivider(div);
	fUsername->SetDivider(div);
	fPassword->SetDivider(div);
	fEncoder->SetDivider(div);
	fRemotePath->SetDivider(div);
	fLocalPath->SetDivider(div);
	
	
	
	fSaveAndConnectButton = new BButton("Connect", B_TRANSLATE("Connect"), new BMessage(CONNECT_CLICKED));
	fSaveAndConnectButton->SetEnabled(false);
	
	fSaveButton = new BButton("Save", B_TRANSLATE("Save"), new BMessage(SAVE_CLICKED));
	fSaveButton->SetEnabled(false);
	
	BButton *cancelButton = new BButton("Cancel", kCancel, new BMessage(B_QUIT_REQUESTED));
	
	BLayoutBuilder::Grid<>(this,2,2)
		.SetInsets(10)
		.Add(fBookmarkName->CreateLabelLayoutItem(),0,0)
		.Add(fBookmarkName->CreateTextViewLayoutItem(),1,0)
		.Add(fHostAddress->CreateLabelLayoutItem(),0,1)
		.Add(fHostAddress->CreateTextViewLayoutItem(),1,1)
		.Add(fPort->CreateLabelLayoutItem(),0,2)
		.Add(fPort->CreateTextViewLayoutItem(),1,2)
		.Add(fUsername->CreateLabelLayoutItem(),0,3)
		.Add(fUsername->CreateTextViewLayoutItem(),1,3)
		.Add(fPassword->CreateLabelLayoutItem(),0,4)
		.Add(fPassword->CreateTextViewLayoutItem(),1,4)
		.Add(fEncoder->CreateLabelLayoutItem(),0,5)
		.Add(fEncoder->CreateTextViewLayoutItem(),1,5)
		.Add(fRemotePath->CreateLabelLayoutItem(),0,6)
		.Add(fRemotePath->CreateTextViewLayoutItem(),1,6)
		.Add(fLocalPath->CreateLabelLayoutItem(),0,7)
		.Add(fLocalPath->CreateTextViewLayoutItem(),1,7)
		.AddGroup(B_HORIZONTAL,10,0,8,2)
			.AddStrut(B_USE_BIG_SPACING)
			.Add(fSaveAndConnectButton,2)
			.Add(fSaveButton,1)
			.Add(cancelButton,1)
		.End()
	.View()->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	
	fBookmarkName->SetTarget(this);
	fHostAddress->SetTarget(this);
	fPort->SetTarget(this);
	fUsername->SetTarget(this);
	fPassword->SetTarget(this);
	fPassword->TextView()->HideTyping(true);
	
	AddShortcut('W', 0, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('S', 0, new BMessage(SAVE_CLICKED));
	
	fSaveAndConnectButton->MakeFocus(true);
	fStatus = B_BUSY;
}

TBookmarkWindow::~TBookmarkWindow()
{
}

bool TBookmarkWindow::QuitRequested()
{
	if (fStatus == B_BUSY) {
		fStatus = B_ERROR;
		return false;
	}
	return true;
}

void TBookmarkWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case SAVE_CLICKED:
			Save();
			BMessenger(this).SendMessage(B_QUIT_REQUESTED);
			break;
		case CONNECT_CLICKED:
			Save();
			fStatus = B_OK;
			break;
		case EDIT_CHANGED:
			EditChanged();
			break;
		default: BWindow::MessageReceived(msg);
	}
}

bool TBookmarkWindow::Go(const char *pathName, BEntry *entry)
{
	if (pathName != NULL) Load(pathName);
	BWindow *window = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
	this->Show();
	while(this->fStatus == B_BUSY) {
		snooze(1000);
		window->UpdateIfNeeded();
	}
	*entry = fEntry;
	bool result = this->fStatus == B_OK;
	BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	return result;
}

status_t TBookmarkWindow::Save()
{
	BString data;
	data << "host " << fHostAddress->Text() << "\n"
		<< "username " << fUsername->Text() << "\n"
		<< "password " << fPassword->Text() << "\n"
		<< "port " << fPort->Text() << "\n"
		<< "encoder " << fEncoder->Text() << "\n"
		<< "remotepath " << fRemotePath->Text() << "\n"
		<< "localpath " << fLocalPath->Text() << "\n"
		;
	
	if (fEntry.InitCheck() != B_OK) {
		BString path(TFtpPositive::GetBookmarksDir().String());
		path << "/" << fBookmarkName->Text();
		fEntry.SetTo(path.String());
	} else {
		status_t e = fEntry.Rename(fBookmarkName->Text());
		if (e != B_OK) {
			BString emsg;
			emsg << strerror(e);
			if (e == B_FILE_EXISTS) {
				if (fNewBookmark) {
					if ((new BAlert("", emsg.String(), B_TRANSLATE("Overwrite"), kCancel))->Go() == 1)
						return B_ERROR;
				}
			} else {
				(new BAlert("", emsg.String(), kCancel))->Go();
				return B_ERROR;
			}
		}
	}
	
	BPath path;
	fEntry.GetPath(&path);
	BFile file(path.Path(), B_CREATE_FILE | B_WRITE_ONLY);
	const char* str = B_TRANSLATE("Bookmark save failed");
	if (file.InitCheck() != B_OK) {
		BString emsg;
		emsg << str << ".\n" << strerror(file.InitCheck());
		(new BAlert("", emsg.String(), kCancel))->Go();
		return B_ERROR;
	}
	file.SetSize(0);
	ssize_t s = file.Write(data.String(), data.Length());
	if (s < 0) {
		BString msg(str);
		msg << ".\n" << strerror(s);
		(new BAlert("", msg.String(), kOK))->Go();
		return B_ERROR;
	}
	return B_OK;
}

void TBookmarkWindow::EditChanged()
{	
	BString str;
	uint32 p = atoi(fPort->Text());
	if (p == 0) str << "21"; else str << p;
	fPort->SetText(str.String());
	SetTitle(fBookmarkName->Text());
	
	fSaveButton->SetEnabled(
		(strlen(fBookmarkName->Text()) != 0) &&
		(strlen(fHostAddress->Text()) != 0) &&
		(strlen(fUsername->Text()) != 0));
	fSaveAndConnectButton->SetEnabled(fSaveButton->IsEnabled());
}

void TBookmarkWindow::Load(const char *pathName)
{
	TConfigFile config(pathName);
	if (config.Status() != B_OK) {
		BString msg(B_TRANSLATE("Bookmark load error"));
		msg << ".\n" << strerror(config.Status()) << "\n" << pathName;
		(new BAlert("", msg.String(), kOK))->Go();
		return;
	}
	fEntry.SetTo(pathName);
	BString host, username, password, port, encoder, remotepath, localpath;
	config.Read("host", &host, "");
	config.Read("username", &username, "");
	config.Read("password", &password, "");
	config.Read("port", &port, "21");
	config.Read("encoder", &encoder, "");
	config.Read("remotepath", &remotepath, "");
	config.Read("localpath", &localpath, TFtpPositive::GetDefaultLocalDir().String());
	
	BPath path(pathName), dir;
	path.GetParent(&dir);
	fBookmarkDir.SetTo(dir.Path());
	
	fBookmarkName->SetText(path.Leaf());
	fHostAddress->SetText(host.String());
	fPort->SetText(port.String());
	fUsername->SetText(username.String());
	fPassword->SetText(password.String());
	fEncoder->SetText(encoder.String());
	fRemotePath->SetText(remotepath.String());
	fLocalPath->SetText(localpath.String());
	
	EditChanged();
	fNewBookmark = false;
}
