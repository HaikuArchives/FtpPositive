#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include <List.h>
#include "StringEx.h"


typedef struct config_t {
	char key[64];
	char value[1024];
} config_t;

class TConfigFile
{
public:
	TConfigFile(const char *pathName);
	~TConfigFile();
	status_t Load();
	status_t Update();
	void Read(const char *key, BString *value, const char *defaultValue) const;
	status_t Write(const char *key, const char *value);
	status_t Status() const { return fStatus; };
private:
	BList fList;
	BString fPathName;
	status_t fStatus;
};


#endif
