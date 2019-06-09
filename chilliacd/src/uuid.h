#pragma once
#include <string>
#include <iostream>
#ifdef WIN32
#include <objbase.h>
#else
#include <uuid/uuid.h>
#endif

namespace helper {
	std::string uuid()
	{
#define GUID_LEN 64
		char buffer[GUID_LEN] = { 0 };
		uuid_t guid;

#ifdef WIN32

		if (CoCreateGuid(&guid)) {
			std::cerr << "create guid error\n";
			return buffer;
		}
		_snprintf(buffer, sizeof(buffer),
			"%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2],
			guid.Data4[3], guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
#else
		uuid_generate(guid);
		snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X", 
			guid[0],guid[1],guid[2],guid[3],
			guid[4],guid[5],guid[6],guid[7],
			guid[8],guid[9],guid[10],guid[11],
			guid[12],guid[13],guid[14],guid[15]);

#endif
		//std::cout << "create guid " << buffer << std::endl;
		return buffer;
	}
}
