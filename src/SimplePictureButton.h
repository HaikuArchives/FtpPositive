#ifndef _SIMPLEPICTUREBUTTON_H_
#define _SIMPLEPICTUREBUTTON_H_

#include <PictureButton.h>

class TSimplePictureButton
{
public:
	TSimplePictureButton();
	
	static BPicture *ResToPicture(BRect frame, const char *resName, uint32 resType);
	static BBitmap *ResToBitmap(const char *resName, uint32 resType);
private:
};


#endif
