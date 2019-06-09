/*******************************************************************************
 * Copyright (C) 2019 ling-ban Ltd. All rights reserved.
 * 
 * file  : base.h
 * author: jzwu
 * date  : 2019-03-28
 * remark: 常用宏
 * 
 ******************************************************************************/

#ifndef MY_BASE_H_
#define MY_BASE_H_

#include <sstream>

#ifdef _STR
#undef _STR
#endif

#ifdef STR
#undef STR
#endif

#ifdef IFNULLRETURNFALSE
#undef IFNULLRETURNFALSE
#endif

#ifdef IFNULLRETURNVOID
#undef IFNULLRETURNVOID
#endif

#ifdef IFNULLRETURNCAUSE
#undef IFNULLRETURNCAUSE
#endif

////////////////////////////////////////////////////////////////////////////////

#define _STR(str) #str
#define STR(str) _STR(str)
// param --> a pointer
// lineno --> line number
// func --> function name
// param_name --> parameter's name
#define IFNULLRETURNFALSE(param, lineno, func, param_name) do \
{ \
    if (!(param)) { \
        std::stringstream ss;\
        ss << "ERROR: FUNCTION [" << func << "] LINE [" << lineno << "] PARAMETER NAME [" << param_name << "] IS NULL\n";\
        cout << ss.str() << endl; \
        throw ss.str().data(); \
        return false; \
    }\
} while (0)

// param --> a pointer
// lineno --> line number
// func --> function name
// param_name --> parameter's name
#define IFNULLRETURNVOID(param, lineno, func, param_name) do \
{ \
    if (!(param)) { \
        std::stringstream ss;\
        ss << "ERROR: FUNCTION [" << func << "] LINE [" << lineno << "] PARAMETER NAME [" << param_name << "] IS NULL\n";\
        cout << ss.str() << endl; \
        throw ss.str().data(); \
        return ; \
    }\
} while (0)

// param --> a pointer
// lineno --> line number
// func --> function name
// param_name --> parameter's name
// code --> error code
#define IFNULLRETURNVOID2(param, lineno, func, param_name, code) do \
{ \
    if (!(param)) { \
        std::stringstream ss;\
        ss << "ERROR: FUNCTION [" << func << "] LINE [" << lineno << "] PARAMETER NAME [" << param_name << "] IS NULL RETURN WITH CODE " << code << "\n";\
        cout << ss.str() << endl; \
        throw ss.str().data(); \
        return (code); \
    }\
} while (0)

#endif  // MY_BASE_H

