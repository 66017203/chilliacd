#ifndef __SIA_SERVER_H_
#define __SIA_SERVER_H_

#include <log4cplus/logger.h>

#include "lua.hpp"

void sia_addSearchPath(lua_State * l_luastate, const char* path);
int sia_docall(lua_State * L, int narg, int nresults, int perror, int fatal, log4cplus::Logger & log, uint64_t sessionid);
int sia_dofile(lua_State * l_luastate, const char *filename, log4cplus::Logger &log, uint64_t sessionid, int sys_or_user = 1);
lua_State * sia_luainit(log4cplus::Logger &log, uint64_t sessionid);
void sia_luauninit(lua_State * l_luastate);

#endif
