#ifndef _SIMPLEPICTUREBUTTON_H_
#define _SIMPLEPICTUREBUTTON_H_

#include <PictureButton.h>

class TSimplePictureButton : public BPictureButton
{
public:
	TSimplePictureButton(BRect frame, const char *name,
			const char *offBmpResName, uint32 offBmpResType,
			const char *onBmpResName, uint32 onBmpResType,
			const char *disabledBmpResName, uint32 disabledBmpResType,
			BMessage *message, uint32 behavior = B_ONE_STATE_BUTTON,
			uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE
		);
	
	static BPicture *ResToPicture(BRect frame, const char *resName, uint32 resType);
private:
};


#endif
