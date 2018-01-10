/*
 * Based on StyledEdit and ShowImage StatusView
 *
 */
#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H

#include <String.h>
#include <View.h>

enum {
	kItemsNumberCell,
	kStatusCellCount
};


class StatusView : public BView {
public:
								StatusView(BScrollBar* scrollBar);

	virtual	void				AttachedToWindow();
	virtual void				GetPreferredSize(float* _width, float* _height);
	virtual	void				ResizeToPreferred();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				Update(
									const BString& text, const BString& pages,
									const BString& imageType);
			void				SetText(const char *);
 private:
			void				_SetItemsNumberText(const BString& text);
			void				_SetRenamesNumberText(const BString& pages);
			void				_SetDuplicatesNumberText(const BString& imageType);
			void				_ValidatePreferredSize();
			BScrollBar*			fScrollBar;
			BSize				fPreferredSize;
			BString				fCellText[kStatusCellCount];
			float				fCellWidth[kStatusCellCount];
};


#endif	// STATUS_VIEW_H
