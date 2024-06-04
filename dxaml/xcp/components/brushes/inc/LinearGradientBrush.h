// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Gradient.h>
#include <LinearGradientBrushMap.h>
#include <FacadeAnimationHelper.h>
#include <FacadeStorage.h>

class CLinearGradientBrush final : public CGradientBrush
{
private:
    CLinearGradientBrush(_In_ CCoreServices *pCore)
        : CGradientBrush(pCore)
    {}

    ~CLinearGradientBrush() override;

protected:
// CNoParentShareableDependencyObject overrides

    CLinearGradientBrush( _In_ const CLinearGradientBrush& original, _Out_ HRESULT& hr );
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

public:
// Creation method

    DECLARE_CREATE(CLinearGradientBrush);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLinearGradientBrush>::Index;
    }

// CNoParentShareableDependencyObject overrides

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CLinearGradientBrush);

// CLinearGradientBrush fields

    XPOINTF m_ptStart           = {};
    XPOINTF m_ptEnd             = { 1.0f, 1.0f };
    XUINT32 m_requestedTexels   = 0;

    bool GetRequestedTexelCount(_Out_ XUINT32 *pTexelCount)
    {
        *pTexelCount = m_requestedTexels;
        return (m_requestedTexels != 0);
    }


//
// Cache based storage for rendering system.
    IPALSurface *GetRenderingCache()
    {
        return m_pRenderingCache;
    }

    void SetRenderingCache(_In_ IPALSurface *pRenderingCache)
    {
        ReplaceInterface(m_pRenderingCache, pRenderingCache);
    }

    void ReleaseRenderingCache()
    {
        ReleaseInterface(m_pRenderingCache);
    }

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    void ReleaseDCompResources() final;
    void SetDCompResourceDirty() final;

    // Returns a WUC brush that implements this Xaml LinearGradientBrush. The cached m_wucBrush is reused whenever possible,
    // but there are some properties that require us to make a new WUC brush for each consumer of this LinearGradientBrush.
    // If there's a brushTransform (for RichTextBlock scenarios), we'll also make a new WUC brush and not cache it.
    wrl::ComPtr<WUComp::ICompositionBrush> GetWUCBrush(
        _In_ const XRECTF& brushBounds,
        _In_opt_ const CMILMatrix* brushTransform,
        _In_ WUComp::ICompositor4* compositor);

private:
    // Whether we can use the same WUC linear gradient brush managed by this LinearGradientBrush. If false, then we'll create
    // separate WUC brushes for each element that uses this LinearGradientBrush.
    bool CanUseSingleWUCBrush() const;

    void ApplyWUCBrushProperties(
        _In_ WUComp::ICompositor4* compositor,
        _In_ WUComp::ICompositionBrush* brush,
        _In_ const XRECTF& brushBounds,
        _In_ const CMILMatrix* brushTransform);

    _Check_return_ HRESULT SetExpressionAnimation(_In_ WUComp::ICompositor4* compositor4, WUComp::ICompositionGradientBrush* brush, _In_ LPCWSTR propertyName, _In_opt_ LPCWSTR expressionString, _In_opt_ const CMILMatrix* brushTransform = nullptr);

    _Check_return_ HRESULT EnsurePrimaryWUCBrush(_In_ WUComp::ICompositor* compositor);
    _Check_return_ HRESULT EnsurePrimaryWUCBrush(_In_ WUComp::ICompositor4* compositor);
private:
    IPALSurface *m_pRenderingCache = nullptr;

    wrl::ComPtr<WUComp::ICompositionBrush> m_wucBrush;  // For when one WUC gradient brush can be used for all consumers or as a primary brush for facade animations
    LinearGradientBrushMap m_wucBrushMap;               // For when one WUC gradient brush per consumer is needed

    bool m_isWUCBrushDirty : 1;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT D2DEnsureDeviceIndependentResources(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const CMILMatrix *pMyAccumulatedTransform,
        _In_ const XRECTF_RB *pBrushBounds,
        _Inout_ AcceleratedBrushParams *pPALBrushParams
        ) override;

    _Check_return_ HRESULT UpdateAcceleratedBrush(
        _In_ const D2DRenderParams &renderParams
        ) override;

private:
    _Check_return_ HRESULT CreateAcceleratedBrush(
        _In_ const D2DRenderParams &renderParams,
        _Outptr_ IPALAcceleratedBrush **ppBrush
        );

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT GetPrintBrush(
        _In_ const D2DRenderParams &printParams,
        _Outptr_ IPALAcceleratedBrush **ppBrush
        ) override;

//-----------------------------------------------------------------------------
// PC walk overrides
//-----------------------------------------------------------------------------
    void NWPropagateDirtyFlag(DirtyFlags flag) override;

//-----------------------------------------------------------------------------
// Facade properties
//-----------------------------------------------------------------------------
public:
    wfn::Vector2 GetTranslation(bool preferAnimatingValue = false) const;
    void SetTranslation(const wfn::Vector2& translation);

    DOUBLE GetRotation(bool preferAnimatingValue = false) const;
    void SetRotation(DOUBLE rotation);

    wfn::Vector2 GetScale(bool preferAnimatingValue = false) const;
    void SetScale(const wfn::Vector2& scale);

    wfn::Matrix3x2 GetTransformMatrix(bool preferAnimatingValue = false) const;
    void SetTransformMatrix(const wfn::Matrix3x2& transformMatrix);

    wfn::Vector2 GetCenterPoint(bool preferAnimatingValue = false) const;
    void SetCenterPoint(const wfn::Vector2& centerPoint);

    wfn::Vector2 GetAnimatedTranslation() const;
    void SetAnimatedTranslation(const wfn::Vector2& translation);

    DOUBLE GetAnimatedRotation() const;
    void SetAnimatedRotation(DOUBLE rotation);

    wfn::Vector2 GetAnimatedScale() const;
    void SetAnimatedScale(const wfn::Vector2& scale);

    wfn::Matrix3x2 GetAnimatedTransformMatrix() const;
    void SetAnimatedTransformMatrix(const wfn::Matrix3x2& transformMatrix);

    wfn::Vector2 GetAnimatedCenterPoint() const;
    void SetAnimatedCenterPoint(const wfn::Vector2& centerPoint);

//-------------------------------------------------------------------------------
// Facade Methods
//-------------------------------------------------------------------------------
public:
    _Check_return_ HRESULT StartAnimation(_In_ WUComp::ICompositionAnimationBase* animation) override;
    _Check_return_ HRESULT StopAnimation(_In_ WUComp::ICompositionAnimationBase* animation) override;

    // Facade Animation Helper Callback methods.
    void GetFacadeEntries(_Out_ const FacadeMatcherEntry** entries, _Out_ size_t * count);
    void PopulateBackingCompositionObjectWithFacade(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID);
    _Check_return_ HRESULT PullFacadePropertyValueFromCompositionObject(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeId);
    void FacadeAnimationComplete(KnownPropertyIndex animatedProperty);
    void AllFacadeAnimationsComplete();
    void CreateBackingCompositionObjectForFacade(_In_ WUComp::ICompositor* compositor, _Out_ WUComp::ICompositionObject** backingCO, _Outptr_result_maybenull_ IFacadePropertyListener** listener);

};
