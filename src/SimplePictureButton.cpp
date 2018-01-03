#include <stdio.h>
#include <Application.h>
#include <File.h>
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
