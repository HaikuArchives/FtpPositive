#include <stdio.h>
#include <netinet/in.h>

#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Messenger.h>
#include <Path.h>
#include <StringForSize.h>

#include "EncoderAddonManager.h"
#include "FTPLooper.h"
#include "TCPStream.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FTPLooper"

#define RECEIVE_BUFF_SIZE 65536
#define POLLING_INTERVAL 5000000
#define _FLUSH (const char *)NULL


// ----------------------------------- TFtpLooper -----------------------------------

TFtpLooper::TFtpLooper(BHandler *target, const char *host, uint16 port,
	const char *username, const char *password, TDirentParser *direntParser)
	:	BLooper("FtpLooper")
{
	_LOG.SetTarget(target);
	fPollingTimer = new BMessageRunner(this, new BMessage(FTP_POLLING_TIMER), POLLING_INTERVAL);
	fMessenger = new BMessenger(target);
	fFtpClient = new TFTPClient();
	fHost.SetTo(host);
	fPort = port;
	fUsername.SetTo(username);
	fPassword.SetTo(password);
	fDirentParser = direntParser;
	fDownloadListingMode = DL_LIST_R_MODE;
}

TFtpLooper::~TFtpLooper()
{
	if (fFtpClient) delete fFtpClient;
	if (fPollingTimer) delete fPollingTimer;
	delete fDirentParser;
}

void TFtpLooper::SetTarget(BHandler *target)
{
	delete fMessenger;
	fMessenger = new BMessenger(target);
}

void TFtpLooper::Abort()
{
	if (fFtpClient) {
		fAbort = true;
	}
}

void TFtpLooper::SendStatusMessage(const char *msgstring)
{
	BMessage msg(FTP_LOG);
	msg.AddString("ftp:statusMsg", msgstring);
	fMessenger->SendMessage(&msg);
}

status_t TFtpLooper::SendResultMessage(status_t e)
{
	BMessage result;
	switch(e) {
		case EPIPE: 
			return SendResultMessage(&result, REQ_KILL_ME, B_TRANSLATE("Disconnected."),
				B_TRANSLATE("Disconnected.\n"), false);
		default:
			return SendResultMessage(&result, REQ_NO_REQUEST, B_TRANSLATE("Idle"), "", false);
	}
}

status_t TFtpLooper::SendResultMessage(BMessage *result, int32 requset, const char *statusMsg,
		const char *logMsg, bool busy)
{
	status_t s = B_OK;
	result->what = FTP_REPORT;
	if ((s = result->AddInt32("ftp:requset", requset)) != B_OK) return s;
	if (statusMsg) if ((s = result->AddString("ftp:statusMsg", statusMsg)) != B_OK) return s;
	if (logMsg) if ((s = result->AddString("ftp:logMsg", logMsg)) != B_OK) return s;
	if ((s = result->AddBool("ftp:busy", busy)) != B_OK) return s;
	fMessenger->SendMessage(result);
	return s;
}

void TFtpLooper::MessageReceived(BMessage *msg)
{
	if (!fFtpClient) return;
	fPollingTimer->SetInterval(INT_MAX);
	switch(msg->what) {
		case FTP_POLLING_TIMER:     PollingTimer(); break;
		case FTP_CONNECT:	        Connect(msg); break;
		case FTP_PASV_LIST:	        GetDirList(); break;
		case FTP_CHDIR:             Chdir(msg); break;
		case FTP_DOWNLOAD:          Download(msg); break;
		case FTP_UPLOAD:            Upload(msg); break;
		case FTP_RENAME:            Rename(msg); break;
		case FTP_MKDIR:             Mkdir(msg); break;
		case FTP_SITE:              Site(msg); break;
		case FTP_SITE_2:            Site2(msg); break;
		case FTP_DELETE:            Delete(msg); break;
//		case FTP_SCANDIR:           RecurseDirList(msg); break;
		default: BLooper::MessageReceived(msg);
	}
	if (fPollingTimer) fPollingTimer->SetInterval(POLLING_INTERVAL);
}


// ----------------------------------------------------------------------------------------

status_t TFtpLooper::SendCmd(const char *command)
{
	fFtpClient->ClearMessages();
	BString cmd(command);
	cmd << "\r\n";
	_LOG << ">" << command << "\n" << _FLUSH;
	return fFtpClient->SendCmd(cmd.String(), cmd.Length());
}

status_t TFtpLooper::Readres(int32 *reply, BString *lastLine)
{
	bool isLast;
	BString str;
	
	while(!fAbort) {
		if (fFtpClient->Status() != B_OK) {
			_LOG << fFtpClient->StrError() << "\n" << _FLUSH;
			return EPIPE;
		}
		status_t s = fFtpClient->GetLastMessage(&str, reply, &isLast);
		if (s == B_OK) {
			if (str.Length() > 0) _LOG << str << "\n" << _FLUSH;
			if (isLast) {
				break;
			}
		}
		snooze(1000);
	}
	
	if (lastLine != NULL) {
		lastLine->SetTo(str.String());
	}
	
	if (fAbort) {
		fFtpClient->ClearMessages();
		return EINTR;
	}
	
	return B_OK;
}

status_t TFtpLooper::USER(const char *userName, int32 *reply)
{
	BString buf("USER ");
	buf << userName;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::PASS(const char *password, int32 *reply)
{
	BString cmd("PASS ");
	cmd << password << "\r\n";
	_LOG << ">PASS ********\n" << _FLUSH;
	status_t e = fFtpClient->SendCmd(cmd.String(), cmd.Length());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::PASV(struct sockaddr_in *addr, int32 *reply)
{
	BString lastLine;
	char *cp;
	memset(addr, 0, sizeof(struct sockaddr_in));
	unsigned int v[6];
	
	status_t e = SendCmd("PASV");
	if (e != B_OK) return e;
	if ((e = Readres(reply, &lastLine)) != B_OK) return e;
	
	if ((cp = strchr(lastLine.String(), '(')) == NULL) {
//		fStrError.SetTo(B_TRANSLATE("Invalid Response."));
		return B_OK;
	}
	cp++;
	
	sscanf(cp, "%u,%u,%u,%u,%u,%u", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
	((uint8 *)&addr->sin_addr.s_addr)[0] = v[0];
	((uint8 *)&addr->sin_addr.s_addr)[1] = v[1];
	((uint8 *)&addr->sin_addr.s_addr)[2] = v[2];
	((uint8 *)&addr->sin_addr.s_addr)[3] = v[3];
	((uint8 *)&addr->sin_port)[0] = v[4];
	((uint8 *)&addr->sin_port)[1] = v[5];
	addr->sin_len = sizeof(struct sockaddr_in);
	addr->sin_family = AF_INET;
	
//	fprintf(stderr, " %d\n", 256 * v[4] + v[5]);
	
	return B_OK;
}

status_t TFtpLooper::LIST(struct sockaddr_in *addr, bool passive, const char *dir, const char *option)
{
	BString buf("LIST "), _dir(dir);
	buf << option;
	if (_dir.Length() > 0) {
		buf << " \"" << _dir << "\"";
	}
	return SendCmd(buf.String());
}

status_t TFtpLooper::XPWD(BString *result, int32 *reply)
{
	BString lastLine;
	status_t e = SendCmd("XPWD");
	if (e != B_OK) return e;
	if ((e = Readres(reply, &lastLine)) != B_OK) return e;
	
	char *s = strchr(lastLine.String(), '"');
	if (!s) {
//		fStrError.SetTo(B_TRANSLATE("Invalid response."));
		return B_ERROR;
	}
	s++;
	char *f = strchr(s, '"');
	if (!f) {
//		fStrError.SetTo(B_TRANSLATE("Invalid response."));
		return B_ERROR;
	}
	*f = 0;
	result->SetTo(s);
	return B_OK;
}

status_t TFtpLooper::CWD(const char *dir, int32 *reply)
{
	BString buf("CWD ");
	buf << dir;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::TYPE(const char *type, int32 *reply)
{
	BString buf("TYPE ");
	buf << type;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::SIZE(const char *remoteFilePath, int64 *size, int32 *reply)
{
	BString lastLine;
	BString buf("SIZE ");
	buf << remoteFilePath;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	if ((e = Readres(reply, &lastLine)) != B_OK) return e;
	
	char *s = strchr(lastLine.String(), '"');
	if (!s) {
//		fStrError.SetTo(B_TRANSLATE("Invalid response."));
		return B_ERROR;
	}

	s++;
	*size = atoll(s);
	return B_OK;
}

status_t TFtpLooper::RETR(const char *remoteFilePath)
{
	BString buf("RETR ");
	buf << remoteFilePath;
	return SendCmd(buf.String());
}

status_t TFtpLooper::STOR(const char *remoteFilePath)
{
	BString buf("STOR ");
	buf << remoteFilePath;
	return SendCmd(buf.String());
}

status_t TFtpLooper::RNFR(const char *remoteFilePath, int32 *reply)
{
	BString buf("RNFR ");
	buf << remoteFilePath;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::RNTO(const char *remoteFilePath, int32 *reply)
{
	BString buf("RNTO ");
	buf << remoteFilePath;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::MKD(const char *dirName, int32 *reply)
{
	BString buf("MKD ");
	buf << dirName;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::SITE(const char *siteParam, int32 *reply)
{
	BString buf("SITE ");
	buf << siteParam;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::REST(off_t startPos, int32 *reply)
{
	BString buf("REST ");
	buf << startPos;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::DELE(const char *remotePath, int32 *reply)
{
	BString buf("DELE ");
	buf << remotePath;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::RMD(const char *remotePath, int32 *reply)
{
	BString buf("RMD ");
	buf << remotePath;
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}

status_t TFtpLooper::NOOP(int32 *reply)
{
	BString buf("NOOP");
	status_t e = SendCmd(buf.String());
	if (e != B_OK) return e;
	return Readres(reply);
}


// ----------------------------------------------------------------------------------------

void TFtpLooper::PollingTimer()
{
/*
	if (!fFtpClient) return;
//	fprintf(stderr, "."); fflush(stderr);
	bool ready;
	int e = fFtpClient->IsReady(0, 0, &ready);
	if (e != 0) {
		// error
		_LOG << strerror(e) << "\n" << _FLUSH;
		return;
	}
	if (ready) {
		if (fFtpClient->Readres(1000000) != B_OK) {
			_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
			delete fFtpClient;
			fFtpClient = NULL;
			delete fPollingTimer;
			fPollingTimer = NULL;
			SendResultMessage(EPIPE);
			return;
		}
		_LOG << fFtpClient->StrReply() << _FLUSH;
	}
*/
}

void TFtpLooper::Connect(BMessage *msg)
{
	fAbort = false;
	
	int32 r;
	status_t e;
	BString remoteDir;
	BString text(B_TRANSLATE("Connecting to %host% (%port%)\n"));
	BString port("");
	port << (uint32)fPort;
	text.ReplaceFirst("%host%", fHost);
	text.ReplaceFirst("%port%", port);

	_LOG << text << _FLUSH;

	if (fFtpClient->Connect(fHost.String(), fPort) != B_OK) {
		_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
		goto _err_exit;
	}
	e = Readres(&r);
	if ((e != B_OK) || (r < 200) || (r > 299)) goto _err_exit;
	
	// ID 送り
	if (USER(fUsername.String(), &r) != B_OK) {
		_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
		goto _err_exit;
	}
	if ((r < 300) || (r > 399)) goto _err_exit;
	
	//パスワード送り
	if (PASS(fPassword.String(), &r) != B_OK) {
		_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
		goto _err_exit;
	}
	if ((r < 200) || (r > 299)) goto _err_exit;
	
	// 最初のディレクトリ一覧取得
	if (msg->FindString("dir", &remoteDir) == B_OK) {
		Chdir(msg);
	} else {
		GetDirList();
	}
	return;
	
_err_exit:
	
	SendResultMessage(EPIPE);
	return;
}

void TFtpLooper::GetDirList()
{
	fAbort = false;
	
	int32 r;
	status_t e = B_OK;
	TEntryList pathList;
	
	if (PasvList("", &pathList, "-alL") == B_OK) {
		// カレントディレクトリの確認
		BString xpwd_result;
		if ((e = XPWD(&xpwd_result, &r)) != B_OK) {
			_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
			goto _err_exit;
		}
		if ((r < 200) || (r > 299)) goto _err_exit;
		
		BMessage entryList;
		
		for(int32 i = 0; i < pathList.CountItems(); i++) {
			entry_item *entry = pathList.EntryAt(i);
			BMessage ent;
			ent.AddString("path", entry->ei_path->String());
			ent.AddString("date", entry->ei_date->String());
			ent.AddString("permission", entry->ei_permission->String());
			ent.AddString("owner", entry->ei_owner->String());
			ent.AddString("group", entry->ei_group->String());
			ent.AddInt64("size", entry->ei_size);
			entryList.AddMessage("entry", &ent);
		}
		
		entryList.AddString("xpwd", xpwd_result);
		SendResultMessage(&entryList, REQ_WALKED, B_TRANSLATE("Idle"), "", false);
		
		return;
	}
	
_err_exit:
	
	SendResultMessage(e);
}

void TFtpLooper::Chdir(BMessage *msg)
{
	fAbort = false;
	
	status_t e = B_OK;
	int32 r;
	BString dir;
	if (msg->FindString("dir", &dir) != B_OK) goto _err_exit;
	
	// CWD
	if ((e = CWD(dir.String(), &r)) != B_OK) {
		_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
//		goto _err_exit;
	}
//	if ((r < 200) || (r > 299)) goto _err_exit;
	
	// 移動後、ディレクトリ一覧取得
	GetDirList();
	return;
	
_err_exit:
	
	SendResultMessage(e);
	return;
}

void TFtpLooper::Download(BMessage *msg)
{
	int32 dontAsk = 0, defaultAnswer = 0;
	
	status_t status = B_OK;
	BPath dirpath;
	BString remoteDirName, localDirName;
	entry_ref localDirRef;
	TProgressWindow *progressWindow;
	bool ignore = false, modal;
	BRect rect(100, 100, 400, 191);
	
	fAbort = false;
	
	// 階層下のファイル・ディレクトリを全て取得
	TEntryList pathList;
	switch(fDownloadListingMode) {
		case DL_LIST_R_MODE:  ListRDirList(msg, &pathList);   break;
		case DL_RECURSE_MODE: RecurseDirList(msg, &pathList); break;
		default: goto _err_exit;
	}
	
	if (fAbort) goto _err_exit;
	
	if (msg->FindString("remote_dir_name", &remoteDirName) != B_OK) goto _err_exit;
	if (msg->FindRef("local_dir", &localDirRef) != B_OK) goto _err_exit;
	dirpath.SetTo(&localDirRef);
	if (dirpath.InitCheck() != B_OK) goto _err_exit;
	
	if (msg->FindRect("rect", &rect) == B_OK) {
		rect.left += 80;
		rect.top += 80;
		rect.right = rect.left + 300;
		rect.bottom = rect.top + 182;
	}
	
	progressWindow = new TProgressWindow(rect, "", &fAbort);
	if (msg->FindBool("modal", &modal) == B_OK) {
		if (modal) {
			progressWindow->SetType(B_MODAL_WINDOW);
		}
	}
	progressWindow->Show();
	
	for(int32 i = 0; i < pathList.CountItems(); i++) {
		entry_item *item = pathList.EntryAt(i);
		BString remotePathName(item->ei_path->String());
		if (remotePathName.FindFirst(remoteDirName) == 0) {
			BString localPathName(remotePathName.String()), utf8localPath;
			if (remoteDirName.Compare("/") != 0) {
				localPathName.RemoveFirst(remoteDirName);
			}
			encoder_addon_manager->ConvertToLocalName(localPathName.String(), &utf8localPath);
			utf8localPath.Prepend(dirpath.Path());
			if (item->ei_permission->String()[0] == 'd') {
				// ディレクトリを作る
				fprintf(stderr, "creating directory %s\n", utf8localPath.String());
				status_t s = create_directory(utf8localPath.String(), 0755);
				if ((s != B_OK) && (!ignore)) {
					BString alertMsg(B_TRANSLATE("Error creating folder '%path%'.\n%error%"));
					alertMsg.ReplaceFirst("%path%", utf8localPath.String());
					alertMsg.ReplaceFirst("%error%", strerror(s));
					int32 reply = (new BAlert("", alertMsg,
						B_TRANSLATE("Ignore all"), B_TRANSLATE("Cancel"), B_TRANSLATE("Continue")))->Go();
					if (reply == 0)
						ignore = true;
					else if (reply == 1)
						break;
				}
			} else {
				// ファイルをダウンロード
				fprintf(stderr, "downloading %s -> %s\n", remotePathName.String(), utf8localPath.String());
				if (PasvDownload(utf8localPath.String(), remotePathName.String(),
								item->ei_size, progressWindow, &dontAsk, &defaultAnswer) != B_OK) {
					status = EPIPE;
					break;
				}
			}
		}
	}
	
	progressWindow->Lock();
	progressWindow->Quit();
	
_err_exit:
	
	SendResultMessage(status);
	return;
}

void TFtpLooper::Rename(BMessage *msg)
{
	fAbort = false;
	
	BString from, to;
	status_t e = B_OK;
	int32 r;
	
	msg->FindString("from", &from);
	msg->FindString("to", &to);
	
	// RNFR
	if ((e = RNFR(from.String(), &r)) != B_OK) {
		_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
		goto _err_exit;
	}
	if ((r < 300) || (r > 399)) goto _err_exit;
	
	// RNTO
	if ((e = RNTO(to.String(), &r)) != B_OK) {
		_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
		goto _err_exit;
	}
	if ((r < 200) || (r > 299)) goto _err_exit;
	
	// ディレクトリ一覧再取得
	GetDirList();
	return;
	
_err_exit:
	
	SendResultMessage(e);
	return;
}

void TFtpLooper::Mkdir(BMessage *msg)
{
	fAbort = false;
	
	BString dirName;
	status_t e = B_OK;
	int32 r;
	
	msg->FindString("dir_name", &dirName);
	
	// MKD
	if ((e = MKD(dirName.String(), &r)) != B_OK) {
		_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
		goto _err_exit;
	}
	if ((r < 200) || (r > 299)) goto _err_exit;
	
	// ディレクトリ一覧再取得
	GetDirList();
	return;
	
_err_exit:
	
	SendResultMessage(e);
	return;
}

void TFtpLooper::Site(BMessage *msg)
{
	fAbort = false;
	
	BString siteParam;
	status_t e = B_OK;
	int32 r;
	
	msg->FindString("site_param", &siteParam);
	
	// SITE
	if ((e = SITE(siteParam.String(), &r)) != B_OK) {
		_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
		goto _err_exit;
	}
	if ((r < 200) || (r > 299)) goto _err_exit;
	
	// ディレクトリ一覧再取得
	GetDirList();
	return;
	
_err_exit:
	
	SendResultMessage(e);
	return;
}

void TFtpLooper::Site2(BMessage *msg)
{
	fAbort = false;
	
	BString siteParam;
	status_t e = B_OK;
	int32 r;
	
	for(int32 i = 0; msg->FindString("site_param", i, &siteParam) == B_OK; i++) {
		// SITE
		if ((e = SITE(siteParam.String(), &r)) != B_OK) {
			_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
			goto _err_exit;
		}
		if ((r < 200) || (r > 299)) goto _err_exit;
		
		if (fAbort) break;
	}
	
	// ディレクトリ一覧再取得
	GetDirList();
	return;
	
_err_exit:
	
	SendResultMessage(e);
	return;
}

// 階層下のファイル・ディレクトリを全て削除
void TFtpLooper::Delete(BMessage *msg)
{
	fAbort = false;
	
	status_t e = B_OK;
	
	// 階層下のファイル・ディレクトリを全て取得
	TEntryList pathList;
	switch(fDownloadListingMode) {
		case DL_LIST_R_MODE:  ListRDirList(msg, &pathList);   break;
		case DL_RECURSE_MODE: RecurseDirList(msg, &pathList); break;
		default:
			SendResultMessage(e);
			return;
	}
	
	// ファイルを全て削除
	for(int32 i = 0; i < pathList.CountItems(); i++) {
		entry_item *item = pathList.EntryAt(i);
		if (item->ei_permission->String()[0] != 'd') {
			DeleteEntry(item->ei_path->String(), false);
		}
	}
	
	// ディレクトリ削除
	for(int32 i = pathList.CountItems() - 1; i >= 0; i--) {
		entry_item *item = pathList.EntryAt(i);
		if (item->ei_permission->String()[0] == 'd') {
			DeleteEntry(item->ei_path->String(), true);
		}
	}
	
	// ディレクトリ一覧再取得
	GetDirList();
	return;
}

// ファイルまたはディレクトリを削除
status_t TFtpLooper::DeleteEntry(const char *remotePath, bool isDirectory)
{
	int32 r;
	status_t e = B_OK;
	
	if (isDirectory) {
		// RMD
		if ((e = RMD(remotePath, &r)) != B_OK) {
			_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
			return e;
		}
	} else {
		// DELE
		if ((e = DELE(remotePath, &r)) != B_OK) {
			_LOG << "Error: " << fFtpClient->StrError() << "\n" << _FLUSH;
			return e;
		}
	}
	if ((r < 200) || (r > 299)) {
		//
	}
	
	return e;
}

// List ----------------------------------------------------------------------

status_t TFtpLooper::PasvListR(const char *dirPath, TEntryList *pathList)
{
	status_t e;
	int32 r;
	struct sockaddr_in addr;
	BMallocIO mallocIO;
	
	// CWD
	if (strlen(dirPath) != 0) {
		if (CWD(dirPath, &r) != B_OK) {
			_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
			return B_ERROR;
		}
		if ((r < 200) || (r > 299)) return B_ERROR;
	}
	
	// PASV モード
	if (PASV(&addr, &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return B_ERROR;
	}
	if ((r < 200) || (r > 299)) return B_ERROR;
	
	// LIST
	if (LIST(&addr, true, "", "-alLR") != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return B_ERROR;
	}
	
	// データ受信
	TTCPStream tcpStream(&mallocIO, &addr, 60000000, RECEIVE_BUFF_SIZE, TCP_STREAM_DOWNLOAD);
	if (tcpStream.Status() != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << strerror(tcpStream.Status()) << "\n" << _FLUSH;
		return B_ERROR;
	}
	
	e = Readres(&r);
	if (e != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << strerror(e) << "\n" << _FLUSH;
		return B_ERROR;
	}
	if ((r < 100) || (r > 199)) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return B_ERROR;
	}
	
	if (tcpStream.Go() != B_OK) {
		_LOG << B_TRANSLATE("Error: Could not spawn thread.\n") << _FLUSH;
		return B_ERROR;
	}

	int64 transferredSize;

	while ((!fAbort) && (tcpStream.Result(&transferredSize) == EINPROGRESS)) {
		char size[B_PATH_NAME_LENGTH];
		string_for_size(transferredSize, size, sizeof(size));
		BString text(B_TRANSLATE("Getting directory list" B_UTF8_ELLIPSIS "%transferred%"));
		text.ReplaceFirst("%transferred%", size);
		SendStatusMessage(text.String());
		snooze(1000);
	}
	if ((!fAbort) && (tcpStream.Result(&transferredSize) == B_OK)) {
		char size[B_PATH_NAME_LENGTH];
		string_for_size(transferredSize, size, sizeof(size));
		BString text(B_TRANSLATE("Getting directory list" B_UTF8_ELLIPSIS "%transferred%. Finished."));
		text.ReplaceFirst("%transferred%", size);
		SendStatusMessage(text.String());
	} else {
		_LOG << B_TRANSLATE("Error:") << " " << strerror(tcpStream.Status()) << "\n" << _FLUSH;
	}
	if (fAbort) return B_ERROR;
	
	e = Readres(&r);
	
	if (mallocIO.BufferLength() == 0) return B_OK;
	
	((char *)mallocIO.Buffer())[mallocIO.BufferLength()] = 0;
	BFile listLog("/var/log/ftp+listr.log", B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	listLog.Write(mallocIO.Buffer(), mallocIO.BufferLength());
	listLog.Unset();
	
	// 取得したディレクトリリストの文字列を解析
	fDirentParser->MakeEmpty();
	fDirentParser->AddEntries((char *)mallocIO.Buffer(), "R");
	for(int32 i = 0; i < fDirentParser->CountEntries(); i++) {
		BString dir, name, date, permission, owner, group;
		int64 size;
		fDirentParser->GetEntryData(i, &dir, &name, &size, &date, &permission, &owner, &group);
		if (name.Compare(".") == 0) continue;
		if (name.Compare("..") == 0) continue;
		BString path;
		if (dir.Length() == 0) {
			path << dirPath << "/" << name;
		} else {
			path << dirPath << "/" << dir << "/" << name;
			path.ReplaceAll("/./", "/");
		}
		pathList->AddEntry(path.String(), date.String(), permission.String(),
							owner.String(), group.String(), size);
	}
	
	return B_OK;
}

void TFtpLooper::ListRDirList(BMessage *msg, TEntryList *pathList)
{
	int32 r;
	
	// カレントディレクトリの保存
	BString xpwd_result;
	if (XPWD(&xpwd_result, &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return;
	}
	if ((r < 200) || (r > 299)) return;
	
	// ディレクトリ走査
	type_code type;
	int32 count;
	if (msg->GetInfo("remote_entry", &type, &count) != B_OK) return;
	for(int32 i = 0; i < count; i++) {
		BMessage entry;
		int64 size;
		if (msg->FindMessage("remote_entry", i, &entry) != B_OK) return;
		BString dirName, leafName, permission, pathName;
		if (entry.FindString("remote_entry_permission", &permission) != B_OK) return;
		if (permission.Length() < 1) continue;
		if (entry.FindString("remote_dir_name", &dirName) != B_OK) return;
		if (entry.FindString("remote_leaf_name", &leafName) != B_OK) return;
		if (entry.FindInt64("remote_file_size", &size) != B_OK) return;
		pathName << dirName << leafName;
		pathList->AddEntry(pathName.String(), "", permission.String(), "", "", size);
		if (permission.String()[0] == 'd') {
			if (PasvListR(pathName.String(), pathList) != B_OK) break;
		}
	}
	
	// カレントディレクトリの復元
	if (CWD(xpwd_result.String(), &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return;
	}
}

status_t TFtpLooper::_recurseDirList(const char *dir, TEntryList *pathList)
{
	TEntryList dlist;
	if (PasvList(dir, &dlist, "-alL") != B_OK) return B_ERROR;
	
	for(int32 i = 0; i < dlist.CountItems(); i++) {
		entry_item *entry = dlist.EntryAt(i);
		pathList->AddEntry(entry);
		if (entry->ei_permission->String()[0] == 'd') {
			status_t s = _recurseDirList(entry->ei_path->String(), pathList);
			if (s != B_OK) return s;
		}
	}
	
	return B_OK;
}

void TFtpLooper::RecurseDirList(BMessage *msg, TEntryList *pathList)
{
	int32 r;
	
	// カレントディレクトリの保存
	BString xpwd_result;
	if (XPWD(&xpwd_result, &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return;
	}
	if ((r < 200) || (r > 299)) return;
	
	// ディレクトリ走査
	type_code type;
	int32 count;
	if (msg->GetInfo("remote_entry", &type, &count) != B_OK) return;
	for(int32 i = 0; i < count; i++) {
		BMessage entry;
		if (msg->FindMessage("remote_entry", i, &entry) != B_OK) return;
		BString dirName, leafName, permission, pathName;
		if (entry.FindString("remote_entry_permission", &permission) != B_OK) return;
		if (permission.Length() < 1) continue;
		if (entry.FindString("remote_dir_name", &dirName) != B_OK) return;
		if (entry.FindString("remote_leaf_name", &leafName) != B_OK) return;
		pathName << dirName << leafName;
		pathList->AddEntry(pathName.String(), "", permission.String(), "", "", 0);
		if (permission.String()[0] == 'd') {
			if (_recurseDirList(pathName.String(), pathList) != B_OK) break;
		}
	}
	
	// カレントディレクトリの復元
	if (CWD(xpwd_result.String(), &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return;
	}
}

status_t TFtpLooper::PasvList(const char *dirPath, TEntryList *pathList, const char *option)
{
	status_t e;
	int32 r;
	struct sockaddr_in addr;
	BMallocIO mallocIO;
	
	// CWD
	if (strlen(dirPath) != 0) {
		if (CWD(dirPath, &r) != B_OK) {
			_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
			return B_ERROR;
		}
		if ((r < 200) || (r > 299)) return B_ERROR;
	}
	
	// PASV モード
	if (PASV(&addr, &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return B_ERROR;
	}
	if ((r < 200) || (r > 299)) return B_ERROR;
	
	// LIST
	if (LIST(&addr, true, "", option) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return B_ERROR;
	}
	
	// データ受信
	TTCPStream tcpStream(&mallocIO, &addr, 60000000, RECEIVE_BUFF_SIZE, TCP_STREAM_DOWNLOAD);
	if (tcpStream.Status() != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << strerror(tcpStream.Status()) << "\n" << _FLUSH;
		return B_ERROR;
	}
	
	e = Readres(&r);
	if (e != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << strerror(e) << "\n" << _FLUSH;
		return B_ERROR;
	}
	if ((e != B_OK) || (r < 100) || (r > 199)) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return B_ERROR;
	}
	
	if (tcpStream.Go() != B_OK) {
		_LOG << B_TRANSLATE("Error: Could not spawn thread.\n") << _FLUSH;
		return B_ERROR;
	}
	
	int64 transferredSize;
	while ((!fAbort) && (tcpStream.Result(&transferredSize) == EINPROGRESS)) {
		BString text(B_TRANSLATE("Getting directory list" B_UTF8_ELLIPSIS));
		text << " " << transferredSize << " bytes.";
		SendStatusMessage(text.String());
		snooze(1000);
	}
	if ((!fAbort) && (tcpStream.Result(&transferredSize) == B_OK)) {
		BString text(B_TRANSLATE("Getting directory list" B_UTF8_ELLIPSIS));
		text << " " << transferredSize << " bytes. Done.";
		SendStatusMessage(text.String());
	} else {
		_LOG << B_TRANSLATE("Error:") << " " << strerror(tcpStream.Status()) << "\n" << _FLUSH;
	}
	fAbort = false;
	
	e = Readres(&r);
	
	if (mallocIO.BufferLength() == 0) return B_OK;
	
	((char *)mallocIO.Buffer())[mallocIO.BufferLength()] = 0;
	BFile listLog("/var/log/ftp+list.log", B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	listLog.Write(mallocIO.Buffer(), mallocIO.BufferLength());
	listLog.Unset();
	
	// 取得したディレクトリリストの文字列を解析
	fDirentParser->MakeEmpty();
	fDirentParser->AddEntries((char *)mallocIO.Buffer(), "");
	for(int32 i = 0; i < fDirentParser->CountEntries(); i++) {
		BString dir, name, date, permission, owner, group;
		int64 size;
		fDirentParser->GetEntryData(i, &dir, &name, &size, &date, &permission, &owner, &group);
		if (name.Compare(".") == 0) continue;
		if (name.Compare("..") == 0) continue;
		BString path;
		if (strlen(dirPath) == 0) {
			path.SetTo(name);
		} else {
			if (dir.Length() == 0) {
				path << dirPath << "/" << name;
			} else {
				path << dirPath << "/" << dir << "/" << name;
				path.ReplaceAll("/./", "/");
			}
		}
		pathList->AddEntry(path.String(), date.String(), permission.String(),
							owner.String(), group.String(), size);
	}
	
	return B_OK;
}

// download ------------------------------------------------------------------

status_t TFtpLooper::_download_alert(const char *name, const char *err)
{
	BString alertMsg(B_TRANSLATE("Downloading: %filename%\n%error%"));
	alertMsg.ReplaceFirst("%filename%", name);
	alertMsg.ReplaceFirst("%error%", err);
	if ((new BAlert("", alertMsg,
			B_TRANSLATE("Cancel"), B_TRANSLATE("Continue")))->Go() == 0)
		return B_ERROR;

	return B_OK;
}

status_t TFtpLooper::PasvDownload(const char *localFilePath, const char *remoteFilePath, int64 size,
	TProgressWindow *progressWindow, int32 *defaultAnswer, int32 *dontAsk)
{
	fAbort = false;
	int32 r;
	status_t e;
	struct sockaddr_in addr;
	off_t startPos = 0;
	
	BPath path(localFilePath);
	
	// Progress Window 表示
	progressWindow->SetProgressInfo(path.Leaf(), false, size);
	
	// 
//	fprintf(stderr, "localfile = %s\n", localFilePath);
	BFile localFile(localFilePath, B_CREATE_FILE | B_FAIL_IF_EXISTS | B_WRITE_ONLY);
	
	switch(localFile.InitCheck()) {
		case B_OK:
			break;
		case B_FILE_EXISTS: {
			// 上書き or レジューム問い合わせ
			if (*dontAsk == 0) {
				BPath path(localFilePath);
				char alertMsg[1000];
				snprintf(alertMsg, sizeof(alertMsg),
					B_TRANSLATE("A local file with the name \"%s\" already exists.\n\n"
					"Would you like to resume, overwrite or skip the download of the file?\n"), path.Leaf());
				TDontAskAgainAlert *alert = new TDontAskAgainAlert("", alertMsg,
					B_TRANSLATE("Resume"), B_TRANSLATE("Overwrite"), B_TRANSLATE("Skip"), dontAsk);
				*defaultAnswer = alert->Go();
			}
			switch(*defaultAnswer){
				case 0:
					// Resume
					localFile.SetTo(localFilePath, B_WRITE_ONLY | B_OPEN_AT_END);
					startPos = localFile.Position();
					break;
				case 1:
					// Overwrite
					localFile.SetTo(localFilePath, B_CREATE_FILE | B_WRITE_ONLY);
					localFile.SetSize(0);
					break;
				default:
					// Skip
					return B_OK;
			}
			break;
		}
		default:
			_LOG << B_TRANSLATE("Error: Download failed.") << " " << strerror(localFile.InitCheck()) << "\n" << _FLUSH;
			return _download_alert(remoteFilePath, strerror(localFile.InitCheck()));
	}
	
	// PASV モード
	if (PASV(&addr, &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return _download_alert(remoteFilePath, fFtpClient->StrError());
	}
	if ((r < 200) || (r > 299)) {
		return _download_alert(remoteFilePath, fFtpClient->StrReply());
	}
	
	// BIN モード
	if (TYPE("I", &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return _download_alert(remoteFilePath, fFtpClient->StrError());
	}
	if ((r < 200) || (r > 299)) {
		return _download_alert(remoteFilePath, fFtpClient->StrReply());
	}
	
	// REST (レジュームする場合)
	if (startPos > 0) {
		if (REST(startPos, &r) != B_OK) {
			_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
			return _download_alert(remoteFilePath, fFtpClient->StrError());
		}
		if ((r < 300) || (r > 399)) {
			return _download_alert(remoteFilePath, fFtpClient->StrReply());
		}
	}
	
	// RETR
	if (RETR(remoteFilePath) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return _download_alert(remoteFilePath, fFtpClient->StrError());
	}
	
	// ファイル受信
	TTCPStream tcpStream(&localFile, &addr, 60000000, RECEIVE_BUFF_SIZE, TCP_STREAM_DOWNLOAD);
	if (tcpStream.Status() != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << strerror(tcpStream.Status()) << "\n" << _FLUSH;
		return _download_alert(remoteFilePath, strerror(tcpStream.Status()));
	}
	
	e = Readres(&r);
	if (e != B_OK) {
		return _download_alert(remoteFilePath, strerror(e));
	}
	if ((r < 100) || (r > 199)) {
		return _download_alert(remoteFilePath, fFtpClient->StrReply());
	}
	
	if (tcpStream.Go() != B_OK) {
		_LOG << B_TRANSLATE("Error: Could not spawn download thread.\n") << _FLUSH;
		return _download_alert(remoteFilePath, B_TRANSLATE("Error: Could not spawn download thread.\n"));
	}
	
	int64 transferredSize;
	while ((!fAbort) && (tcpStream.Result(&transferredSize) == EINPROGRESS)) {
		progressWindow->SetProgress(transferredSize + startPos);
		snooze(100);
	}
	if (fAbort) {
		tcpStream.Abort();
		return B_ERROR;
	} else {
		if (tcpStream.Result(&transferredSize) == B_OK) {
			progressWindow->SetProgress(transferredSize + startPos);
			e = Readres(&r);
		} else {
			//
			_LOG << B_TRANSLATE("Error:") << " " << strerror(tcpStream.Status()) << "\n" << _FLUSH;
		}
	}
	
	return B_OK;
}

// upload ---------------------------------------------------------------------

status_t TFtpLooper::_upload_alert(const char *name, const char *err)
{
	BString alertMsg(B_TRANSLATE("Uploading: %filename%\n%error%"));
	alertMsg.ReplaceFirst("%filename%", name);
	alertMsg.ReplaceFirst("%error%", err);
	if ((new BAlert("", alertMsg,
			B_TRANSLATE("Cancel"), B_TRANSLATE("Continue")))->Go() == 0)
		return B_ERROR;

	return B_OK;
}

status_t TFtpLooper::UploadDir(const char *dirName)
{
	BString text("Creating directory '%directory%'\n");
	text.ReplaceFirst("%directory%", dirName);
	_LOG << text << _FLUSH;
	fprintf(stderr, "Creating directory %s\n", dirName);
	
	status_t e = B_OK;
	int32 r;
	
	// MKD
	if ((e = MKD(dirName, &r)) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return e;
	}
	if ((r < 200) || (r > 299)) {
		//
	}
	
	return e;
}

int32 TFtpLooper::UploadLinkAlert(const char *name, int32 *dontAskTraverse)
{
	BString alertMsg(B_TRANSLATE("The entry\n\"%name%\"\nis a symbolic link. What to do?\n"));
	alertMsg.ReplaceFirst("%name%", name);

	int32 traverseWantTo = (new TDontAskAgainAlert("", alertMsg.String(),
		B_TRANSLATE("Cancel"), B_TRANSLATE("Upload with original name"),
		B_TRANSLATE("Upload with link name"), dontAskTraverse))->Go();
	return traverseWantTo;
}

status_t TFtpLooper::UploadEntry(BEntry *entry, const char *baseDir,
	int32 *dontAskTraverse, int32 *traverseWantTo, TProgressWindow *progressWindow)
{
	BPath path(entry);
	BString leafName(path.Leaf());
	entry_ref ref;
	entry->GetRef(&ref);
	
	if (entry->IsSymLink()) {
		// アップするエントリがシンボリックリンクだった場合
		BEntry linkent(&ref, true);
		if (linkent.InitCheck() != B_OK) return B_OK;
		if (*dontAskTraverse == 0) {
			*traverseWantTo = UploadLinkAlert(path.Path(), dontAskTraverse);
		}
		switch(*traverseWantTo) {
			case 0:
				// Ignore Symbolic link
				return B_OK;
			case 1: {
				// Upload as original name
				entry->SetTo(&ref, true);
				BPath opath(entry);
				leafName.SetTo(opath.Leaf());
				break;
			}
			case 2:
				// Upload as link name
				entry->SetTo(&ref, true);
				break;
		}
	}
	
	if (entry->InitCheck() != B_OK) {
		_LOG << strerror(entry->InitCheck()) << "\n" << _FLUSH;
		return B_ENTRY_NOT_FOUND;
	}
	
	BString uploadPath(baseDir);
	uploadPath << "/" << leafName;
	
	BString remotePath;
	encoder_addon_manager->ConvertToRemoteName(uploadPath.String(), &remotePath);
	status_t e = B_OK;
	
	path.SetTo(entry);
	if (entry->IsDirectory()) {
		// ディレクトリのアップロード
		progressWindow->SetProgressInfo(path.Leaf(), true, 0);
		UploadDir(remotePath.String());
		BDirectory dir(entry);
		e = UploadEntries(&dir, uploadPath.String(), dontAskTraverse, traverseWantTo, progressWindow);
	} else {
		// ファイルのアップロード
		off_t size;
		entry->GetSize(&size);
		progressWindow->SetProgressInfo(path.Leaf(), true, size);
		e = PasvUpload(entry, remotePath.String(), progressWindow);
	}
	progressWindow->Clear();
	
	return e;
}

status_t TFtpLooper::UploadEntries(BDirectory *dir, const char *baseDir,
	int32 *dontAskTraverse, int32 *traverseWantTo, TProgressWindow *progressWindow)
{
	status_t e;
	if ((e = dir->InitCheck()) != B_OK) {
		_LOG << strerror(e) << "\n" << _FLUSH;
		return e;
	}
	
	BEntry entry;
	while ((!fAbort) && (dir->GetNextEntry(&entry) == B_OK) && (e == B_OK)) {
		e = UploadEntry(&entry, baseDir, dontAskTraverse, traverseWantTo, progressWindow);
	}
	
	return e;
}

void TFtpLooper::Upload(BMessage *msg)
{
	int32 dontAskTraverse = 0, traverseWantTo = 0;
	fAbort = false;
	bool modal;
	
	BRect rect(100, 100, 400, 191);
	if (msg->FindRect("rect", &rect) == B_OK) {
		rect.left += 80;
		rect.top += 80;
		rect.right = rect.left + 300;
		rect.bottom = rect.top + 91;
	}
	TProgressWindow *progressWindow = new TProgressWindow(rect, "", &fAbort);
	if (msg->FindBool("modal", &modal) == B_OK) {
		if (modal) {
			progressWindow->SetType(B_MODAL_WINDOW);
		}
	}
	progressWindow->Show();
	
	int32 count;
	type_code type;
	if (msg->GetInfo("refs", &type, &count) != B_OK) return;
	BMessage entries;
	
	status_t status = B_OK;
	for(int32 i = 0; (!fAbort) && (i < count); i++) {
		entry_ref ref;
		status_t e;
		if (msg->FindRef("refs", i, &ref) != B_OK) continue;
		if ((e = entries.AddRef("entries", &ref)) != B_OK) {
			_LOG << strerror(e) << "\n" << _FLUSH;
			return;
		}
		BEntry entry(&ref);
		status = UploadEntry(&entry, ".", &dontAskTraverse, &traverseWantTo, progressWindow);
		if (status != B_OK) {
			break;
		}
	}
	
	progressWindow->Lock();
	progressWindow->Quit();
	
	if (status == B_OK) {
		// ディレクトリ一覧再取得
		GetDirList();
	} else {
		SendResultMessage(status);
	}
	
	return;
}

status_t TFtpLooper::PasvUpload(BEntry *localEntry, const char *remoteFilePath,
	TProgressWindow *progressWindow)
{
	BString text(B_TRANSLATE("Uploading '%remotepath%'\n"));
	text.ReplaceFirst("%remotepath%", remoteFilePath);
	_LOG << text << _FLUSH;
	fprintf(stderr, "Uploading %s\n", remoteFilePath);
	
	status_t e;
	int32 r;
	struct sockaddr_in addr;
	off_t startPos = 0;
	
	BFile localFile(localEntry, B_READ_ONLY);
	if (localFile.InitCheck() != B_OK) return localFile.InitCheck();
	
	// PASV モード
	if (PASV(&addr, &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return _upload_alert(remoteFilePath, fFtpClient->StrError());
	}
	if ((r < 200) || (r > 299)) {
		return _upload_alert(remoteFilePath, fFtpClient->StrReply());
	}
	
	// BIN モード
	if (TYPE("I", &r) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return _upload_alert(remoteFilePath, fFtpClient->StrError());
	}
	if ((r < 200) || (r > 299)) {
		return _upload_alert(remoteFilePath, fFtpClient->StrReply());
	}
	
	// STOR
	if (STOR(remoteFilePath) != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << fFtpClient->StrError() << "\n" << _FLUSH;
		return _upload_alert(remoteFilePath, fFtpClient->StrError());
	}
	
	// ファイル送信
	TTCPStream tcpStream(&localFile, &addr, 60000000, RECEIVE_BUFF_SIZE, TCP_STREAM_UPLOAD);
	if (tcpStream.Status() != B_OK) {
		_LOG << B_TRANSLATE("Error:") << " " << strerror(tcpStream.Status()) << "\n" << _FLUSH;
		return _upload_alert(remoteFilePath, strerror(tcpStream.Status()));
	}
	
	e = Readres(&r);
	if (e != B_OK) {
		return _upload_alert(remoteFilePath, strerror(e));
	}
	if ((r < 100) || (r > 199)) {
		return _upload_alert(remoteFilePath, fFtpClient->StrReply());
	}
	
	if (tcpStream.Go() != B_OK) {
		_LOG << B_TRANSLATE("Error: Could not spawn thread.\n") << _FLUSH;
		return _upload_alert(remoteFilePath, B_TRANSLATE("Error: Could not spawn thread.\n"));
	}
	
	int64 transferredSize;
	while ((!fAbort) && (tcpStream.Result(&transferredSize) == EINPROGRESS)) {
		progressWindow->SetProgress(transferredSize + startPos);
		snooze(100);
	}
	if (fAbort) {
		tcpStream.Abort();
		return EPIPE;
	} else {
		if (tcpStream.Result(&transferredSize) == B_OK) {
			progressWindow->SetProgress(transferredSize + startPos);
			e = Readres(&r);
			_LOG << fFtpClient->StrReply() << _FLUSH;
		} else {
			//
			_LOG << B_TRANSLATE("Error:") << " " << strerror(tcpStream.Status()) << "\n" << _FLUSH;
		}
	}
	
	return B_OK;
}
