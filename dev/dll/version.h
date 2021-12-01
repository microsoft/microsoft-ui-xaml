#pragma once

// Macro to convert the value of a variable to a string
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)


#define VER_FILE_VERSION            MUX_VERSION_MAJOR, MUX_VERSION_MINOR, MUX_VERSION_BUILD, MUX_VERSION_REVISION
#define VER_FILE_VERSION_STR        STRINGIZE(MUX_VERSION_MAJOR)        \
                                    "." STRINGIZE(MUX_VERSION_MINOR)    \
                                    "." STRINGIZE(MUX_VERSION_BUILD)    \
                                    "." STRINGIZE(MUX_VERSION_REVISION) \

#define VER_PRODUCT_VERSION         VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR     VER_FILE_VERSION_STR

