#pragma once
#include "mlogger.hpp"

#define logmessage logger->log
#define logperror(x) logger->log(ERROR, "In %s() line %d. %s: %s", __FUNCTION__,  __LINE__, x, strerror(errno))
#define logcppinfo *logger<<INFO
#define logcppwarn *logger<<WARN
#define logcpperror *logger<<ERROR<<"In "<<__FUNCTION__<<"() line "<<__LINE__<<". "
#define logcppdebug *logger<<DEBUG<<"In "<<__FUNCTION__<<"() line "<<__LINE__<<". "

extern cLogger *logger;
