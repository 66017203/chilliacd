#ifndef __SIA_H_
#define __SIA_H_

#include <string>
#ifndef WIN32
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#else
#define strdup(src) _strdup(src)
#define fileno _fileno
#define read _read
#include <io.h>
#define CC_NORM         0
#define CC_NEWLINE      1
#define CC_EOF          2
#define CC_ARGHACK      3
#define CC_REFRESH      4
#define CC_CURSOR       5
#define CC_ERROR        6
#define CC_FATAL        7
#define CC_REDISPLAY    8
#define CC_REFRESH_BEEP 9
#define HISTLEN 10
#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_TAB 3
#define CLEAR_OP 4
#define DELETE_REFRESH_OP 5
#define KEY_LEFT 6
#define KEY_RIGHT 7
#define KEY_INSERT 8
#define PROMPT_OP 9
#define KEY_DELETE 10
#endif

enum ConsoleType {
	DEBUG_TYPE,
	NORMAL_TYPE,
	WARNING_TYPE,
	ERROR_TYPE
};

struct cc_directories {
	std::string sys_script;
	std::string user_script;
};

extern cc_directories g_globaldir;


#endif

