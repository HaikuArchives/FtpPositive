#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "ConfigFile.h"

TConfigFile::TConfigFile(const char *pathName)
{
	fPathName.SetTo(pathName);
	fStatus = Load();
}

TConfigFile::~TConfigFile()
{
	config_t *item;
	for(int32 i = 0; i < fList.CountItems(); i++) {
		item = (config_t *)fList.ItemAt(i);
		delete item;
	}
}

status_t TConfigFile::Load()
{
	int32 textlen;
	char text[2048], *value;
	BString buff;
	config_t *item;
	FILE *fd = fopen(fPathName.String(), "r");
	if (!fd) return errno;
	while (!feof(fd)) {
		if (fgets(text, sizeof(text), fd) == NULL) break;
		buff.Append(text);
		textlen = strlen(text);
		if (text[textlen - 1] == '\n') {
			buff.RemoveLast("\n");
			value = strchr(buff.String(), ' ');
			if (value != NULL) {
				if ((item = (config_t *)calloc(sizeof(config_t), 1)) == NULL) {
					fclose(fd);
					return ENOMEM;
				}
				if (!fList.AddItem(item)) {
					free(item);
					fclose(fd);
					return ENOMEM;
				}
				*value = 0;
				value++;
				strncpy(item->key, buff.String(), sizeof(item->key));
				strncpy(item->value, value, sizeof(item->value));
			}
			buff.SetTo("");
		}
	}
	fclose(fd);
	return B_OK;
}

status_t TConfigFile::Update()
{
	status_t e = B_OK;
	config_t *item;
	TStringEx text, key, value;
	FILE *fd = fopen(fPathName.String(), "w");
	if (!fd) return errno;
	for(int32 i = 0; i < fList.CountItems(); i++) {
		item = (config_t *)fList.ItemAt(i);
		text.SetTo("");
		key.SetTo(item->key);
		value.SetTo(item->value);
		key.Trim();
		value.Trim();
		text << key << " " << value << "\n";
		fwrite(text.String(), text.Length(), 1, fd);
	}
	fclose(fd);
	return e;
}

void TConfigFile::Read(const char *key, BString *value, const char *defaultValue) const
{
	TStringEx _key(key);
	_key.Trim();
	config_t *item;
	for(int32 i = 0; i < fList.CountItems(); i++) {
		item = (config_t *)fList.ItemAt(i);
		if (strncmp(_key.String(), item->key, sizeof(item->key)) == 0) {
			value->SetTo(item->value);
			return;
		}
	}
	value->SetTo(defaultValue);
}

status_t TConfigFile::Write(const char *key, const char *value)
{
	TStringEx _key(key);
	_key.Trim();
	config_t *item;
	for(int32 i = 0; i < fList.CountItems(); i++) {
		item = (config_t *)fList.ItemAt(i);
		if (strncmp(_key.String(), item->key, sizeof(item->key)) == 0) {
			strncpy(item->value, value, sizeof(item->value));
			return B_OK;
		}
	}
	if ((item = (config_t *)calloc(sizeof(config_t), 1)) == NULL) return ENOMEM;
	if (!fList.AddItem(item)) {
		free(item);
		return ENOMEM;
	}
	strncpy(item->key, _key.String(), sizeof(item->key));
	strncpy(item->value, value, sizeof(item->value));
	return B_OK;
}
