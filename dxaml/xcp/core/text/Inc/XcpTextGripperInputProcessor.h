// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
//
//    Copyright (C) Microsoft.  All rights reserved.
//

class IXcpHandleInputProcessor
{
public:
    virtual HRESULT SetHiMetricsPerPixel(float fScale) = 0;
    virtual void SetGripperDisplayLocation(XPOINT ptAnchor, XPOINT ptGripperTopEdgeOffsetFromAnchor, XUINT32 cDiameter) = 0;
    virtual HRESULT OnStartGripperDrag(XPOINT unFilteredPoint, XUINT32 cCurrentLineHeight) = 0;
    virtual HRESULT OnGripperDrag(XPOINT unFilteredPoint, XUINT32 cCurrentLineHeight, _Out_ XPOINT *ptResult) = 0;
    virtual HRESULT OnEndGripperDrag(void) = 0;
    virtual ~IXcpHandleInputProcessor() {}
    static IXcpHandleInputProcessor *s_CreateInstance();
};


