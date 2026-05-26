// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include <chrono>
#include <ratio>

class CTimeSpan : public CDependencyObject
{
protected:
    CTimeSpan(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CTimeSpan(XDOUBLE timeSpan) // !!! FOR UNIT TESTING ONLY !!!
        : CDependencyObject(nullptr)
        , m_rTimeSpan(timeSpan)
    {}
#endif

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override;

// WinSE BUG #421235: Media Element:: Slower updates on Media Element Position for live source
//
// Since playback time in Media Engine (Media Foundation) is presented as XDOUBLE, XAML
// media element's presentation using XFLOAT cuases a loss of precision and slow update
// when the playback time (counted from 1970) is very large. To fix this, m_rTimeSpan is
// changed from XFLOAT to XDOUBLE. This affects media position, duration, and markers.
//
// NOTE: The other parts of XAML, especially animation, is still designed for XFLOAT.
// Therefore, XAML elements other than media element should convert m_rTimeSpan into
// XFLOAT to match the XFLOAT local variables when CTimeSpan is consumed.

    XDOUBLE m_rTimeSpan = 0.0;

protected:
    _Check_return_ HRESULT InitFromString(
        _In_ const xstring_ptr_view& inString);
};
