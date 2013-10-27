#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Alert.h>
#include <FindDirectory.h>
#include <Roster.h>
#include <Path.h>
#include <Entry.h>

#include "FtpPositive.h"
#include "MimeDB.h"
#include "EncoderAddonManager.h"


TConfigFile *app_config;


	
TFtpPositive::TFtpPositive()
	:	BApplication("application/x-vnd.momoziro-FtpPositive")
{
	BRect frame;
	BString v;
	
	mkdir(TFtpPositive::GetConfigDir().String(), 0777);
	mkdir(TFtpPositive::GetBookmarksDir().String(), 0777);
	app_config = new TConfigFile(TFtpPositive::GetConfigDir().Append("/FtpPositive.cnf").String());
	
	app_info info;
	if (be_app->GetAppInfo(&info) == B_OK) {
		BEntry entry(&info.ref), parent;
		if (entry.GetParent(&parent) == B_OK) {
			BPath path(&parent);
			path.Append("add-ons/encoders");
			encoder_addon_manager = new TEncoderAddonManager();
			encoder_addon_manager->LoadAddons(path.Path());
		}
	}
	
	app_mimedb = new TMimeDB();
	
	v.SetTo(""); app_config->Read("frame_left",   &v, "100"); frame.left   = atoi(v.String());
	v.SetTo(""); app_config->Read("frame_top",    &v, "100"); frame.top    = atoi(v.String());
	v.SetTo(""); app_config->Read("frame_right",  &v, "600"); frame.right  = atoi(v.String());
	v.SetTo(""); app_config->Read("frame_bottom", &v, "450"); frame.bottom = atoi(v.String());
	
	fFTPWindow = new TFTPWindow(frame, "FtpPositive");
	fFTPWindow->Show();
}

TFtpPositive::~TFtpPositive()
{
	status_t e;
	delete app_mimedb;
	delete encoder_addon_manager;
	if ((e = app_config->Update()) != B_OK) {
		fprintf(stderr, "%s\n", strerror(e));
	}
	delete app_config;
}
	
void TFtpPositive::AboutRequested()
{
	(new BAlert("", "FtpPositive " VERSION "\n" COPY, "OK"))->Go();
}


BString &TFtpPositive::GetConfigDir()
{
	if (sConfigDir == B_EMPTY_STRING) {
		BPath path;
		find_directory(B_USER_SETTINGS_DIRECTORY, &path);
		sConfigDir = path.Path();
		sConfigDir += "/FtpPositive";
	}
	
	return sConfigDir;
}

BString &TFtpPositive::GetBookmarksDir()
{
	if (sBookmarksDir == B_EMPTY_STRING)
		sBookmarksDir = GetConfigDir().Append("/bookmarks");
	
	return sBookmarksDir;
}

BString &TFtpPositive::GetDefaultLocalDir()
{
	if (sDefaultLocalDir == B_EMPTY_STRING) {
		BPath path;
		find_directory(B_DESKTOP_DIRECTORY, &path);
		sDefaultLocalDir = path.Path();
	}
	
	return sDefaultLocalDir;
}

BString TFtpPositive::sConfigDir = B_EMPTY_STRING;
BString TFtpPositive::sBookmarksDir = B_EMPTY_STRING;
BString TFtpPositive::sDefaultLocalDir = B_EMPTY_STRING;


int main(int argc, char *argv[])
{
	TFtpPositive app;
	app.Run();
	return 0;
}
