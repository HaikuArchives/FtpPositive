#include <stdio.h>
#include <Application.h>
#include <File.h>
#include <IconUtils.h>
#include <Roster.h>
#include <TranslationUtils.h>
#include <DataIO.h>
#include <Picture.h>
#include <Bitmap.h>
#include <Resources.h>
#include <View.h>
#include "SimplePictureButton.h"


TSimplePictureButton::TSimplePictureButton()
{
}

BBitmap *TSimplePictureButton::ResToBitmap(const char *resName, uint32 resType)
{
	BResources res;
	size_t size;
	app_info appInfo;
	
	be_app->GetAppInfo(&appInfo);
	BFile appFile(&appInfo.ref, B_READ_ONLY);
	res.SetTo(&appFile);
	
	void *bmp = res.FindResource(resType, resName, &size);
	BMemoryIO aBmpMem(bmp, size);
	BBitmap *aBmp = BTranslationUtils::GetBitmap(&aBmpMem);
	return aBmp;
}


BBitmap *TSimplePictureButton::ResVectorToBitmap(const char *resName)
{
	BResources res;
	size_t size;
	app_info appInfo;

	be_app->GetAppInfo(&appInfo);
	BFile appFile(&appInfo.ref, B_READ_ONLY);
	res.SetTo(&appFile);
	BBitmap *aBmp = NULL;
	const uint8* iconData = (const uint8*) res.LoadResource('VICN', resName, &size);

	if (size > 0 ) {
		aBmp = new BBitmap (BRect(0,0,24,24), 0, B_RGBA32);
		status_t result = BIconUtils::GetVectorIcon(iconData, size, aBmp);
		if (result != B_OK) {
			delete aBmp;
			aBmp = NULL;
		}
	}
	return aBmp;
}
