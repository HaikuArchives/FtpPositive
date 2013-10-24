#ifndef _ENCODER_ADDON_MANAGER_H_
#define _ENCODER_ADDON_MANAGER_H_

#include <image.h>
#include <List.h>
#include "EncoderAddon.h"


class TEncoderAddonManager
{
public:
	TEncoderAddonManager();
	~TEncoderAddonManager();
	status_t LoadAddons(const char *addondDir);
	void UnloadAddons();
	int32 CountAddons() const;
	const char *NameAt(int32 index) const;
	fp_text_convert_t FuncAt(const char *name) const;
	
	status_t SetEncoder(const char *name);
	void ConvertToLocalName(const char *from, BString *to);
	void ConvertToRemoteName(const char *from, BString *to);
	
private:
	BList fEncoderAddonList;
	fp_text_convert_t fCurrentFunc;
};


extern TEncoderAddonManager *encoder_addon_manager;


#endif
