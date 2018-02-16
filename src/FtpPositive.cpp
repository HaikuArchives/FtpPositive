#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AboutWindow.h>
#include <Alert.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <Roster.h>
#include <Path.h>
#include <Entry.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FtpPositive"

#include "FtpPositive.h"
#include "MimeDB.h"
#include "EncoderAddonManager.h"

const char* kAppName = B_TRANSLATE_SYSTEM_NAME("FtpPositive");
const char* kAppsignature = "application/x-vnd.momoziro-FtpPositive";

TConfigFile *app_config;


	
TFtpPositive::TFtpPositive()
	:	BApplication(kAppsignature)
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
	
	fFTPWindow = new TFTPWindow(frame, B_TRANSLATE_SYSTEM_NAME("FtpPositive"));
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
	BAboutWindow* about = new BAboutWindow(kAppName, kAppsignature);
	const char* extraCopyrights[] = {
		"2009 Momoziro",
		"2013 Kacper Kasper",
		"2014 Frederik Modeen",
		"2017 Augustin Cavalier, Joel Koreth, Owen Pan, Sergei Reznikov",
		"2018 Humdinger, Janus, Ryo Kenrie Wongso, Xiang Fan",
		NULL
	};
	const char* authors[] = {
		B_TRANSLATE("Momoziro (original author)"),
		"Augustin Cavalier",
		"Humdinger",
		"Kacper Kasper",
		"Joel Koreth",
		"Janus",
		"Frederik Modeen",
		"Owen Pan",
		"Sergei Reznikov",
		"Ryo Kenrie Wongso",
		"Xiang Fan",
		NULL
	};
	about->AddCopyright(2007, "Momoziro", extraCopyrights);
	about->AddAuthors(authors);
	about->Show();
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
