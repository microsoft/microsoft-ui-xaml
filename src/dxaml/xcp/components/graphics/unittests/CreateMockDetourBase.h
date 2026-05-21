// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Turn off validation of mock function prototypes as this requires setting up symbols
#define NO_MOCK_VALIDATION
#include <detours.h>

#define FAILFAST_IF_FAILED(__x) { auto __result = __x; if (NO_ERROR != __result) { abort(); }}

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    //  CreateMockDetourBase - base class for creating RAII detours for various creation functions
    //
    //  T is the type of a function (not a pointer to a function) that describes the function
    //  being used to create an object.
    //
    //  Constructor accepts the dll and function names as parameter
    //
    //  Then use the SetDetour function replace the real create function.
    
    template<typename T> 
    class CreateMockDetourBase
    {
    private:
        T* m_realFunction = nullptr;
        T* m_mockFunction = nullptr;
        HMODULE m_LoadLibraryModule = NULL;

    protected:
        void SetDetour(T* function)
        {
            FAILFAST_IF_FAILED(DetourTransactionBegin());
            FAILFAST_IF_FAILED(DetourUpdateThread(GetCurrentThread()));
            FAILFAST_IF_FAILED(DetourAttach(&reinterpret_cast<PVOID&>(m_realFunction), function));
            FAILFAST_IF_FAILED(DetourTransactionCommit());

            m_mockFunction = function;
        }

        void ClearDetour()
        {
            FAILFAST_IF_FAILED(DetourTransactionBegin());
            FAILFAST_IF_FAILED(DetourUpdateThread(GetCurrentThread()));
            FAILFAST_IF_FAILED(DetourDetach(&reinterpret_cast<PVOID&>(m_realFunction), m_mockFunction));
            FAILFAST_IF_FAILED(DetourTransactionCommit());
        }

    public:
        CreateMockDetourBase(LPCWSTR dllName, LPCSTR functionName)
        {
            m_LoadLibraryModule = ::LoadLibrary(dllName);
            ASSERT(m_LoadLibraryModule);

            m_realFunction = reinterpret_cast<T*>(::GetProcAddress(m_LoadLibraryModule, functionName));
            ASSERT(m_realFunction);
        }

        CreateMockDetourBase(T* realFunction) : m_realFunction(realFunction)
        {

        }

        ~CreateMockDetourBase()
        {
            ClearDetour();
            if (m_LoadLibraryModule != NULL)
            {
                ::FreeLibrary(m_LoadLibraryModule);
            }
        }

    };

} } } } }

