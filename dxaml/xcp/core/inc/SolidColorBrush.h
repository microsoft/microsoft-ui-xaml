// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "brush.h"
#include "BrushParams.h"

#include <microsoft.ui.composition.h>

struct D2DRenderParams;

class CSolidColorBrush : public CBrush
{
public:
#if defined(__XAML_UNITTESTS__)
    CSolidColorBrush()  // !!! FOR UNIT TESTING ONLY !!!
        : CSolidColorBrush(nullptr)
    {}
#endif

    ~CSolidColorBrush() override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSolidColorBrush>::Index;
    }

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CSolidColorBrush);

    _Check_return_ HRESULT UpdateAcceleratedBrush(
        _In_ const D2DRenderParams &renderParams
        ) final;

    _Check_return_ HRESULT GetPrintBrush(
        _In_ const D2DRenderParams &printParams,
        _Outptr_ IPALAcceleratedBrush **ppBrush
        ) final;

    void SetIsColorAnimationDirty(bool value);

    WUComp::ICompositionBrush* GetWUCBrush(_In_ WUComp::ICompositor* compositor);

    void ReleaseDCompResources() final;

    void SetDCompResourceDirty() override;

    static void NWSetRenderDirty(
        _In_ CDependencyObject* pTarget,
        DirtyFlags flags
        );

    void EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context) override;

protected:
    CSolidColorBrush(_In_ CCoreServices *pCore)
        : CBrush(pCore)
        , m_isWUCColorAnimationDirty(true)
        , m_isWUCBrushDirty(true)
    {}

    _Check_return_ HRESULT FromStringOrColor(
        _In_ CREATEPARAMETERS *pCreate);

    CSolidColorBrush(_In_ const CSolidColorBrush& original, _Out_ HRESULT& hr);

private:
    _Check_return_ HRESULT CreateAcceleratedBrush(
        _In_ const D2DRenderParams &renderParams,
        _Outptr_ IPALAcceleratedBrush **ppBrush
        );

public:
    XUINT32 m_rgb = 0;  // Actually argb

    wrl::ComPtr<WUComp::ICompositionBrush> m_wucBrush;

    // Until we make the switch completely to WUC brushes, keep separate dirty flags so that the legacy and WUC
    // code paths don't stomp over each other.
    bool m_isWUCColorAnimationDirty : 1;
    bool m_isWUCBrushDirty          : 1;
};

class CTransparentUnoptimizedBrush final : public CSolidColorBrush
{
    CTransparentUnoptimizedBrush(_In_ CCoreServices *pCore)
        : CSolidColorBrush(pCore)
    {}

    ~CTransparentUnoptimizedBrush() override {};

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        )
    {
        HRESULT hr = S_OK;

        CTransparentUnoptimizedBrush* _this = new CTransparentUnoptimizedBrush(pCreate->m_pCore);
        IFC(ValidateAndInit(_this, ppObject));

        _this = NULL;

    Cleanup:
        delete _this;
        RRETURN(hr);
    }
};

