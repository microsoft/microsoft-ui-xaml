// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "MultiParentShareableDependencyObject.h"
#include "Matrix.h"

#pragma once

class CTransform;
struct D2DRenderParams;
struct D2DPrecomputeParams;
class HitTestPolygon;
struct IPALAcceleratedGeometry;
struct IPALAcceleratedGraphicsFactory;
struct IPALAcceleratedRenderTarget;
struct IPALAcceleratedBrush;
class DCompTreeHost;

struct AcceleratedBrushParams
{
public:
    CMILMatrix m_transform;
    XRECTF m_clipRect;
    IPALAcceleratedGeometry *m_pClipGeometry;

    AcceleratedBrushParams();
    ~AcceleratedBrushParams();

    void Reset();

    _Check_return_ HRESULT SetBrushTransformAndClip(
        _In_ IPALAcceleratedGraphicsFactory *pD2DFactory,
        _In_ CMILMatrix *pBrushTransform,
        _In_opt_ XRECTF *prcBrushClip
        );

    _Check_return_ HRESULT PushBrushClip(
        bool canPushAxisAlignedClip,
        _In_ const CMILMatrix *pWorldTransform,
        _In_ IPALAcceleratedRenderTarget *pD2DRenderTarget,
        _In_opt_ const XRECTF_RB *pContentBounds,
        _Inout_ bool *pPushedBrushClipLayer,
        _Inout_ bool *pPushedAxisAlignedBrushClip
        );

private:
    void ResetBrushClip();
};


//------------------------------------------------------------------------
//
//  Class:  CBrush
//
//  Synopsis:
//      Base brush class. All other brushes are derived from this. This class
// can't be created directly.
//
//------------------------------------------------------------------------

class CBrush : public CMultiParentShareableDependencyObject
{
protected:
    CBrush(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {}

    ~CBrush() override;

    // CNoParentShareableDependencyObject overrides

    CBrush( _In_ const CBrush& original, _Out_ HRESULT& hr );

public:
    // Creation function

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBrush>::Index;
    }

// CBrush methods

    virtual bool HasClipRect()
    {
        return false;
    }

    virtual void GetNaturalBounds(
        _Out_ XRECTF *pNaturalBounds )
    {
        pNaturalBounds->X = 0;
        pNaturalBounds->Y = 0;
        pNaturalBounds->Width = 1;
        pNaturalBounds->Height = 1;
    }

    virtual void GetActualBounds(
        _Out_ XRECTF *pActualBounds )
    {
        GetNaturalBounds(pActualBounds);
    }

    virtual _Check_return_ HRESULT ComputeDeviceToSource(
        _In_ const CMILMatrix  *pWorldTransform,
        _In_ const XRECTF      *pRenderBounds,
        _Out_      CMILMatrix  *pDeviceToSource
        )
    {
        (pWorldTransform); // Ignore the parameter.
        (pRenderBounds);   // Ignore the parameter.
        (pDeviceToSource); // Ignore the parameter.
        return E_UNEXPECTED;
    }

    _Check_return_ HRESULT GetValue(
        _In_ const CDependencyProperty *pdp,
        _Out_ CValue *pValue) final;

    virtual _Check_return_ HRESULT HitTestBrushClipInLocalSpace(
        _In_ const XRECTF *pGeometryRenderBounds,
        _In_ const XPOINTF& target,
        _Out_ bool* pIsHit
        );

    virtual _Check_return_ HRESULT HitTestBrushClipInLocalSpace(
        _In_ const XRECTF *pGeometryRenderBounds,
        _In_ const HitTestPolygon& target,
        _Out_ bool* pIsHit
        );


    // Public Facade Methods
    virtual _Check_return_ HRESULT StartAnimation(_In_ WUComp::ICompositionAnimationBase* animation);
    virtual _Check_return_ HRESULT StopAnimation(_In_ WUComp::ICompositionAnimationBase* animation);

    void AddLightTargetId(_In_ const xstring_ptr& lightId);
    void RemoveLightTargetId(_In_ const xstring_ptr& lightId);

    bool IsLightTarget();
    bool IsTargetedByLight(_In_ CXamlLight* light);

    static void SetLightTargetDirty(_In_ CDependencyObject* target, DirtyFlags flags);

private:
    DCompTreeHost* GetDCompTreeHost();

public:
    xref_ptr<CTransform> GetTransform() const;
    xref_ptr<CTransform> GetRelativeTransform() const;
    FLOAT GetOpacity() const;

// CBrush fields

    XFLOAT m_eOpacity = 1.0f;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    virtual _Check_return_ HRESULT D2DEnsureDeviceIndependentResources(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const CMILMatrix *pMyAccumulatedTransform,
        _In_ const XRECTF_RB *pBrushBounds,
        _Inout_ AcceleratedBrushParams *pPALBrushParams
        );

    virtual _Check_return_ HRESULT UpdateAcceleratedBrush(
        _In_ const D2DRenderParams &renderParams
        );

    _Ret_maybenull_ IPALAcceleratedBrush* GetAcceleratedBrush();

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
public:
    virtual _Check_return_ HRESULT GetPrintBrush(
        _In_ const D2DRenderParams &printParams,
        _Outptr_ IPALAcceleratedBrush **ppBrush
        );

protected:
    // TODO: Text printing creates this D2D brush, but it's never cleaned up until this CBrush is deleted.
    IPALAcceleratedBrush* m_pPALBrush = nullptr;
};

#include "SolidColorBrush.h"
