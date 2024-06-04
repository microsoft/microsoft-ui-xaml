// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CMediaPlayerPresenter;

class CMediaPlayerElement final : public CControl
{
public:
    DECLARE_CREATE(CMediaPlayerElement);

    CMediaPlayerElement(_In_ CCoreServices *pCore)
        : CControl(pCore),
        m_fAutoPlay(false),
        m_isFullWindow(false),
        m_areTransportControlsEnabled(false),
        m_Stretch(DirectUI::Stretch::Uniform),
        m_pPosterSource(nullptr),
        m_bTemplateApplied(false)
    {
        SetIsCustomType();
    }

    ~CMediaPlayerElement() override
    {
        ReleaseInterface(m_pPosterSource);
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CMediaPlayerElement>::Index;
    }

    // CUIElement overrides
    static void NWSetMediaSourceDirty(
        _In_ CDependencyObject *pTarget,
        DirtyFlags flags
        ) {}

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    //
    // The storage for public DependencyProperties
    //
    CImageSource *m_pPosterSource;
    DirectUI::Stretch m_Stretch;
    bool m_fAutoPlay;
    bool m_isFullWindow;
    bool m_areTransportControlsEnabled;

    bool HasTemplateChild() override;

    void MarkTemplateApplied()
    {
        m_bTemplateApplied = true;
    }

    void SetPresenter(_In_opt_ CMediaPlayerPresenter* pPresenter);

protected:

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) final;

    _Check_return_ HRESULT MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF& desiredSize) final;

private:
    bool m_bTemplateApplied;
    xref::weakref_ptr<CMediaPlayerPresenter> m_wpPresenter;
};
