// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CppWinRTIncludes.h"

//
// Usage: Add any downlevel APIs that you need to check the existence of here,
// including the name of the method, the type on which it exists,
// the base interface implemented by that type that is guaranteed to exist,
// and the new interface on which the method exists that may or may not exist.
//
// Doing that will automatically add everything that we need to DownlevelHelper,
// and you can then check DownlevelHelper::<your method>Exists() to verify
// that it is present before calling it.
//
#define DOWNLEVEL_API_OPERATION(Operation) \
    Operation(ToDisplayName, winrt::ColorHelper, winrt::IColorHelperStatics, winrt::IColorHelperStatics2) \
    Operation(SetIsTranslationEnabled, winrt::ElementCompositionPreview, winrt::IElementCompositionPreviewStatics, winrt::IElementCompositionPreviewStatics2) \


#define ADD_DOWNLEVEL_API(ApiName, Type, BaseInterface, NewInterface) \
public: \
    static bool ApiName##Exists() \
    { \
        if (!s_##ApiName##ExistsInitialized) \
        { \
            auto factory = winrt::get_activation_factory<Type, BaseInterface>(); \
            s_##ApiName##Exists = factory.try_as<NewInterface>() != nullptr; \
            s_##ApiName##ExistsInitialized = true; \
        } \
        \
        return s_##ApiName##Exists; \
} \
\
private: \
    static bool s_##ApiName##ExistsInitialized; \
    static bool s_##ApiName##Exists;

#define ADD_DOWNLEVEL_API_STATIC_DEFINITION(ApiName, Unused1, Unused2, Unused3) \
bool DownlevelHelper::s_##ApiName##ExistsInitialized{ false }; \
bool DownlevelHelper::s_##ApiName##Exists{ false };

class DownlevelHelper
{
    DOWNLEVEL_API_OPERATION(ADD_DOWNLEVEL_API)

private:
    DownlevelHelper() {}
};