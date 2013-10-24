#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Alert.h>
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
	
	mkdir(CONFIG_DIR, 0777);
	mkdir(BOOKMARKS_DIR, 0777);
	app_config = new TConfigFile(CONFIG_DIR "/FtpPositive.cnf");
	
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


int main(int argc, char *argv[])
{
	TFtpPositive app;
	app.Run();
	return 0;
}
