#ifndef __features_FeatureStaging_SampleFeature
#define __features_FeatureStaging_SampleFeature
#endif

#ifndef WIL_STAGING_DLL
#define WIL_STAGING_DLL
#endif

// wil does not work well with code analysis enabled so we disable some warnings:
#pragma warning(push) 
#pragma warning(disable : 6001)
#pragma warning(disable : 6319)
#pragma warning(disable : 6387)
#pragma warning(disable : 6553)
#pragma warning(disable : 26460)
#pragma warning(disable : 26461)
#pragma warning(disable : 26462)
#pragma warning(disable : 26495)
#pragma warning(disable : 26496)
#pragma warning(disable : 26497)
#pragma warning(disable : 26812)

#include <wil/ResultMacros.h>
using namespace wil::details;
#include <wil/Staging.h>

WI_DEFINE_FEATURE(
    Feature_NWMTest1, 39145991, DisabledByDefault,
    WilStagingChangeTime(OnReboot),
    WilStagingGroup("", R"()"));

WI_DEFINE_FEATURE(
    Feature_Tabs, 37634385, DisabledByDefault,
    WilStagingChangeTime(OnReboot),
    WilStagingRequiresFeature(Feature_NWMTest1),
    WilStagingGroup("", R"()"));

#pragma warning(pop)