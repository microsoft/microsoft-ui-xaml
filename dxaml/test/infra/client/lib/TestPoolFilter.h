// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {

class TestPoolFilter
{
protected:
   TestPoolFilter() = default;

public:
    virtual HRESULT Initialize()
    {
        return S_OK;
    }

    virtual const wchar_t* GetName() const = 0;
    virtual bool IsEnabled()
    {
        return true;
    }
    virtual bool IsDirty() = 0;
};

} }

#define REGISTER_TESTPOOLFILTER(FilterType)\
    {\
        static FilterType filter;\
        LOG_OUTPUT(L"Registering TestPoolFilter: %s", filter.GetName());\
        testPoolFilters.push_back(&filter);\
        if (testPoolFilters.size()>NumberOfTestPoolFilters)\
        {\
            WEX::Logging::Log::Error(L"Run out of space for test pool filters, please update NumberOfTestPoolFilters in Utilities.h");\
        }\
    }

