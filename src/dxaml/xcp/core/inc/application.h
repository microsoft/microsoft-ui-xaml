// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define APPLICATION_BASE_URI L"ms-resource:///Files/"
#define APPLICATION_BASE_URI_LEN SZ_COUNT(APPLICATION_BASE_URI)

class CUIElement;

//------------------------------------------------------------------------
//
//  Class:  CApplication
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
class CApplication final : public CDependencyObject
{
    CApplication(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
        m_requiresThreadSafeAddRefRelease = true;
    }

    ~CApplication() override;

public:
    // Creation method
    DECLARE_CREATE(CApplication);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CApplication>::Index;
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult,
        _In_ bool fHandledEventsToo = false) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

    void CleanupDeviceRelatedResourcesRecursive(bool cleanupDComp) final;

    void FireStartupEvent();
    void FireExitEvent();
    void FireStartingEvent();
    void FireStartedEvent();
    void FireExitingEvent();
    void FireExitedEvent();

    static _Check_return_ HRESULT LoadComponent(
        _In_ CCoreServices     *pCore,
        _In_ CDependencyObject *pComponent,
        _In_ IPALUri *pUri
    );

    // Accesses the xaml::Application::FocusVisualKind property value.
    static DirectUI::FocusVisualKind GetFocusVisualKind();

    // Accesses the xaml::Application::ApplicationHighContrastAdjustment property value.
    static _Check_return_ HRESULT GetApplicationHighContrastAdjustment(
        _Out_ DirectUI::ApplicationHighContrastAdjustment* pApplicationHighContrastAdjustment);

    // Invoked when the xaml::Application::FocusVisualKind property changed.
    // Ensures the current focused element's focus rect gets re-rendered according to the new property value.
    void OnFocusVisualKindChanged();

    _Check_return_ HRESULT OnHighContrastAdjustmentChanged();

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Application needs to participate in managed tree to prevent
        // managed ResourceDictionary from being gc'd

        return PARTICIPATES_IN_MANAGED_TREE;
    }

    static _Check_return_ HRESULT CreateBaseUri(
        _In_ XUINT32 cUriString,
        _In_reads_(cUriString) const WCHAR *pUriString,
        _Outptr_ IPALUri** ppUri);

public:
    CResourceDictionary*    m_pResources = nullptr;  // Application.Resources
    CXcpList<REQUEST>*      m_pEventList = nullptr;
    CUIElement*             m_pRootVisual = nullptr;
    CScrollContentControl*  m_pRootScrollViewer = nullptr;
    CContentPresenter*      m_pRootContentPresenter = nullptr;
    bool                    m_fRootVisualSet = false;
    bool                    m_bApplicationStartupCompleted = false;
};

#include "eventargs.h"

class CStartupEventArgs final : public CEventArgs
{};