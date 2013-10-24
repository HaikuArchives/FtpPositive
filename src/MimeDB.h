#ifndef _MIME_DB_H_
#define _MIME_DB_H_

#include <Mime.h>
#include <List.h>
#include <Bitmap.h>


class TMimeDB
{
public:
	TMimeDB();
	~TMimeDB();
	void GetMimeIcon(const char *mime, BBitmap *icon, icon_size which);
	void GetExtensionIcon(const char *fileName, BBitmap *icon, icon_size which);
	
private:
	BList fExtList;
};


extern TMimeDB *app_mimedb;


#endif
