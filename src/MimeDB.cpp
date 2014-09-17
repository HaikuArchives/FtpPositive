#include <strings.h>
#include <Message.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "MimeDB.h"


TMimeDB *app_mimedb;


typedef struct ext_item_t {
	char ei_ext[32];
	char ei_mimetype[64];
} ext_item_t;


TMimeDB::TMimeDB()
{
	BMessage types, ext;
	BMimeType mime;
	BMimeType::GetInstalledTypes(&types);
	const char *typeName, *extName;
	ext_item_t *item;
	for (int32 i = 0; types.FindString("types", i, &typeName) == B_OK; i++) {
		mime.SetTo(typeName);
		if (mime.GetFileExtensions(&ext) == B_OK) {
			fprintf(stderr, "MimeDB: %s ->", typeName);
			for(int32 k = 0; ext.FindString("extensions", k, &extName) == B_OK; k++) {
				if ((item = (ext_item_t *)malloc(sizeof(ext_item_t))) == NULL) break;
				strncpy(item->ei_ext, extName, sizeof(item->ei_ext));
				strncpy(item->ei_mimetype, typeName, sizeof(item->ei_mimetype));
				fExtList.AddItem(item);
				fprintf(stderr, " .%s", extName);
			}
			fprintf(stderr, "\n");
		}
	}
}

TMimeDB::~TMimeDB()
{
	for(int32 i = 0; i < fExtList.CountItems(); i++) free(fExtList.ItemAt(i));
}

void TMimeDB::GetMimeIcon(const char *mime, BBitmap *icon, icon_size which)
{
	BMimeType mimeType;
	mimeType.SetTo(mime);
	if (mimeType.GetIcon(icon, which) != B_OK) {
		mimeType.SetTo(B_FILE_MIME_TYPE);
		mimeType.GetIcon(icon, which);
	}
}

void TMimeDB::GetExtensionIcon(const char *fileName, BBitmap *icon, icon_size which)
{
	BMimeType mimeType(B_FILE_MIME_TYPE);
	const char *ext = strrchr(fileName, '.');
	if (ext) {
		ext++;
		for (int32 i = 0; i < fExtList.CountItems(); i++) {
			ext_item_t *item = (ext_item_t *)fExtList.ItemAt(i);				
			if (strcasecmp(ext, item->ei_ext) == 0) {
				mimeType.SetTo(item->ei_mimetype);
				break;
			}
		}
	}
	if (mimeType.GetIcon(icon, which) != B_OK) {
		mimeType.SetTo(B_FILE_MIME_TYPE);
		mimeType.GetIcon(icon, which);
	}
}
