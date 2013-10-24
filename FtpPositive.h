#ifndef _TFTPPOSITIVE_H_
#define _TFTPPOSITIVE_H_

#include <Application.h>
#include "FTPWindow.h"
#include "ConfigFile.h"


#define VERSION "1.0"
#define COPY "Copyright ©momoziro 2007 All Right Reserved."

#define CONFIG_DIR "/boot/home/config/settings/FtpPositive"
#define BOOKMARKS_DIR "/boot/home/config/settings/FtpPositive/bookmarks"
#define DEFAULT_LOCAL_DIR "/boot/home/Desktop"


class TFtpPositive : public BApplication
{
public:
	TFtpPositive();
	~TFtpPositive();
	void AboutRequested();
	
private:
	TFTPWindow *fFTPWindow;
};

extern TConfigFile *app_config;



#endif
