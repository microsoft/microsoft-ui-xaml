// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "brush.h"
#include <DOCollection.h>
#include <RenderTypes.h>
#include <fwd/windows.ui.composition.h>

class CGradientStop final : public CDependencyObject
{
private:
    CGradientStop(_In_ CCoreServices *pCore);

public:
// Creation method

    DECLARE_CREATE(CGradientStop);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGradientStop>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT Clone( _Out_ CGradientStop **ppClone );

    void AddWUCStop(_In_ WUComp::ICompositor4* compositor, _In_ wfc::IVector<WUComp::CompositionColorGradientStop*> * stops);

// CGradientStop fields

    XcpGradientStop m_stop;
    XUINT32 m_rgb = 0; // Color kept in sRGB space, m_stop has the scRGB color
    XUINT32 m_uLastUpdate;
};

//------------------------------------------------------------------------
//
//  Class:  CGradientStopCollection
//
//  Synopsis:
//      A collection of gradient stops.
//
//------------------------------------------------------------------------

class CGradientStopCollection final : public CDOCollection
{
private:
    CGradientStopCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
// Creation methods

    DECLARE_CREATE(CGradientStopCollection);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGradientStopCollection>::Index;
    }

    _Check_return_ HRESULT Clone( _Out_ CGradientStopCollection **ppClone );

// CDOCollection overrides
    _Check_return_ HRESULT Append(CValue& value, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, CValue& value) override;
    _Check_return_ void *RemoveAt(_In_ XUINT32 nIndex) override;
    _Check_return_ HRESULT Clear() override;

// CGradientStopCollection methods

    static _Check_return_ HRESULT CreateFromArray(
        _Outptr_result_buffer_((nCount + 2)) XcpGradientStop **ppGradientStopsTarget,
        _In_ XUINT32 nCount,
        _In_ CGradientStopCollection *pGradientStopsSource
        );

    void AddWUCStops(_In_ WUComp::ICompositor4* compositor, _In_ WUComp::ICompositionGradientBrush* wucBrush);
};

//------------------------------------------------------------------------
//
//  Class:  CGradientBrush
//
//  Synopsis:
//      The base class for linear and radial gradient brushes.  This class can
//  not be directly created.
//
//------------------------------------------------------------------------

class CGradientBrush : public CBrush
{
protected:
    CGradientBrush(_In_ CCoreServices *pCore)
        : CBrush(pCore)
    {}

    ~CGradientBrush() override;

// CNoParentShareableDependencyObject overrides
    CGradientBrush( _In_ const CGradientBrush& original, _Out_ HRESULT& hr );

public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGradientBrush>::Index;
    }

    DECLARE_CREATE_RETURN(CGradientBrush, E_UNEXPECTED);

    bool IsOpaque() const;

public:
    XcpColorInterpolationMode m_nInterpolate = XcpColorInterpolationModeSRgbLinearInterpolation;
    XcpGradientWrapMode m_nSpread = XcpGradientWrapModeExtend;
    XcpMappingMode m_nMapping = XcpRelative;
    CGradientStopCollection* m_pStops = nullptr;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT GetAcceleratedGradientStops(
        _Outptr_ XcpGradientStop **ppGradientStops,
        _Out_ XUINT32 *pGradientStopCount,
        _Out_ InterpolationMode *pInterpolationMode,
        _Out_ GradientWrapMode *pGradientWrapMode
        );
};
