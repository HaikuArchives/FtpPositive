#include <strings.h>
#include <Messenger.h>
#include <Mime.h>
#include <stdio.h>
#include "RemoteFileView.h"
#include "FTPWindow.h"


TStringColumn::TStringColumn(const char* title, float width, float minWidth,
						 float maxWidth, uint32 truncate, alignment align)
	:	BStringColumn(title, width, minWidth, maxWidth, truncate, align)
{
}

TStringColumn::~TStringColumn()
{
}

// ディレクトリ優先のファイル名ソート関数 (改造 BColumnListView が必要)
int TStringColumn::CompareFields(BField *field1, BField *field2, BRow *row1, BRow *row2)
{
	BStringField *pf1 = (BStringField *)row1->GetField(CLM_PERMISSION);
	BStringField *pf2 = (BStringField *)row2->GetField(CLM_PERMISSION);
	const char *s1 = pf1->String(), *s2 = pf2->String();
	bool isdir1 = false, isdir2 = false;
	if (strlen(s1) > 0) isdir1 = s1[0] == 'd';
	if (strlen(s2) > 0) isdir2 = s2[0] == 'd';
	if (isdir1 ^ isdir2) {
		return isdir1 ? -1 : 1;
	} else {
		return ICompare(((BStringField *)field1)->String(), ((BStringField *)field2)->String());
	}
}

TRemoteFileView::TRemoteFileView(BRect frame, const char *name)
	:	BColumnListView(frame, name, B_FOLLOW_ALL_SIDES, B_NAVIGABLE, B_FANCY_BORDER)
{
	fISReseter = NULL;
}

TRemoteFileView::~TRemoteFileView()
{
	if (fISReseter) delete fISReseter;
}

void TRemoteFileView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case MSG_ISWORD_RESET:
			fISWord.SetTo("");
			if (fISReseter) delete fISReseter;
			fISReseter = NULL;
			break;
		default: BColumnListView::MessageReceived(msg);
	}
}

// 外から View へ Drop された
void TRemoteFileView::MessageDropped(BMessage *msg, BPoint point)
{
	switch(msg->what){
		case B_SIMPLE_DATA:
			// ファイルが Drop された
			BMessenger(this->Window()).SendMessage(msg);
			break;
		default: BColumnListView::MessageDropped(msg, point);
	}
}

void TRemoteFileView::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes <= 0) return;
//	int32 mod = modifiers();
	
	switch (bytes[0]) {
		case B_DELETE:
			BMessenger(this->Window()).SendMessage(MSG_DELETE);
			break;
		case B_BACKSPACE:
			BMessenger(this->Window()).SendMessage(MSG_BACKWARD_CLICKED);
			break;
		case B_FUNCTION_KEY: {
			// F5 key (Reload)
			key_info info;
			get_key_info(&info);
			if ((info.key_states[0] & 0x2) != 0) BMessenger(this->Window()).SendMessage(MSG_RELOAD_CLICKED);
			break;
		}
		default: {
			if (bytes[0] >= 0x20) {
				// incremental search
				if (fISReseter) delete fISReseter;
				fISReseter = new BMessageRunner(this, new BMessage(MSG_ISWORD_RESET), 1500000, 1);
				fISWord.Append(bytes);
				SearchAndSelect(fISWord.String());
			} else {
				BColumnListView::KeyDown(bytes, numBytes);
			}
		}
	}
}

void TRemoteFileView::MouseDown(BPoint point)
{
	BPoint currentPos;
	uint32 buttons;
	GetMouse(&currentPos, &buttons);
	if (buttons == 2) BMessenger(this->Window()).SendMessage(MSG_MENU_OPEN);
}

uint32 TRemoteFileView::GetSelectedEntries(BMessage *entries, BRect *rect)
{
	BRow *row = NULL;
	BRect rowRect;
	uint32 cnt = 0;
	float top = FLT_MAX, bottom = FLT_MIN;
	
	while((row = this->CurrentSelection(row))) {
		BMessage intMsg((uint32) B_OK);
		BStringField *intNameField = (BStringField *)row->GetField(CLM_INTERNAL_NAME);
		BStringField *permField = (BStringField *)row->GetField(CLM_PERMISSION);
		BSizeField *sizeField = (BSizeField *)row->GetField(CLM_SIZE);
		if (strcmp(intNameField->String(), ".") == 0) continue;
		if (strcmp(intNameField->String(), "..") == 0) continue;
		intMsg.AddString("remote_dir_name", fCurrentRemoteDir.String());
		intMsg.AddString("remote_leaf_name", intNameField->String());
		intMsg.AddString("remote_entry_permission", permField->String());
		intMsg.AddInt64("remote_file_size", sizeField->Size());
		entries->AddMessage("remote_entry", &intMsg);
		
		// DnD の矩形計算
		this->GetRowRect(row, &rowRect);
		top = min_c(rowRect.top, top);
		bottom = max_c(rowRect.bottom, bottom);
		
		cnt++;
	}
	rect->left = Bounds().left;
	rect->top = top;
	rect->right = Bounds().right;
	rect->bottom = bottom;
	return cnt;
}

// View から外へ Drag and Drop 開始
bool TRemoteFileView::InitiateDrag(BPoint, bool)
{
	BMessage msg;
	BRect rect;
	if (GetSelectedEntries(&msg, &rect) == 0) return false;
	
	// drop 先の Tracker ウィンドウの Path を教えてもらうスクリプトを送る -> FTPWindow.cpp で Reply を受け取り、ダウンロード開始
	msg.what = B_GET_PROPERTY;
	msg.AddSpecifier("Path");
	DragMessage(&msg, rect, Window());
	
	return true;
}

// Icon 描画
void TRemoteFileView::DrawLatch(BView *view, BRect rect, LatchType type, BRow *row)
{
	// outline_view 以外は無視 (でないと、表示が乱れる)
	if (strcmp(view->Name(), "outline_view") != 0) return;
	
	TIconRow *_row = dynamic_cast<TIconRow *>(row);
	if (_row == NULL) return;
	
	view->SetDrawingMode(B_OP_OVER);
	view->DrawBitmap(_row->Icon(), BPoint(4, rect.top));
}

void TRemoteFileView::SetRemoteDir(const char *remoteDir)
{
	fCurrentRemoteDir.SetTo(remoteDir);
	uint32 len = strlen(remoteDir);
	if (len > 0) {
		if (remoteDir[len - 1] != '/') fCurrentRemoteDir << "/";
	}
}

const char *TRemoteFileView::RemoteDir() const
{
	return fCurrentRemoteDir.String();
}

void TRemoteFileView::SearchAndSelect(const char *word)
{
	int32 len = strlen(word);
	this->DeselectAll();
	for(int32 i = 0; i < this->CountRows(); i++) {
		BRow *row = this->RowAt(i);
		BStringField *nameField = (BStringField *)row->GetField(CLM_NAME);		
		if (strncasecmp(nameField->String(), word, len) == 0) {
			this->SetFocusRow(row, true);
			this->ScrollTo(row);
			break;
		}
	}
}
