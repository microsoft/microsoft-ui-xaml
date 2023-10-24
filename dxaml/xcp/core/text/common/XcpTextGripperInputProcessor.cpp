// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <XcpTextGripperInputProcessor.h>

#ifndef UINT
    typedef unsigned int UINT;
#endif

#ifndef UINT64
    typedef unsigned __int64 UINT64;
#endif

#include <SelectionBehavior.h>

//---------------------------------------------------------------------------
//
//  XCPHandleInputProcessor
//
//  Adjusts the input processor parameters from win32 to xaml.
//
//---------------------------------------------------------------------------
class XCPHandleInputProcessor: public IXcpHandleInputProcessor
{
public:
    XCPHandleInputProcessor(_In_ IHandleInputProcessor *pProcessor) : m_pProcessor(pProcessor) {}
    HRESULT SetHiMetricsPerPixel(float fScale) override;
    void SetGripperDisplayLocation(XPOINT ptAnchor, XPOINT ptGripperTopEdgeOffsetFromAnchor, XUINT32 cDiameter) override;
    HRESULT OnStartGripperDrag(XPOINT unFilteredPoint, XUINT32 cCurrentLineHeight) override;
    HRESULT OnGripperDrag(XPOINT unFilteredPoint, XUINT32 cCurrentLineHeight, _Out_ XPOINT *ptResult) override;
    HRESULT OnEndGripperDrag(void) override;
    ~XCPHandleInputProcessor() override;
private:
    IHandleInputProcessor *m_pProcessor;
};

XCPHandleInputProcessor::~XCPHandleInputProcessor() 
{
    delete m_pProcessor;
}

HRESULT XCPHandleInputProcessor::SetHiMetricsPerPixel(float fScale)
{
    return m_pProcessor->SetHimetricsPerPixel(fScale);
}

void XCPHandleInputProcessor::SetGripperDisplayLocation(XPOINT ptAnchorXaml, XPOINT ptGripperTopEdgeOffsetFromAnchor, XUINT32 cDiameter)
{
    POINT ptAnchorWin32;
    ptAnchorWin32.x = ptAnchorXaml.x;
    ptAnchorWin32.y = ptAnchorXaml.y;
    POINT ptGripperTopEdgeOffsetWin32;
    ptGripperTopEdgeOffsetWin32.x = ptGripperTopEdgeOffsetFromAnchor.x;
    ptGripperTopEdgeOffsetWin32.y = ptGripperTopEdgeOffsetFromAnchor.y;
    return m_pProcessor->SetGripperDisplayLocation(ptAnchorWin32, ptGripperTopEdgeOffsetWin32, static_cast<UINT>(cDiameter));
}

HRESULT XCPHandleInputProcessor::OnStartGripperDrag(XPOINT unFilteredPoint, XUINT32 cCurrentLineHeight)
{
    POINT ptRawWin32;
    ptRawWin32.x = unFilteredPoint.x;
    ptRawWin32.y = unFilteredPoint.y;
    UINT cCurrentLineHeightWin32 = cCurrentLineHeight;
    return m_pProcessor->OnStartGripperDrag(ptRawWin32, cCurrentLineHeightWin32);
}

HRESULT XCPHandleInputProcessor::OnGripperDrag(XPOINT unFilteredPoint, XUINT32 cCurrentLineHeight, _Out_ XPOINT *ptResult)
{
    POINT ptRawWin32;
    ptRawWin32.x = unFilteredPoint.x;
    ptRawWin32.y = unFilteredPoint.y;
    POINT ptResultWin32;

    HRESULT hr = m_pProcessor->OnGripperDrag(ptRawWin32, static_cast<UINT>(cCurrentLineHeight), &ptResultWin32);
    ptResult->x = ptResultWin32.x;
    ptResult->y = ptResultWin32.y;
    return hr;
}
    
HRESULT XCPHandleInputProcessor::OnEndGripperDrag(void)
{
    return m_pProcessor->OnEndGripperDrag();
}

IXcpHandleInputProcessor * IXcpHandleInputProcessor::s_CreateInstance()
{
    IXcpHandleInputProcessor *pWrappedProcessor = nullptr;
    IHandleInputProcessor *pProcessor = IHandleInputProcessor::s_CreateInstance();
    if (pProcessor)
    {
        pWrappedProcessor = new (std::nothrow) XCPHandleInputProcessor(pProcessor);
        if (pWrappedProcessor)
        {
            pProcessor = nullptr;
        }
    }
    delete pProcessor;
    return pWrappedProcessor;
}