#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "EncoderAddonManager.h"


TEncoderAddonManager *encoder_addon_manager;


typedef struct encoder_addon_t {
	image_id id;
	fp_text_convert_t fp_text_convert;
	char name[B_FILE_NAME_LENGTH];
} encoder_addon_t;


TEncoderAddonManager::TEncoderAddonManager()
{
	fCurrentFunc = NULL;
}

TEncoderAddonManager::~TEncoderAddonManager()
{
	UnloadAddons();
}

status_t TEncoderAddonManager::LoadAddons(const char *addondDir)
{
	fp_text_convert_t fp_text_convert;
	image_id id;
	BDirectory dir(addondDir);
	BEntry entry;
	
	// load encoders add-on
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath addonPath(&entry);
		if ((id = load_add_on(addonPath.Path())) < 0) continue;
		if (get_image_symbol(id, "fp_text_convert", B_SYMBOL_TYPE_TEXT, (void **)&fp_text_convert)) {
			unload_add_on(id);
			continue;
		}
		encoder_addon_t *addon = (encoder_addon_t *)malloc(sizeof(encoder_addon_t));
		if (!addon) {
			fprintf(stderr, "%s\n", strerror(ENOMEM));
			unload_add_on(id);
			return ENOMEM;
		}
		addon->id = id;
		addon->fp_text_convert = fp_text_convert;
		strncpy(addon->name, addonPath.Leaf(), sizeof(addon->name));
		fEncoderAddonList.AddItem(addon);
		fprintf(stderr, "load: %s\n", addon->name);
	}
	return B_OK;
}

void TEncoderAddonManager::UnloadAddons()
{
	for(int32 i = 0; i < fEncoderAddonList.CountItems(); i++) {
		encoder_addon_t *addon = (encoder_addon_t *)fEncoderAddonList.ItemAt(i);
		fprintf(stderr, "unload: %s\n", addon->name);
		unload_add_on(addon->id);
		free(addon);
	}
	fCurrentFunc = NULL;
}

int32 TEncoderAddonManager::CountAddons() const
{
	return fEncoderAddonList.CountItems();
}

const char *TEncoderAddonManager::NameAt(int32 index) const
{
	encoder_addon_t *addon = (encoder_addon_t *)fEncoderAddonList.ItemAt(index);
	if (addon == NULL) return NULL;
	return addon->name;
}

fp_text_convert_t TEncoderAddonManager::FuncAt(const char *name) const
{
	for(int32 i = 0; i < fEncoderAddonList.CountItems(); i++) {
		encoder_addon_t *addon = (encoder_addon_t *)fEncoderAddonList.ItemAt(i);
		if (strcmp(name, addon->name) == 0) {
			return addon->fp_text_convert;
		}
	}
	return NULL;
}

status_t TEncoderAddonManager::SetEncoder(const char *name)
{
	fCurrentFunc = FuncAt(name);
	return (fCurrentFunc == NULL) ? B_ERROR : B_OK;
}

void TEncoderAddonManager::ConvertToLocalName(const char *from, BString *to)
{
	if (fCurrentFunc == NULL) {
		to->SetTo(from);
	} else {
		fCurrentFunc(from, to, true);
	}
}

void TEncoderAddonManager::ConvertToRemoteName(const char *from, BString *to)
{
	if (fCurrentFunc == NULL) {
		to->SetTo(from);
	} else {
		fCurrentFunc(from, to, false);
	}
}
