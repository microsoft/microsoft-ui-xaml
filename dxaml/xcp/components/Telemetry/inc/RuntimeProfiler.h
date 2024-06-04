// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define RuntimeProfiler_Constructor     9999

namespace RuntimeProfiler
{
    enum ProfileGroup
    {
        PG_Class = 0,                        //  All controls
        PG_IslandDesktopWindowXamlSource = 2 //  Controls created through an instance of DesktopWindowXamlSource
    };

    void FireEvent(_In_ bool bSuspend);
    void RegisterMethod(_In_ ProfileGroup group, _In_ PCSTR pszIdentifier, _In_ UINT16 uTypeIndex, _In_ UINT16 uMethodIndex, _Inout_ volatile LONG *pCount);
}

#if !defined(EXP_CLANG)
    #define __RP_Marker(group, friendlyname, typeindex, methodindex) \
        __pragma (message ("RuntimeProfiler Inventory:[" #group "|" #friendlyname "," #typeindex "," #methodindex "]:'" __FUNCSIG__ "'")) \
        { \
            static volatile LONG __RuntimeProfiler_Counter = -1; \
            if (0 == ::InterlockedIncrement(&__RuntimeProfiler_Counter)) \
            { \
                RuntimeProfiler::RegisterMethod(group, friendlyname, (UINT16)typeindex, (UINT16)methodindex, &__RuntimeProfiler_Counter); \
            } \
        }

    #define __RP_Marker_WUX(group, friendlyname, typeindex, methodindex) \
        __pragma (message ("RuntimeProfiler Inventory:[" #group "|" #friendlyname "," #typeindex "," #methodindex "]:'" __FUNCSIG__ "'")) \
        { \
            static volatile LONG __RuntimeProfiler_Counter = -1; \
            if (0 == ::InterlockedIncrement(&__RuntimeProfiler_Counter)) \
            { \
                RuntimeProfiler::RegisterMethod(group, friendlyname, (UINT16)typeindex, (UINT16)methodindex, &__RuntimeProfiler_Counter); \
            } \
        }
#else
    #define __RP_Marker(group, friendlyname, typeindex, methodindex) \
        { \
            static volatile LONG __RuntimeProfiler_Counter = -1; \
            if (0 == ::InterlockedIncrement(&__RuntimeProfiler_Counter)) \
            { \
                RuntimeProfiler::RegisterMethod(group, friendlyname, (UINT16)typeindex, (UINT16)methodindex, &__RuntimeProfiler_Counter); \
            } \
        }

    #define __RP_Marker_WUX(group, friendlyname, typeindex, methodindex) \
        { \
            static volatile LONG __RuntimeProfiler_Counter = -1; \
            if (0 == ::InterlockedIncrement(&__RuntimeProfiler_Counter)) \
            { \
                RuntimeProfiler::RegisterMethod(group, friendlyname, (UINT16)typeindex, (UINT16)methodindex, &__RuntimeProfiler_Counter); \
            } \
        }
#endif

#define __RP_Marker_ClassById(typeindex) __RP_Marker_WUX(RuntimeProfiler::PG_Class, nullptr, typeindex, RuntimeProfiler_Constructor)
#define __RP_Marker_ClassMemberById(typeindex, memberindex) __RP_Marker_WUX(RuntimeProfiler::PG_Class, nullptr, typeindex, memberindex)
#define __RP_Marker_ClassByName(friendlyname) __RP_Marker(RuntimeProfiler::PG_Class, friendlyname, 0, 0)
