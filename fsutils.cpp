#include <malloc.h>
#include <string.h>

// dirname
char *dirname(char *path)
{
	char *newpath;
	char *slash;
	int length;
	
	slash = strrchr(path, '/');
	if (slash == 0) {
		path = ".";
		length = 1;
	} else {
		while (slash > path && *slash == '/') slash--;
		length = slash - path + 1;
	}
	newpath = (char *)malloc(length + 1);
	if (newpath == 0) return 0;
	strncpy (newpath, path, length);
	newpath[length] = 0;
	return newpath;
}

// basename
char *basename(char *path)
{
	char *base = (char *)strrchr(path, '/');
	return base ? base + 1 : path;
}
