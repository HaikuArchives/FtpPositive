#ifndef _PARSER_H_
#define _PARSER_H_


#include <SupportDefs.h>
#include <List.h>
#include <String.h>
#include <stdlib.h>


typedef struct fp_dirlist_t {
	char *fd_dir;
	char *fd_name;
	int64 fd_size;
	char *fd_date;
	char *fd_permission;
	char *fd_owner;
	char *fd_group;
} fp_dirlist_t;


class TDirentParser
{
public:
	
	TDirentParser() {};
	
	virtual ~TDirentParser() {
		this->MakeEmpty();
	};
	
	void MakeEmpty() {
		for(int32 i = 0; i < EntryList.CountItems(); i++) {
			fp_dirlist_t *entry = (fp_dirlist_t *)EntryList.ItemAt(i);
			free(entry->fd_dir);
			free(entry->fd_name);
			free(entry->fd_date);
			free(entry->fd_owner);
			free(entry->fd_group);
			free(entry);
		}
		EntryList.MakeEmpty();
	}
	
	virtual status_t AddEntries(const char *strDirList, const char *option) = 0;
	
	status_t AddEntry(const char *dir, const char *name, int64 size, const char *date, const char *permission, const char *owner, const char *group) {
		fp_dirlist_t *entry = (fp_dirlist_t *)malloc(sizeof(fp_dirlist_t));
		if (entry == NULL) return ENOMEM;
		if (!EntryList.AddItem(entry)) {
			free(entry);
			return ENOMEM;
		}
		entry->fd_dir = strdup(dir);
		entry->fd_name = strdup(name);

		entry->fd_size = size;

		entry->fd_date = strdup(date);
		entry->fd_permission = strdup(permission);
		entry->fd_owner = strdup(owner);
		entry->fd_group = strdup(group);
		if (!entry->fd_name || !entry->fd_date || !entry->fd_owner || !entry->fd_group) {
			free(entry->fd_name);
			free(entry->fd_date);
			free(entry->fd_owner);
			free(entry->fd_group);
			EntryList.RemoveItem(entry);
			free(entry);
			return ENOMEM;
		}
		return B_OK;
	}
	
	int32 CountEntries() const { return EntryList.CountItems(); };
	
	bool GetEntryData(int32 index, BString *dir, BString *name, int64 *size, BString *date, BString *permission, BString *owner, BString *group) const {
		fp_dirlist_t *entry = (fp_dirlist_t *)EntryList.ItemAt(index);
		if (entry == NULL) return false;
		dir->SetTo(entry->fd_dir);
		name->SetTo(entry->fd_name);
		*size = entry->fd_size;
		date->SetTo(entry->fd_date);
		permission->SetTo(entry->fd_permission);
		owner->SetTo(entry->fd_owner);
		group->SetTo(entry->fd_group);
		return true;
	}
protected:
	BList EntryList;
};


class TGenericDirentParser : public TDirentParser
{
public:
	TGenericDirentParser() : TDirentParser() {};
	~TGenericDirentParser() {};
	status_t AddEntries(const char *strDirList, const char *option);
};


#endif
