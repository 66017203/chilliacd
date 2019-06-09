#pragma once
#include <string>
#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h> 
#endif

static int mkpath(const std::string &s, int mode = 0755)
{
	size_t pre = 0, pos;
	std::string dir;
	int mdret = 0;

#ifdef WIN32
	char slash = '\\';
#else
	char slash = '/';
#endif

	while ((pos = s.find_first_of(slash, pre)) != std::string::npos) {
		dir = s.substr(0, pos++);
		pre = pos;
		if (dir.size() == 0) continue; // if leading / first time is 0 length  
#ifdef WIN32
		if ((mdret = ::_mkdir(dir.c_str())) && errno != EEXIST) {
#else
		if ((mdret = ::mkdir(dir.c_str(), mode)) && errno != EEXIST) {
#endif
			return mdret;
		}
	}
	return mdret;
}

