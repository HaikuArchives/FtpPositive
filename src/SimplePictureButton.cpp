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


TSimplePictureButton::TSimplePictureButton(BRect frame, const char *name,
			const char *offBmpResName, uint32 offBmpResType,
			const char *onBmpResName, uint32 onBmpResType,
			const char *disabledBmpResName, uint32 disabledBmpResType,
			BMessage *message,
			uint32 behavior,
			uint32 resizingMode,
			uint32 flags
		)
	:	BPictureButton(frame, name,
			ResToPicture(frame, offBmpResName, offBmpResType),
			ResToPicture(frame, onBmpResName, onBmpResType),
			message, behavior, resizingMode, flags)
{
	SetDisabledOff(ResToPicture(frame, disabledBmpResName, disabledBmpResType));
	SetDisabledOn(ResToPicture(frame, disabledBmpResName, disabledBmpResType));
}

BPicture *TSimplePictureButton::ResToPicture(BRect frame, const char *resName, uint32 resType)
{
	BResources res;
	size_t size;
	app_info appInfo;
	
	be_app->GetAppInfo(&appInfo);
	BFile appFile(&appInfo.ref, B_READ_ONLY);
	res.SetTo(&appFile);
	frame.OffsetTo(0,0);
	
	void *bmp = res.FindResource(resType, resName, &size);
	BMemoryIO aBmpMem(bmp, size);
	BBitmap *aBmp = BTranslationUtils::GetBitmap(&aBmpMem);
	
	BView *aView = new BView(frame, "aView", 0, 0);
	BBitmap *aBitmap = new BBitmap(aView->Bounds(), B_RGB32, true);
	aBitmap->AddChild(aView);
	aBitmap->Lock();
	aView->BeginPicture(new BPicture);
	if (aBmp!=NULL) aView->DrawBitmap(aBmp);
	BPicture *pic = aView->EndPicture();
	aBitmap->Unlock();
	delete aBitmap;
	return pic;
}
