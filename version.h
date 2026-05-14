#define _VERSION_ "1.1.4"
#define _BUILD_TYPE_ "Release"
struct _version_ { char* version; char* machine; char* date; };
_version_ _v_ = { .version = (char *)_VERSION_, .machine = (char *)"YourMachine", .date = (char *)"07.11.1917" };
