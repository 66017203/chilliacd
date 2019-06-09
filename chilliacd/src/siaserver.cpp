
//#include "lua.hpp"
#include "siaserver.h"
#include "../acdd.h"
#include <memory.h>
#include <stdlib.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

/*
extern "C" {
#define toproto(L,i) getproto(L->top+(i))

	int luaopen_siaserver(lua_State* L);
}
*/


#define MAX_SIA_SERVER_STR_LENGTH    260

#ifndef _access
#ifndef __LINUX__
#define access _access
#endif
#endif

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

/*
static int sia_panic(lua_State * L)
{
	static log4cplus::Logger log = log4cplus::Logger::getInstance("siaserver");
	LOG4CPLUS_WARN(log, "unprotected error in call to Lua API (" << lua_tostring(L, -1) << ")");
	return 0;
}
*/

//static int sia_traceback(lua_State * L)
//{
//	lua_getglobal(L, "debug");
//	if (!lua_istable(L, -1)) {
//		lua_pop(L, 1);
//		return 1;
//	}
//	lua_getfield(L, -1, "traceback");
//	if (!lua_isfunction(L, -1)) {
//		lua_pop(L, 2);
//		return 1;
//	}
//	lua_pushvalue(L, 1);		/* pass error message */
//	lua_pushinteger(L, 2);		/* skip this function and traceback */
//	lua_call(L, 2, 1);			/* call debug.traceback */
//	return 1;
//}

/*
void sia_stackDump(lua_State * l_luastate)
{
	static log4cplus::Logger log = log4cplus::Logger::getInstance("siaserver");

	int i;
	char buffer[MAX_SIA_SERVER_STR_LENGTH];
	int top = lua_gettop(l_luastate);
	memset(buffer, 0, MAX_SIA_SERVER_STR_LENGTH);

	LOG4CPLUS_INFO(log, "the size of stack is:" << top);
	for (i = 1; i <= top; i++)
	{
		int type = lua_type(l_luastate, i);
		sprintf(buffer, "Stack(%d) ", i);
		switch (type)
		{
		case LUA_TSTRING:
		{
			LOG4CPLUS_INFO(log, buffer << lua_tostring(l_luastate, i));
			break;
		}

		case LUA_TBOOLEAN:
		{
			LOG4CPLUS_INFO(log, buffer << (lua_toboolean(l_luastate, i) ? "true" : "false"));
			break;
		}
		case LUA_TNUMBER:
		{
			LOG4CPLUS_INFO(log, buffer << lua_tonumber(l_luastate, i));
			break;
		}
		case LUA_TTABLE:
		{
			LOG4CPLUS_INFO(log, buffer << "this is a table!");
			break;
		}
		default:
		{
			LOG4CPLUS_INFO(log, buffer << lua_typename(l_luastate, i));
			break;
		}
		}
	}
}
*/

//void sia_addSearchPath(lua_State * l_luastate, const char* path)
//{
//	if (l_luastate) {
//		lua_getglobal(l_luastate, "package");                                  /* L: package */
//		lua_getfield(l_luastate, -1, "path");                /* get package.path, L: package path */
//		const char* cur_path = lua_tostring(l_luastate, -1);
//		lua_pushfstring(l_luastate, "%s;%s/?.lua", cur_path, path);            /* L: package path newpath */
//		lua_setfield(l_luastate, -3, "path");          /* package.path = newpath, L: package path */
//		lua_pop(l_luastate, 2);                                                /* L: - */
//	}
//	return;
//}
//
//int sia_docall(lua_State * L, int narg, int nresults, int perror, int fatal, log4cplus::Logger &log, uint64_t sessionid)
//{
//	int status;
//	int base = lua_gettop(L) - narg;	/* function index */
//
//	lua_pushcfunction(L, sia_traceback);	/* push traceback function */
//	lua_insert(L, base);		/* put it under chunk and args */
//
//	status = lua_pcall(L, narg, nresults, base);
//
//	lua_remove(L, base);		/* remove traceback function */
//								/* force a complete garbage collection in case of errors */
//	if (status != 0) {
//		lua_gc(L, LUA_GCCOLLECT, 0);
//	}
//
//	if (status && perror) {
//		const char *err = lua_tostring(L, -1);
//		if (err) {
//			LOG4CPLUS_WARN(log, sessionid << " <docall> " << err);
//		}
//
//		// pass error up to top
//		if (fatal) {
//			lua_error(L);
//		}
//		else {
//			lua_pop(L, 1); /* pop error message from the stack */
//		}
//	}
//
//	return status;
//}
//
//int sia_dofile(lua_State * l_luastate, const char *filename, log4cplus::Logger &log, uint64_t sessionid, int sys_or_user )
//{
//
//	int error = 0;
//	std::string file;
//	file = g_globaldir.sys_script + "/" + filename;
//
//	std::string fsys_path = g_globaldir.sys_script;
//
//	if ((access(file.c_str(), 0)) == -1) {
//		file = g_globaldir.user_script + "/" + filename;
//		fsys_path = g_globaldir.user_script;
//	}
//
//	if ((access(file.c_str(), 0)) == -1){
//		LOG4CPLUS_ERROR(log, sessionid << " filename[" << file << "] is not exist");
//		return -1;
//	}
//
//	error = luaL_loadfile(l_luastate, file.c_str());
//
//	sia_addSearchPath(l_luastate, fsys_path.c_str());
//
//	if (error == LUA_OK)
//		error = sia_docall(l_luastate, 0, 0, 0, 1, log, sessionid);
//	if (error != LUA_OK) {
//		const char *msg = lua_tostring(l_luastate, -1);
//		LOG4CPLUS_ERROR(log, sessionid << " do file[" << file << "],err:" << msg);
//		lua_pop(l_luastate, 1);  /* remove message */
//	}
//
//	return error;
//
//}

/*
lua_State * sia_luainit(log4cplus::Logger &log, uint64_t sessionid)
{
	lua_State * l_luastate = NULL;
	l_luastate = luaL_newstate();
	int error = 0;

	if (l_luastate) {
		const char *buff = "os.exit = function() siaserver.verbose(\"error\", \"Surely you jest! exiting is a bad plan....\\n\") end";
		lua_gc(l_luastate, LUA_GCSTOP, 0);
		luaL_openlibs(l_luastate);
		luaopen_siaserver(l_luastate);
		lua_gc(l_luastate, LUA_GCRESTART, 0);
		lua_atpanic(l_luastate, sia_panic);
		error = luaL_loadbuffer(l_luastate, buff, strlen(buff), "line") || sia_docall(l_luastate, 0, 0, 0, 1, log, sessionid);

	}
	return l_luastate;
}
*/

/*
void sia_luauninit(lua_State * l_luastate)
{
	if (l_luastate) {
		lua_gc(l_luastate, LUA_GCCOLLECT, 0);
		lua_close(l_luastate);
	}
}
*/

///////////////////////////////////
/*
struct sialua_thread_helper {
	char *input;
	lua_State * luastate;
};
*/


