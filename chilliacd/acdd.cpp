#include "acdd.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#ifdef WIN32
#include <direct.h>
#endif
#include <signal.h>
#include "src/SiaManager.h"
#include <log4cplus/configurator.h>
#include <log4cplus/loggingmacros.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <fcntl.h>
#include "src/siaserver.h"
#include <atomic>
#include <iostream>
#include "src/SiaPrometheus.h"
#include "src/ACDCallNum.h"

#define CMD_BUFLEN 1024

#ifndef UNICODE
#define UNICODE
#endif

#ifdef WIN32
static int console_bufferInput(char *buf, int len, char *cmd, int key);
#endif

#ifdef HAVE_EDITLINE
#include <histedit.h>
#endif


static int is_color = 1;
static char bare_prompt_str[512] = "";
static int bare_prompt_str_len = 0;
static char prompt_str[512] = "";

std::atomic<bool> g_Running(true);

#define _STRVERSION(str) #str
#define STRVERSION(str) _STRVERSION(str)
const char* siaversion = "sia " STRVERSION(SIAVERSION);

cc_directories g_globaldir;
TSiaManager *g_sia = nullptr;

static int process_command(const std::string & cmd);


static void screen_size(int *x, int *y)
{

#ifdef WIN32
CONSOLE_SCREEN_BUFFER_INFO csbi;
int ret;

if ((ret = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))) {
    if (x) *x = csbi.dwSize.X;
    if (y) *y = csbi.dwSize.Y;
}

#elif defined(TIOCGWINSZ)
struct winsize w;
ioctl(0, TIOCGWINSZ, &w);

if (x) *x = w.ws_col;
if (y) *y = w.ws_row;
#else
if (x) *x = 80;
if (y) *y = 24;
#endif

}


#ifdef HAVE_EDITLINE
static char *prompt(EditLine *e) { return prompt_str; }

static unsigned char console_eofkey(EditLine *el, int ch)
{
LineInfo *line;
/* only exit if empty line */
line = (LineInfo *)el_line(el);
if (line->buffer == line->lastchar) {
    printf("/exit\n\n");
    g_Running = false;
    return CC_EOF;
}
else {
    if (line->cursor != line->lastchar) {
        line->cursor++;
        el_deletestr(el, 1);
    }
    return CC_REDISPLAY;
}
}
#else
#ifdef _MSC_VER
char history[HISTLEN][CMD_BUFLEN + 1];
int iHistory = 0;
int iHistorySel = 0;

static int console_history(char *cmd, int direction)
{
int i;
static int first;
if (direction == 0) {
    first = 1;
    if (iHistory < HISTLEN) {
        if (iHistory && strcmp(history[iHistory - 1], cmd)) {
            iHistorySel = iHistory;
            strcpy(history[iHistory++], cmd);
        }
        else if (iHistory == 0) {
            iHistorySel = iHistory;
            strcpy(history[iHistory++], cmd);
        }
    }
    else {
        iHistory = HISTLEN - 1;
        for (i = 0; i < HISTLEN - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        iHistorySel = iHistory;
        strcpy(history[iHistory++], cmd);
    }
}
else {
    if (!first) {
        iHistorySel += direction;
    }
    first = 0;
    if (iHistorySel < 0) {
        iHistorySel = 0;
    }
    if (iHistory && iHistorySel >= iHistory) {
        iHistorySel = iHistory - 1;
    }
    strcpy(cmd, history[iHistorySel]);
}
return (0);
}

static int console_bufferInput(char *addchars, int len, char *cmd, int key)
{
static int iCmdBuffer = 0;
static int iCmdCursor = 0;
static int ignoreNext = 0;
static int insertMode = 1;
static COORD orgPosition;
static char prompt[80];
int iBuf;
int i;
HANDLE hOut;
CONSOLE_SCREEN_BUFFER_INFO info;
COORD position;
hOut = GetStdHandle(STD_OUTPUT_HANDLE);
GetConsoleScreenBufferInfo(hOut, &info);
position = info.dwCursorPosition;
if (iCmdCursor == 0) {
    orgPosition = position;
}
if (key == PROMPT_OP) {
    if (strlen(cmd) < sizeof(prompt)) {
        strcpy(prompt, cmd);
    }
    return 0;
}
if (key == KEY_TAB) {
    return 0;
}
if (key == KEY_UP || key == KEY_DOWN || key == CLEAR_OP) {
    SetConsoleCursorPosition(hOut, orgPosition);
    for (i = 0; i < (int)strlen(cmd); i++) {
        printf(" ");
    }
    SetConsoleCursorPosition(hOut, orgPosition);
    iCmdBuffer = 0;
    iCmdCursor = 0;
    memset(cmd, 0, CMD_BUFLEN);
}
if (key == DELETE_REFRESH_OP) {
    int l = len < (int)strlen(cmd) ? len : (int)strlen(cmd);
    for (i = 0; i < l; i++) {
        cmd[--iCmdBuffer] = 0;
    }
    iCmdCursor = (int)strlen(cmd);
    printf("%s", prompt);
    GetConsoleScreenBufferInfo(hOut, &info);
    orgPosition = info.dwCursorPosition;
    printf("%s", cmd);
    return 0;
}
if (key == KEY_LEFT) {
    if (iCmdCursor) {
        if (position.X == 0) {
            position.Y -= 1;
            position.X = info.dwSize.X - 1;
        }
        else {
            position.X -= 1;
        }
        SetConsoleCursorPosition(hOut, position);
        iCmdCursor--;
    }
}
if (key == KEY_RIGHT) {
    if (iCmdCursor < (int)strlen(cmd)) {
        if (position.X == info.dwSize.X - 1) {
            position.Y += 1;
            position.X = 0;
        }
        else {
            position.X += 1;
        }
        SetConsoleCursorPosition(hOut, position);
        iCmdCursor++;
    }
}
if (key == KEY_INSERT) {
    insertMode = !insertMode;
}
if (key == KEY_DELETE) {
    if (iCmdCursor < iCmdBuffer) {
        int pos;
        for (pos = iCmdCursor; pos < iCmdBuffer; pos++) {
            cmd[pos] = cmd[pos + 1];
        }
        cmd[pos] = 0;
        iCmdBuffer--;

        for (pos = iCmdCursor; pos < iCmdBuffer; pos++) {
            printf("%c", cmd[pos]);
        }
        printf(" ");
        SetConsoleCursorPosition(hOut, position);
    }
}
for (iBuf = 0; iBuf < len; iBuf++) {
    switch (addchars[iBuf]) {
    case '\r':
    case '\n':
        if (ignoreNext) {
            ignoreNext = 0;
        }
        else {
            int ret = iCmdBuffer;
            if (iCmdBuffer == 0) {
                strcpy(cmd, "Empty");
                ret = (int)strlen(cmd);
            }
            else {
                console_history(cmd, 0);
                cmd[iCmdBuffer] = 0;
            }
            iCmdBuffer = 0;
            iCmdCursor = 0;
            printf("\n");
            return (ret);
        }
        break;
    case '\b':
        if (iCmdCursor) {
            if (position.X == 0) {
                position.Y -= 1;
                position.X = info.dwSize.X - 1;
                SetConsoleCursorPosition(hOut, position);
            }
            else {
                position.X -= 1;
                SetConsoleCursorPosition(hOut, position);
            }
            printf(" ");
            if (iCmdCursor < iCmdBuffer) {
                int pos;
                iCmdCursor--;
                for (pos = iCmdCursor; pos < iCmdBuffer; pos++) {
                    cmd[pos] = cmd[pos + 1];
                }
                cmd[pos] = 0;
                iCmdBuffer--;
                SetConsoleCursorPosition(hOut, position);
                for (pos = iCmdCursor; pos < iCmdBuffer; pos++) {
                    printf("%c", cmd[pos]);
                }
                printf(" ");
                SetConsoleCursorPosition(hOut, position);
            }
            else {
                SetConsoleCursorPosition(hOut, position);
                iCmdBuffer--;
                iCmdCursor--;
                cmd[iCmdBuffer] = 0;
            }
        }
        break;
    default:
        if (!ignoreNext) {
            if (iCmdCursor < iCmdBuffer) {
                int pos;
                if (position.X == info.dwSize.X - 1) {
                    position.Y += 1;
                    position.X = 0;
                }
                else {
                    position.X += 1;
                }
                if (insertMode) {
                    for (pos = iCmdBuffer - 1; pos >= iCmdCursor; pos--) {
                        cmd[pos + 1] = cmd[pos];
                    }
                }
                iCmdBuffer++;
                cmd[iCmdCursor++] = addchars[iBuf];
                printf("%c", addchars[iBuf]);
                for (pos = iCmdCursor; pos < iCmdBuffer; pos++) {
                    GetConsoleScreenBufferInfo(hOut, &info);
                    if (info.dwCursorPosition.X == info.dwSize.X - 1 && info.dwCursorPosition.Y == info.dwSize.Y - 1) {
                        orgPosition.Y -= 1;
                        position.Y -= 1;
                    }
                    printf("%c", cmd[pos]);
                }
                SetConsoleCursorPosition(hOut, position);
            }
            else {
                if (position.X == info.dwSize.X - 1 && position.Y == info.dwSize.Y - 1) {
                    orgPosition.Y -= 1;
                }
                cmd[iCmdBuffer++] = addchars[iBuf];
                iCmdCursor++;
                printf("%c", addchars[iBuf]);
            }
        }
    }
    if (iCmdBuffer == CMD_BUFLEN) {
        printf("Read Console... BUFFER OVERRUN\n");
        iCmdBuffer = 0;
        ignoreNext = 1;
    }
}
return (0);
}

static BOOL console_readConsole(HANDLE conIn, char *buf, int len, int *pRed, int *key)
{
DWORD recordIndex, bufferIndex, toRead, red;
PINPUT_RECORD pInput;
if (GetNumberOfConsoleInputEvents(conIn, &toRead) == 0) {
    return(FALSE);
}
if (len < (int)toRead) {
    toRead = len;
}
if (toRead == 0) {
    return(FALSE);
}
if ((pInput = (PINPUT_RECORD)malloc(toRead * sizeof(INPUT_RECORD))) == NULL) {
    return (FALSE);
}
*key = 0;
ReadConsoleInput(conIn, pInput, toRead, &red);
for (recordIndex = bufferIndex = 0; recordIndex < red; recordIndex++) {
    KEY_EVENT_RECORD keyEvent = pInput[recordIndex].Event.KeyEvent;
    if (pInput[recordIndex].EventType == KEY_EVENT && keyEvent.bKeyDown) {
        if (keyEvent.wVirtualKeyCode == 38 && keyEvent.wVirtualScanCode == 72) {
            buf[0] = 0;
            console_history(buf, -1);
            *key = KEY_UP;
            bufferIndex += (DWORD)strlen(buf);
        }
        if (keyEvent.wVirtualKeyCode == 40 && keyEvent.wVirtualScanCode == 80) {
            buf[0] = 0;
            console_history(buf, 1);
            *key = KEY_DOWN;
            bufferIndex += (DWORD)strlen(buf);
        }
        if (keyEvent.uChar.AsciiChar == 9) {
            *key = KEY_TAB;
            break;
        }
        if (keyEvent.uChar.AsciiChar == 27) {
            *key = CLEAR_OP;
            break;
        }
        if (keyEvent.wVirtualKeyCode == 37 && keyEvent.wVirtualScanCode == 75) {
            *key = KEY_LEFT;
        }
        if (keyEvent.wVirtualKeyCode == 39 && keyEvent.wVirtualScanCode == 77) {
            *key = KEY_RIGHT;
        }
        if (keyEvent.wVirtualKeyCode == 45 && keyEvent.wVirtualScanCode == 82) {
            *key = KEY_INSERT;
        }
        if (keyEvent.wVirtualKeyCode == 46 && keyEvent.wVirtualScanCode == 83) {
            *key = KEY_DELETE;
        }
        while (keyEvent.wRepeatCount && keyEvent.uChar.AsciiChar) {
            buf[bufferIndex] = keyEvent.uChar.AsciiChar;
            if (buf[bufferIndex] == '\r') {
                buf[bufferIndex] = '\n';
            }
            bufferIndex++;
            keyEvent.wRepeatCount--;
        }
    }
}
free(pInput);
*pRed = bufferIndex;
return (TRUE);
}
#endif
#endif

static void handle_SIGINT(int sig)
{
static log4cplus::Logger log = log4cplus::Logger::getRoot();
fprintf(stdout, "Caught SIGINT\n");
LOG4CPLUS_WARN(log, "Caught SIGINT\n");
g_Running = false;
return;
}

static void handle_SIGQUIT(int sig)
{
static log4cplus::Logger log = log4cplus::Logger::getRoot();
fprintf(stdout, "Caught SIGQUIT\n");
LOG4CPLUS_WARN(log, "Caught SIGQUIT\n");
g_Running = false;
return;
}

#ifdef WIN32
static HANDLE hStdout;
static WORD wOldColorAttrs;
static CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
#endif



static const char *cli_usage =
"Command                    \tDescription\n"
"-----------------------------------------------\n"
"/help                      \tHelp\n"
"/version                   \tVersion\n"
"/exit, /quit, /bye, ...    \tExit the program.\n"
"lua                        \tlua thread run\n"
"\n";

static int process_command(const std::string & cmd)
{

if (cmd == "help") {
    std::cout << cli_usage;
    return 0;
}
if (cmd == "version") {
    std::cout << "     " << siaversion << "\n     2014 Megranate Corp\n\n";
    return 0;
}
if (cmd == "exit" ||
    cmd == "quit" ||
    cmd == "..." ||
    cmd == "bye")
{
    return -1;
}
else {
    std::cout << "Unknown command [" << cmd << "]\n";
    return 0;
}
}


static const char *banner =
".=======================================================.\n"
"|             ____                  ____                |\n"
"|            / ___|                / ___|               |\n"
"|           | |                   | |                   |\n"
"|           | |___                | |___                |\n"
"|           \\ \\___|               \\ \\___|               |\n"
"|                                                       |\n"
".=======================================================.\n"
"| Ronaldo.zhao                                          |\n"
".=======================================================.\n"
"\n";

static const char *inf = "Type /help <enter> to see a list of commands\n\n\n";

static void print_banner(FILE *stream, int color)
{
	int x = 0;

	screen_size(&x, NULL);


#ifdef WIN32
	/* Print banner in yellow with blue background */
	if (color) {
		SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | BACKGROUND_BLUE);
	}
	WriteFile(hStdout, banner, (DWORD)strlen(banner), NULL, NULL);
	if (color) {
		SetConsoleTextAttribute(hStdout, wOldColorAttrs);
	}
#endif
	/* Print the rest info in default colors */
	//fprintf(stream, "\n%s\n", inf);


    /*
	if (x < 160) {
		fprintf(stream, "\n[This app Best viewed at 160x60 or more..]\n");
	}
    */
}

void InitGlobalDir()
{
	g_globaldir.sys_script = "./sys_scripts";
	g_globaldir.user_script = "./user_scripts";

/*
#ifdef WIN32
	_mkdir(g_globaldir.sys_script.c_str());
	_mkdir(g_globaldir.user_script.c_str());
#else
	mkdir(g_globaldir.sys_script.c_str(), 0777);
	mkdir(g_globaldir.user_script.c_str(), 0777);
#endif
*/
}

//获取应用程序的绝对路径
static const char* get_execute_path()
{
    static char path[MAX_PATH + 64] = "";
    if (*path)
        return path;
#ifdef WIN32
    ::GetModuleFileName(::GetModuleHandle(NULL), path, MAX_PATH);

    assert(strrchr(path, '\\'));
    *strrchr(path, '\\') = 0;
#else //_LINUX
    char buff[MAX_PATH + 64] = "";
    sprintf(buff, "/proc/%d/exe", getpid());

    getcwd(path, MAX_PATH);
    if (readlink(buff, path, MAX_PATH) == -1)
        return path;

    assert(strrchr(path, '/'));
    *strrchr(path, '/') = 0;
#endif
    return path;
}

void change_current_dir()
{
#ifdef WIN32
	_chdir(get_execute_path());
#else
    chdir(get_execute_path());
#endif
}

static void libevent_log(int severity, const char *msg)
{
    static log4cplus::Logger log = log4cplus::Logger::getInstance("libevent");
    switch (severity) {
    case EVENT_LOG_DEBUG: LOG4CPLUS_DEBUG(log,msg); break;
    case EVENT_LOG_MSG:   LOG4CPLUS_INFO(log, msg);   break;
    case EVENT_LOG_WARN:  LOG4CPLUS_WARN(log, msg);  break;
    case EVENT_LOG_ERR:   LOG4CPLUS_ERROR(log, msg); break;
    default:               LOG4CPLUS_TRACE(log, msg);     break; /* never reached */
    }
}

static void libevent_fatal(int err)
{
    static log4cplus::Logger log = log4cplus::Logger::getInstance("libevent");
    LOG4CPLUS_FATAL(log, err);
}

int main(int argc, char **argv)
{
    change_current_dir();
	std::string parameter;
	if (argc>1 && argv[1]){
		const char * para = argv[1];
		while (*para && !isalpha(*para)){
			para++;
		}
		if (para)
			parameter = para;
	}
	if (parameter == "start")
	{

#ifdef __linux__

	pid_t pid;
	/* 屏蔽一些有关控制终端操作的信号
	* 防止在守护进程没有正常运转起来时，因控制终端受到干扰退出或挂起
	* */

	signal(SIGINT, SIG_IGN);// 终端中断  
	signal(SIGHUP, SIG_IGN);// 连接挂断  
	signal(SIGQUIT, SIG_IGN);// 终端退出  
	signal(SIGPIPE, SIG_IGN);// 向无读进程的管道写数据  
	signal(SIGTTOU, SIG_IGN);// 后台程序尝试写操作  
	signal(SIGTTIN, SIG_IGN);// 后台程序尝试读操作  
	signal(SIGTERM, SIG_IGN);// 终止  

	pid = fork();
	if (pid < 0)
	{
		perror("fork error!");
		exit(1);
	}
	else if (pid > 0)
	{
		exit(0);
	}

	// [2] create a new session  
	setsid();

	// [3] set current path  

	int fd;
	//[4]将标准输入输出重定向到空设备
	fd = open("/dev/null", O_RDWR, 0);
	if (fd != -1)
	{
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > 2)
			close(fd);
	}
	// [5] umask 0  
	umask(0);

#endif
	}
	else if (parameter == "version" || parameter == "v" || parameter == "V"){
		std::cout << siaversion << std::endl;
		return 0;
	}
	log4cplus::initialize();
    log4cplus::ConfigureAndWatchThread logconfig(LOG4CPLUS_TEXT("../conf/log4cplus.properties"), 10 * 1000);
    event_set_log_callback(libevent_log);
    event_set_fatal_callback(libevent_fatal);
#ifdef WIN32
	evthread_use_windows_threads();
#else
	evthread_use_pthreads();
#endif

    static log4cplus::Logger log = log4cplus::Logger::getRoot();
	LOG4CPLUS_INFO(log, siaversion);


	InitGlobalDir();


#ifdef WIN32
	WORD    wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2, 0);
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		LOG4CPLUS_ERROR(log, "WSAStartup ERROR!!");
		return 0;
	}
#endif

	strcpy(bare_prompt_str, "<Center>");
	bare_prompt_str_len = (int)strlen(bare_prompt_str);

	snprintf(prompt_str, sizeof(prompt_str), "%s", bare_prompt_str);

	signal(SIGINT, handle_SIGINT);
#ifdef SIGTSTP
	signal(SIGTSTP, handle_SIGINT);
#endif
#ifdef SIGQUIT
	signal(SIGQUIT, handle_SIGQUIT);
#endif
#ifdef SIGTERM
	signal(SIGTERM, handle_SIGQUIT);
#endif
#ifdef WIN32
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdout != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) {
		wOldColorAttrs = csbiInfo.wAttributes;
	}
#endif
	print_banner(stdout, is_color);

	g_sia = new TSiaManager("../conf/acd.cfg");
	g_sia->Start();
    
    SiaPrometheus * prometheus = new SiaPrometheus;
    prometheus->start();
    
    ReadConfig2Redis * rc2r = &ReadConfig2Redis::get_instance();
    rc2r->set_sql_config("../conf/acd.cfg");
    rc2r->load_data_from_mysql();
    rc2r->save_data_2_redis();
	GatewayManager::get_instance().set_sql_config("../conf/acd.cfg");
	GatewayManager::get_instance().get_data_from_redis();
	GatewayManager::get_instance().get_route_by_caller("t110110");
	std::string str_callee("1234567890");
	GatewayManager::get_instance().get_modified_callee_by_callee(str_callee);
	GatewayManager::get_instance().update_current_sessions_by_route_id(1, 1);


	std::string strCmd;
	while (g_Running) {
		std::cout << prompt_str;
		std::cin >> strCmd;
		if (!strCmd.empty()) {
			if (process_command(strCmd) == -1)
				break;
		}

		std::cin.clear();
        // add begin by jzwu at 2018-12-19 16:49:06
#if defined(linux)
        std::cin.ignore(std::cin.rdbuf()->in_avail(), '\n');
//        std::cin.ignore(10000, '\n');
#endif
        // add end
		std::cin.sync();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	g_sia->Stop();
	delete g_sia;
	//log4cplus::unInitialize();
	return 0;
}


