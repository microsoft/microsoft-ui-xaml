// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContentControl.h"

enum RootScrollViewerSetting
{
    RootScrollViewerSetting_Null                        = 0x00,
    RootScrollViewerSetting_ApplyTemplate               = 0x01,
    RootScrollViewerSetting_ProcessWindowSizeChanged    = 0x02,
};

class CScrollContentControl : public CContentControl
{
protected:
    CScrollContentControl(_In_ CCoreServices *pCore);
    ~CScrollContentControl() override;

public:
    DECLARE_CREATE(CScrollContentControl);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CScrollContentControl>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    bool HasRootScrollViewerApplyTemplate() const { return (m_rootScrollViewerSettings & RootScrollViewerSetting_ApplyTemplate) == RootScrollViewerSetting_ApplyTemplate; }
    bool HasRootScrollViewerProcessWindowSizeChanged() const { return (m_rootScrollViewerSettings & RootScrollViewerSetting_ProcessWindowSizeChanged) == RootScrollViewerSetting_ProcessWindowSizeChanged; }
    void SetRootScrollViewerSettingApplyTemplate(bool bValue) { bValue ? m_rootScrollViewerSettings |= RootScrollViewerSetting_ApplyTemplate : m_rootScrollViewerSettings &= (~RootScrollViewerSetting_ApplyTemplate); }
    void SetRootScrollViewerSettingWindowSizeChanged(bool bValue) { bValue ? m_rootScrollViewerSettings |= RootScrollViewerSetting_ProcessWindowSizeChanged : m_rootScrollViewerSettings &= (~RootScrollViewerSetting_ProcessWindowSizeChanged); }

    XFLOAT GetRootScrollViewerOriginalHeight() { return m_rootScrollViewerOriginalHeight; }
    void SetRootScrollViewerOriginalHeight(XFLOAT rootScrollViewerOriginalHeight) { m_rootScrollViewerOriginalHeight = rootScrollViewerOriginalHeight; }
    XUINT8 GetRootScrollViewerSettings() { return m_rootScrollViewerSettings; }
    void SetRootScrollViewer(bool bRootScrollViewer) { m_bRootScrollViewer = bRootScrollViewer; }

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;

private:
    XFLOAT m_rootScrollViewerOriginalHeight; // only root ScrollViewer use this flag

    // Only used by the root ScrollViewer control. Combination of these enum values
    // RootScrollViewer_ApplyTemplate = 0x01
    // RootScrollViewer_ProcessWindowSizeChanged = 0x02
    // RootScrollViewer_RequestedInputPaneShowing = 0x04
    XUINT8 m_rootScrollViewerSettings;

    bool m_bRootScrollViewer;

};

