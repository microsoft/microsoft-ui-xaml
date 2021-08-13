// WebView2.properties.cpp does a #include "WebView2.h". Our include paths make it so the nuget package's search path wins and
// we get the "wrong" webview2.h header. Add this here to make sure that the right header is picked up.
#include "..\WebView2\WebView2.h"
