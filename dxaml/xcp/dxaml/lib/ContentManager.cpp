// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContentManager.h"
#include "ScrollViewer.g.h"
#include "Window.g.h"
#include "RootScrollViewer.g.h"
#include "ScrollContentPresenter.g.h"
#include "Border.g.h"
#include "SolidColorBrush.g.h"
#include <RuntimeEnabledFeatures.h>

using namespace DirectUI;

ContentManager::ContentManager(_In_ DependencyObject* pOwner, bool isUwpWindowContent)
    : m_owner(pOwner)
    , m_isUwpWindowContent(isUwpWindowContent)
{
    m_tokRootScrollViewerSizeChanged.value = 0;
}

ContentManager::~ContentManager()
{
    if (m_RootScrollViewer)
    {
        if (m_tokRootScrollViewerSizeChanged.value)
        {
            ScrollViewer* pScrollViewer = static_cast<ScrollViewer*>(m_RootScrollViewer.Get());
            IGNOREHR(pScrollViewer->remove_SizeChanged(m_tokRootScrollViewerSizeChanged));
        }

        m_RootScrollViewer.Clear();
    }
    if (m_RootSVContentPresenter)
    {
        m_RootSVContentPresenter.Clear();
    }
}

xaml_controls::IScrollViewer* ContentManager::GetRootScrollViewer()
{
    return m_RootScrollViewer.Get();
}

xaml_controls::IScrollContentPresenter* ContentManager::GetRootSVContentPresenter()
{
    return m_RootSVContentPresenter.Get();
}

xaml::IUIElement* ContentManager::GetContent()
{
    return m_Content.Get();
}

_Check_return_ HRESULT ContentManager::SetContent(_In_opt_ xaml::IUIElement* pContent)
{
    ctl::ComPtr<xaml::IUIElement> spContent(pContent);

    if (m_Content.Get() == spContent.Get())
    {
        // nothing to do
        return S_OK;
    }

    IFC_RETURN(ClearRootScrollViewer());

    ctl::ComPtr<xaml_controls::ICanvas> spContentAsCanvas;
    if (spContent.Get())
    {
        IGNOREHR(ctl::do_query_interface(spContentAsCanvas, spContent.Get()));
    }

    // We can't attach root ScrollViewer if the content is Canvas since we have an issue with Drts(graphics and SUnit).
    // Create the root ScrollViewer if the content isn't Canvas.
    if (pContent && !spContentAsCanvas)
    {
        IFC_RETURN(CreateRootScrollViewer(spContent.Get()));
    }

    CUIElement* pContentNativePeerNoRef = nullptr;
    if (spContent.Get())
    {
        ctl::ComPtr<DirectUI::UIElement> uiElement;
        IFC_RETURN(spContent.As(&uiElement));
        pContentNativePeerNoRef = static_cast<CUIElement*>(uiElement->GetHandle());
    }

    DXamlCore* pCore = DXamlCore::GetCurrent();
    if (m_isUwpWindowContent)
    {
        IFC_RETURN((static_cast<CCoreServices*>(pCore->GetHandle()))->SetRootVisualResetEventSignaledStatus(FALSE));
        IFC_RETURN(CoreImports::Application_SetVisualRoot(pCore->GetHandle(), pContentNativePeerNoRef));
    }

    m_owner->RemovePtrValue(m_Content);
    m_Content.Clear();

    if (spContent.Get())
    {
        m_owner->SetPtrValue(m_Content, spContent.Get());
    }

    IFC_RETURN(pCore->NotifyFirstFramePending());

    return S_OK;
}


_Check_return_ HRESULT ContentManager::ClearRootScrollViewer()
{
    HRESULT hr = S_OK;

    if (!m_RootScrollViewer.Get())
    {
        // nothing to do
        goto Cleanup;
    }

    if (m_tokRootScrollViewerSizeChanged.value)
    {
        // remove our SizeChanged event handler from the current RSV
        ScrollViewer* pScrollViewer = static_cast<ScrollViewer*>(m_RootScrollViewer.Get());
        IFC(pScrollViewer->remove_SizeChanged(m_tokRootScrollViewerSizeChanged));
        m_tokRootScrollViewerSizeChanged.value = 0;
    }

    if (m_isUwpWindowContent)
    {
        // clear core's reference to the current RSV
        IFC(CoreImports::Application_SetRootScrollViewer(DXamlCore::GetCurrent()->GetHandle(), NULL /*pRootScrollViewer*/, NULL/*pRootScrollContentPresenter*/));
    }

    m_owner->RemovePtrValue(m_RootScrollViewer);
    m_RootScrollViewer.Clear();

    if (m_RootSVContentPresenter)
    {
        m_owner->RemovePtrValue(m_RootSVContentPresenter);
        m_RootSVContentPresenter.Clear();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ContentManager::CreateRootScrollViewer(_In_ xaml::IUIElement* pContent)
{
    HRESULT hr = S_OK;
    wf::Rect windowBounds;
    ctl::ComPtr<RootScrollViewer> spRootScrollViewer;
    ctl::ComPtr<ScrollContentPresenter> spRootSVContentPresenter;
    ctl::ComPtr<Border> spRootBorder;
    ctl::ComPtr<xaml::IFrameworkElement> spContentAsFE;
    ctl::ComPtr<xaml::ISizeChangedEventHandler> spRootScrollViewerSizeChangedHandler;
    ctl::ComPtr<SolidColorBrush> spTransparentBrush;
    wu::Color transparentColor;
    CUIElement* pRootScrollViewerNativePeerNoRef = NULL;

    IFCEXPECT_ASSERT(!m_RootScrollViewer.Get());
    IFCEXPECT_ASSERT(!m_RootSVContentPresenter.Get());

    IFC(DXamlCore::GetCurrent()->GetContentBoundsForElement(m_owner->GetHandle(), &windowBounds));

    // Create a new ScrollViewer and set it as root
    IFC(ctl::make(&spRootScrollViewer));

    pRootScrollViewerNativePeerNoRef = static_cast<CUIElement*>(spRootScrollViewer->GetHandle());

    // Create the ScrollContentPresenter for the root ScrollViewer.
    IFC(ctl::make(&spRootSVContentPresenter));

    // Create the root border to ensure the content position under ScrollViewer.
    // Without the border, the content can be compressed by changing the ScrollViewer height.
    IFC(ctl::make(&spRootBorder));

    // Set appropriate properties of the RSV
    IFC(spRootScrollViewer->put_ZoomMode((xaml_controls::ZoomMode_Disabled)));
    IFC(spRootScrollViewer->put_VerticalScrollMode((xaml_controls::ScrollMode_Disabled)));
    IFC(spRootScrollViewer->put_HorizontalScrollMode((xaml_controls::ScrollMode_Disabled)));
    IFC(spRootScrollViewer->put_VerticalScrollBarVisibility(xaml_controls::ScrollBarVisibility_Hidden));
    IFC(spRootScrollViewer->put_HorizontalScrollBarVisibility(xaml_controls::ScrollBarVisibility_Hidden));
    IFC(spRootScrollViewer->put_IsTabStop(TRUE));

    if (SUCCEEDED(ctl::do_query_interface(spContentAsFE, pContent)))
    {
        // Specify the root ScrollViewer and Border width/height with Windows size.
        IFC(spRootScrollViewer->put_Height(windowBounds.Height));
        IFC(spRootScrollViewer->put_Width(windowBounds.Width));
        IFC(spRootScrollViewer->put_HorizontalContentAlignment(xaml::HorizontalAlignment_Left));
        IFC(spRootScrollViewer->put_VerticalContentAlignment(xaml::VerticalAlignment_Top));

        auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        if (!runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DoNotSetRootScrollViewerBackground))
        {
            // Create the transparent brush to set it on the Border as the background property.
            IFC(ctl::make(&spTransparentBrush));
            transparentColor.A = 0;
            transparentColor.R = 255;
            transparentColor.G = 255;
            transparentColor.B = 255;
            IFC(spTransparentBrush->put_Color(transparentColor));
            IFC(spRootBorder->put_Background(spTransparentBrush.Get()));
        }

        IFC(spRootBorder->put_Height(windowBounds.Height));
        IFC(spRootBorder->put_Width(windowBounds.Width));
    }

    m_owner->SetPtrValue(m_RootScrollViewer, spRootScrollViewer);
    m_owner->SetPtrValue(m_RootSVContentPresenter, spRootSVContentPresenter);

    // Set the ScrollContentPresenter manually into the root ScrollViewer to avoid
    // the applying a template that cause the startup time delaying.
    spRootScrollViewer->SetRootScrollContentPresenter(spRootSVContentPresenter.Get());

    // Set the content of root ScrollViewer
    IFC(spRootScrollViewer->put_Content(ctl::as_iinspectable(spRootBorder.Get())));

    // Attach the RootVisual size changed event handler
    spRootScrollViewerSizeChangedHandler.Attach(
        new StaticMemberEventHandler<
            xaml::ISizeChangedEventHandler,
            IInspectable,
            xaml::ISizeChangedEventArgs>(&ContentManager::OnRootScrollViewerSizeChanged));
    IFC(spRootScrollViewer->add_SizeChanged(spRootScrollViewerSizeChangedHandler.Get(), &m_tokRootScrollViewerSizeChanged));

    // Root SV will apply the template to bind the content with page content when the root SV is entered in the tree.
    // This is for firing the loaded event on the right time stead of delaying with Measure happening.
    IFC(CoreImports::ScrollContentControl_SetRootScrollViewerSetting(static_cast<CScrollContentControl*>(pRootScrollViewerNativePeerNoRef), 0x01 /* RootScrollViewerSetting_ApplyTemplate */, TRUE /* fApplyTemplate */));
    IFC(CoreImports::ScrollContentControl_SetRootScrollViewerOriginalHeight(static_cast<CScrollContentControl*>(pRootScrollViewerNativePeerNoRef), windowBounds.Height));

    if (m_isUwpWindowContent)
    {
        // The m_owner can be a XAML Window or a XamlIsland.  The content for the window is special, in that it triggers
        // the initialization of the application.

        // Set the root ScrollViewer
        IFC(CoreImports::Application_SetRootScrollViewer(
            DXamlCore::GetCurrent()->GetHandle(),
            static_cast<CScrollContentControl*>(pRootScrollViewerNativePeerNoRef),
            spRootSVContentPresenter ? static_cast<CContentPresenter*>(spRootSVContentPresenter->GetHandle()) : NULL));
    }

Cleanup:
    if (spRootScrollViewer)
    {
        // UnpegNoRef is necessary because we did a CreateInstance and didn't add the ScrollViewer to the tree.
        spRootScrollViewer->UnpegNoRef();
    }
    if (spRootSVContentPresenter)
    {
        // UnpegNoRef is necessary because we did a CreateInstance and didn't add the ScrollContentPresenter to the tree.
        spRootSVContentPresenter->UnpegNoRef();
    }

    RRETURN(hr);
}

_Check_return_ HRESULT ContentManager::OnWindowSizeChanged()
{
    HRESULT hr = S_OK;
    auto pCore = DXamlCore::GetCurrent();
    if (!pCore)
    {
        // The XamlCore is gone.
        // This can happen at shutdown when multiple CoreWindows are active in a single process.
        return S_OK;
    }

    wf::Rect windowBounds = {};
    ctl::ComPtr<xaml::IFrameworkElement> spRootScrollViewerAsFE;
    ctl::ComPtr<xaml_controls::IContentControl> spRootScrollViewerAsCC;
    ctl::ComPtr<xaml::IUIElement> spRootScrollViewerAsUE;

    // Note that when calling get_Bounds in the scope of a window size changed event,
    // the returned size is the current (new) size.
    IFC(pCore->GetContentBoundsForElement(m_owner->GetHandle(), &windowBounds));

    spRootScrollViewerAsFE = m_RootScrollViewer.AsOrNull<xaml::IFrameworkElement>();
    if (spRootScrollViewerAsFE)
    {
        IFC(spRootScrollViewerAsFE->put_Height(windowBounds.Height));
        IFC(spRootScrollViewerAsFE->put_Width(windowBounds.Width));
    }

    spRootScrollViewerAsCC = m_RootScrollViewer.AsOrNull<xaml_controls::IContentControl>();
    if (spRootScrollViewerAsCC)
    {
        ctl::ComPtr<IInspectable> spContent;
        ctl::ComPtr<xaml_controls::IBorder> spRootBorder;
        ctl::ComPtr<xaml::IFrameworkElement> spRootBorderAsFE;

        IFC(spRootScrollViewerAsCC->get_Content(&spContent));
        spRootBorder = spContent.AsOrNull<xaml_controls::IBorder>();
        if (spRootBorder)
        {
            spRootBorderAsFE = spRootBorder.AsOrNull<xaml::IFrameworkElement>();
            if (spRootBorderAsFE)
            {
                IFC(spRootBorderAsFE->put_Height(windowBounds.Height));
                IFC(spRootBorderAsFE->put_Width(windowBounds.Width));
            }
        }
    }

    spRootScrollViewerAsUE = m_RootScrollViewer.AsOrNull<xaml::IUIElement>();
    if (spRootScrollViewerAsUE)
    {
        CUIElement* pRootScrollViewerNativePeerNoRef = NULL;
        pRootScrollViewerNativePeerNoRef = static_cast<CUIElement*>(spRootScrollViewerAsUE.Cast<DirectUI::UIElement>()->GetHandle());
        IFC(CoreImports::ScrollContentControl_SetRootScrollViewerSetting(
            static_cast<CScrollContentControl*>(pRootScrollViewerNativePeerNoRef),
            0x02 /* RootScrollViewerSetting_ProcessWindowSizeChanged */,
            TRUE /* window size changed */));
        IFC(CoreImports::ScrollContentControl_SetRootScrollViewerOriginalHeight(static_cast<CScrollContentControl*>(pRootScrollViewerNativePeerNoRef), windowBounds.Height));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ContentManager::OnRootScrollViewerSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spRootScrollViewerAsUE;

    if (SUCCEEDED(ctl::do_query_interface(spRootScrollViewerAsUE, pSender)))
    {
        bool bIsProcessWindowSizeChanged = false;

        CUIElement* pRootScrollViewerNativePeerNoRef = static_cast<CUIElement*>(spRootScrollViewerAsUE.Cast<DirectUI::UIElement>()->GetHandle());

        IFC(CoreImports::ScrollContentControl_GetRootScrollViewerSetting(
                static_cast<CScrollContentControl*>(pRootScrollViewerNativePeerNoRef),
                0x02 /* RootScrollViewerSetting_ProcessWindowSizeChanged */,
                bIsProcessWindowSizeChanged));

        if (bIsProcessWindowSizeChanged)
        {
            IFC(CoreImports::ScrollContentControl_SetRootScrollViewerSetting(
                    static_cast<CScrollContentControl*>(pRootScrollViewerNativePeerNoRef),
                    0x02 /* RootScrollViewerSetting_ProcessWindowSizeChanged */,
                    FALSE /* window size changed */));
        }
    }

Cleanup:
    RRETURN(hr);
}
