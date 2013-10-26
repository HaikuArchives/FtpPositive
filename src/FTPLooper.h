#ifndef _FTP_LOOPER_H_
#define _FTP_LOOPER_H_

#include <Looper.h>
#include <String.h>
#include <Message.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <CheckBox.h>
#include <TextView.h>
#include <Rect.h>
#include <Alert.h>
#include "FTPClient.h"
#include "DirentParser.h"
#include "ProgressWindow.h"

enum {
	FTP_REPORT = 'frep',
	FTP_POLLING_TIMER = 'fplt',
	FTP_CONNECT = 'fcon',
	FTP_PASV_LIST = 'fpls',
	FTP_CHDIR = 'mfch',
	FTP_RENAME = 'mfrn',
	FTP_MKDIR = 'mfmd',
	FTP_SITE = 'mfst',
	FTP_SITE_2 = 'mfs2',
	FTP_DELETE = 'mfde',
	FTP_SCANDIR = 'mfsd',
	FTP_LOG = 'mlog',
	
	FTP_DATA_RECEIVING = 'mdrc',
	
	FTP_DOWNLOAD = 'mfdl',
	FTP_UPLOAD = 'mfup',
	
	FTP_ALERT_DONTASK = 'fada',
};

enum {
	REQ_NO_REQUEST = 'rnrq',
	REQ_KILL_ME = 'rkme',
	REQ_WALKED = 'rwlk',
};

enum {
	DL_NLST_R_MODE = 0,
	DL_RECURSE_MODE,
};

typedef struct entry_item {
	BString *ei_path;
	BString *ei_date;
	BString *ei_permission;
	BString *ei_owner;
	BString *ei_group;
	int64 ei_size;
} entry_item;


class TDontAskAgainAlert : public BAlert
{
public:
	TDontAskAgainAlert(const char *title, const char *text,
			const char *button0label, const char *button1label, const char *button2label,
			int32 *dontAskAgain)
		:	BAlert(title, text, button0label, button1label, button2label)
	{
		BTextView *textView = this->TextView();
		BRect rect(textView->Bounds());
		rect.top = rect.bottom - 20;
		rect.bottom = rect.top + 15;
		BCheckBox *dontAsk = new BCheckBox(rect, "", "Don't ask again", new BMessage(FTP_ALERT_DONTASK));
		textView->AddChild(dontAsk);
		dontAsk->SetTarget(this);
		fDontAskAgain = dontAskAgain;
	};
	
	void MessageReceived(BMessage *msg)
	{
		if (msg->what == FTP_ALERT_DONTASK) {
			msg->FindInt32("be:value", fDontAskAgain);
		} else {
			BAlert::MessageReceived(msg);
		}
	}
	
private:
	int32 *fDontAskAgain;
};


class TEntryList : public BList
{
public:
	TEntryList() : BList() {};
	~TEntryList() {
		Clear();
	};
	void Clear() {
		for(int32 i = 0; i < CountItems(); i++) {
			delete EntryAt(i)->ei_path;
			delete EntryAt(i)->ei_date;
			delete EntryAt(i)->ei_permission;
			delete EntryAt(i)->ei_owner;
			delete EntryAt(i)->ei_group;
			free(EntryAt(i));
		}
		MakeEmpty();
	};
	bool AddEntry(entry_item *entry) {
		entry_item *item = (entry_item *)malloc(sizeof(entry_item));
		if (item == NULL) return false;
		item->ei_path = new BString(entry->ei_path->String());
		item->ei_date = new BString(entry->ei_date->String());
		item->ei_permission = new BString(entry->ei_permission->String());
		item->ei_owner = new BString(entry->ei_owner->String());
		item->ei_group = new BString(entry->ei_group->String());
		item->ei_size = entry->ei_size;
		bool r = AddItem(item);
		if (!r) free(item);
		return r;
	};
	bool AddEntry(const char *path, const char *date, const char *perm, const char *owner, const char *group, int32 size) {
		entry_item *item = (entry_item *)malloc(sizeof(entry_item));
		if (item == NULL) return false;
		item->ei_path = new BString(path);
		item->ei_date = new BString(date);
		item->ei_permission = new BString(perm);
		item->ei_owner = new BString(owner);
		item->ei_group = new BString(group);
		item->ei_size = size;
		bool r = AddItem(item);
		if (!r) free(item);
		return r;
	};
	entry_item *EntryAt(int32 index) const {
		return (entry_item *)ItemAt(index);
	};
};


class TLogger
{
public:
	TLogger() {
		fMessenger = NULL;
	};
	~TLogger() {
		if (fMessenger) delete fMessenger;
	};
	
	void SetTarget(BHandler *handler) {
		fMessenger = new BMessenger(handler);
	};
	
	TLogger& operator<<(const char *string) {
		if (string) {
			fPendingMsg.Append(string);
		} else {
			BMessage msg(FTP_LOG);
			msg.AddString("log", fPendingMsg.String());
			fMessenger->SendMessage(&msg);
			fPendingMsg.SetTo("");
		}
		return *this;
	};
	
	TLogger& operator<<(BString string) {
		return *this << string.String();
	};
	
	TLogger& operator<<(int val) {
		BString s;
		s << val;
		return *this << s.String();
	};
	
private:
	BString fPendingMsg;
	BMessenger *fMessenger;
};

class TFtpLooper : public BLooper
{
public:
	TFtpLooper(BHandler *target, const char *host, uint16 port,
		const char *username, const char *password, TDirentParser *direntParser);
	~TFtpLooper();
	void MessageReceived(BMessage *msg);
	void SetTarget(BHandler *target);
	void Abort();
private:
	status_t SendCmd(const char *command);
	status_t Readres(int32 *reply, BString *lastLine = NULL);
	status_t USER(const char *userName, int32 *reply);
	status_t PASS(const char *password, int32 *reply);
	status_t PASV(struct sockaddr_in *addr, int32 *reply);
	status_t NLST(struct sockaddr_in *addr, bool passive, const char *dir, const char *option);
	status_t XPWD(BString *result, int32 *reply);
	status_t CWD(const char *dir, int32 *reply);
	status_t TYPE(const char *type, int32 *reply);
	status_t SIZE(const char *remoteFilePath, int64 *size, int32 *reply);
	status_t RETR(const char *remoteFilePath);
	status_t STOR(const char *remoteFilePath);
	status_t RNFR(const char *remoteFilePath, int32 *reply);
	status_t RNTO(const char *remoteFilePath, int32 *reply);
	status_t MKD(const char *dirName, int32 *reply);
	status_t SITE(const char *siteParam, int32 *reply);
	status_t REST(off_t startPos, int32 *reply);
	status_t DELE(const char *remotePath, int32 *reply);
	status_t RMD(const char *remotePath, int32 *reply);
	status_t NOOP(int32 *reply);
	
	void PollingTimer();
	void SendStatusMessage(const char *msgstring);
	status_t SendResultMessage(status_t e);
	status_t SendResultMessage(BMessage *result, int32 request, const char *statusMsg,
		const char *logMsg, bool busy);
	void Connect(BMessage *msg);
	void GetDirList();
	void NlstRDirList(BMessage *msg, TEntryList *pathList);
	status_t _recurseDirList(const char *dir, TEntryList *pathList);
	void RecurseDirList(BMessage *msg, TEntryList *pathList);
	status_t PasvList(const char *dirPath, TEntryList *pathList, const char *option);
	status_t PasvListR(const char *dirPath, TEntryList *pathList);
	void Chdir(BMessage *msg);
	void Download(BMessage *msg);
	void Upload(BMessage *msg);
	void Rename(BMessage *msg);
	void Mkdir(BMessage *msg);
	void Site(BMessage *msg);
	void Site2(BMessage *msg);
	void Delete(BMessage *msg);
	status_t DeleteEntry(const char *remotePath, bool isDirectory);
	
	int32 _download_alert(const char *name, const char *err);
	status_t PasvDownload(const char *localFilePath, const char *remoteFilePath, int64 size,
		TProgressWindow *progressWindow, int32 *defaultAnswer, int32 *dontAsk);
	
	int32 _upload_alert(const char *name, const char *err);
	int32 UploadLinkAlert(const char *name, int32 *traverseWantTo);
	status_t UploadDir(const char *dirName);
	status_t UploadEntry(BEntry *entry, const char *baseDir,
		int32 *dontAskTraverse, int32 *traverseWantTo, TProgressWindow *progressWindow);
	status_t UploadEntries(BDirectory *dir, const char *baseDir,
		int32 *dontAskTraverse, int32 *traverseWantTo, TProgressWindow *progressWindow);
	status_t PasvUpload(BEntry *localEntry, const char *remoteFilePath,
		TProgressWindow *progressWindow);
	
	int32 fDownloadListingMode;
	
	TFTPClient *fFtpClient;
	BMessenger *fMessenger;
	BMessageRunner *fPollingTimer;
	
	BString fHost;
	uint16 fPort;
	BString fUsername;
	BString fPassword;
	TDirentParser *fDirentParser;
	
	volatile bool fAbort;
	
	TLogger _LOG;
};

#endif
