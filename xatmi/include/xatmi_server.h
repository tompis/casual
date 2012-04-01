//!
//! xatmi_server.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#ifndef XATMI_SERVER_H_
#define XATMI_SERVER_H_

#include "xatmi.h"
#include <stddef.h>

struct casual_service_name_mapping
{
	tpservice m_functionPointer;
	const char* m_name;
};


#ifdef __cplusplus
extern "C" {
#endif

int casual_startServer( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size);


#ifdef __cplusplus
}
#endif


#endif /* XATMI_SERVER_H_ */
