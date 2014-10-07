#include <malloc.h>
#include <string.h>

// dirname
char *dirname(const char *path)
{
	char *newpath;
	char *slash;
	int length;
	
	char oldPath[sizeof(path)];
	strncpy (oldPath, path, sizeof(path));
	
	slash = strrchr(oldPath, '/');
	
	if (slash == 0) {
		strncpy (oldPath, ".", sizeof("."));
		length = 1;
	} else {
		while (slash > oldPath && *slash == '/')
			slash--;
		length = slash - oldPath + 1;
	}
	newpath = (char *)malloc(length + 1);
	
	if (newpath == 0)
		return 0;
	
	strncpy (newpath, oldPath, length);
	newpath[length] = 0;
	return newpath;
}

// basename
char *basename(char *path)
{
	char *base = (char *)strrchr(path, '/');
	return base ? base + 1 : path;
}
