#ifndef _TFTPPOSITIVE_H_
#define _TFTPPOSITIVE_H_

#include <Application.h>
#include <String.h>
#include "FTPWindow.h"
#include "ConfigFile.h"


#define VERSION "1.0"
#define COPY "Copyright ©momoziro 2007 All Right Reserved."


class TFtpPositive : public BApplication
{
public:
	TFtpPositive();
	~TFtpPositive();
	void AboutRequested();
	
	static BString &GetConfigDir();
	static BString &GetBookmarksDir();
	static BString &GetDefaultLocalDir();
	
private:
	TFTPWindow *fFTPWindow;
	
	static BString sConfigDir;
	static BString sBookmarksDir;
	static BString sDefaultLocalDir;
};

extern TConfigFile *app_config;



#endif
