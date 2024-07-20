// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WeakReferenceSource.h"
#include "JoltClasses.h"
#include <ContentControl.h>
#include <EnumDefs.g.h>
#include <optional>

namespace DirectUI
{
    class WindowChrome;
}

class CBrush;
class CUIElement;

class CWindowChrome : public CContentControl
{
    // this is the same for all DPIs
    static constexpr const int topBorderVisibleHeight = 1;
    static constexpr const float defaultTitlebarHeight = 32.0f;


protected:
    CWindowChrome(_In_ CCoreServices * pCore)
        : CContentControl(pCore)
    {
        SetIsCustomType();
    }

public:
    
    DECLARE_CREATE(CWindowChrome);

    
    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
    
    _Check_return_ HRESULT Initialize(_In_ HWND parentWindow);
    
    ~CWindowChrome() override;
    
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CWindowChrome>::Index;
    }

    DirectUI::Visibility m_titleBarVisibility = DirectUI::Visibility::Collapsed;

    _Check_return_ HRESULT OnTitleBarSizeChanged();
    _Check_return_ HRESULT SetDragRegion(RECT rf);

    bool HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, _Out_ LRESULT* pResult);

    void UpdateBridgeWindowSizePosition();
    void UpdateContainerSize(WPARAM wParam, LPARAM lParam);
    
    ctl::ComPtr<DirectUI::WindowChrome> GetPeer();
    _Check_return_ HRESULT SetIsChromeActive(bool isActive);
    bool IsChromeActive() const  { return m_bIsActive; }

    bool IsTitlebarVisible() const;
    int GetTopBorderHeight() const noexcept;
    _Check_return_ HRESULT ConfigureWindowChrome();
    _Check_return_ HRESULT ApplyStyling();
    [[nodiscard]] LRESULT OnNcHitTest(const POINT ptMouse);
    
    // sometimes when you need to temporarily disable dragging like in case when Content Dialog is shown
    // these apis control whether a drag region gets created or not.
    bool CanDrag() const { return m_enabledDrag; }
    void UpdateCanDragStatus(bool enabled);

private:
    enum class CaptionButtonState {Normal, Hover, Pressed};

    [[nodiscard]] LRESULT OnCreate();
    [[nodiscard]] LRESULT OnPaint();
    
    
    // Repositions the Visual Studio in-app toolbar below the custom titlebar if necessary,
    // as the custom titlebar's glass window would otherwise intercept input to it and make it inoperable
    HRESULT RefreshToolbarOffset();
    
    bool m_bIsActive = false;
    HWND m_topLevelWindow = NULL;
    bool m_enabledDrag =  true;
    RECT m_dragRegionCached{};
    bool m_isDefaultCaptionButtonStyleSet = false; // do it only the first time window chrome is created
    
};
