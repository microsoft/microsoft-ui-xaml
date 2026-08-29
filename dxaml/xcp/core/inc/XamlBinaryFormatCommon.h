// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef _XAMLBINARYFORMATCOMMON_H_
#define _XAMLBINARYFORMATCOMMON_H_
//------------------------------------------------------------------------
//
//  Abstract:
//
//      Provides common types for use in binary XAML serialization.
//
//------------------------------------------------------------------------

struct LineInfoDeltaXamlNodeData
{
    LineInfoDeltaXamlNodeData()
        : m_uiLineNumberDelta(0)
        , m_uiLinePositionDelta(0)
    {
    }

    XINT16 m_uiLineNumberDelta;
    XINT16 m_uiLinePositionDelta;
};

struct LineInfoAbsoluteXamlNodeData
{
    LineInfoAbsoluteXamlNodeData()
        : m_uiLineNumber(0)
        , m_uiLinePosition(0)
    {
    }

    XUINT32 m_uiLineNumber;
    XUINT32 m_uiLinePosition;
};

#endif
