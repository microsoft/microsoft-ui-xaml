// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "CustomPages.h"
#include <XamlTailored.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;

namespace Tests {
    namespace Native {
        namespace External {
            namespace Framework {
                namespace ComponentConnector {

void SimulatedBinding::Connect(int connectionId, ::Platform::Object^ target)
{
    PageHelper^ helper = m_wrHelper.Resolve<PageHelper>();

    m_calls.Mark(connectionId);

    if (helper != nullptr)
    {
        helper->ValidateBindingConnect(this, connectionId, target);
    }
}

Microsoft::UI::Xaml::Markup::IComponentConnector^ SimulatedBinding::GetBindingConnector(int connectionId, ::Platform::Object^ target)
{
    Microsoft::UI::Xaml::Markup::IComponentConnector^ returnedConnector = nullptr;

    PageHelper^ helper = m_wrHelper.Resolve<PageHelper>();

    m_calls.Mark(connectionId);

    if (helper != nullptr)
    {
        returnedConnector = helper->GetBindingConnector(connectionId, target);
    }

    return returnedConnector;
}

void SimulatedBinding::SetHelper(PageHelper^ helper)
{
    m_wrHelper = helper;
}

unsigned SimulatedBinding::GetCallCount()
{
    return m_calls.m_count;
}

PageHelper::PageHelper()
    : m_binding0(nullptr)
    , m_binding1(nullptr)
    , m_binding2(nullptr)
    , m_nullConnector(false)
{}

void PageHelper::SetReturnNullConnector()
{
    m_nullConnector = true;
}

void PageHelper::SetPage(Microsoft::UI::Xaml::Controls::Page^ page)
{
    m_wrPage = page;
}

unsigned PageHelper::GetCallCount(unsigned level)
{
    switch (level)
    {
        case 0:
            return m_calls.m_count;

        case 1:
            return (m_binding0) ? m_binding0->GetCallCount() : 0;

        case 2:
            return (m_binding1) ? m_binding1->GetCallCount() : 0;

        case 3:
            return (m_binding2) ? m_binding2->GetCallCount() : 0;

        default:
            VERIFY_IS_TRUE(false);
    }

    return 0;
}

void PageHelper::ValidatePageConnect(int connectionId, ::Platform::Object^ target)
{
    m_calls.Mark(connectionId);
    ValidateConnectPrivate(nullptr, connectionId, target);
}

Microsoft::UI::Xaml::Markup::IComponentConnector^ PageHelper::GetBindingConnector(int connectionId, ::Platform::Object^ target)
{
    Microsoft::UI::Xaml::Markup::IComponentConnector^ returnedConnector = nullptr;

    if (m_nullConnector)
    {
        return nullptr;
    }

    if (connectionId == 0)
    {
        m_binding0 = ref new SimulatedBinding();
        m_binding0->SetHelper(this);
        returnedConnector = m_binding0;
    }
    else if (connectionId == 100)
    {
        m_binding1 = ref new SimulatedBinding();
        m_binding1->SetHelper(this);
        returnedConnector = m_binding1;
    }
    else if (connectionId == 200)
    {
        m_binding2 = ref new SimulatedBinding();
        m_binding2->SetHelper(this);
        returnedConnector = m_binding2;
    }

    return returnedConnector;
}

void PageHelper::ValidateBindingConnect(SimulatedBinding^ sender, int connectionId, ::Platform::Object^ target)
{
    ValidateConnectPrivate(sender, connectionId, target);
}

void PageHelper::ValidateConnectPrivate(SimulatedBinding^ sender, int connectionId, ::Platform::Object^ target)
{
    if (connectionId >= 0 &&
        connectionId < 100)
    {
        VERIFY_IS_TRUE(sender == nullptr || sender == m_binding0);
        ValidateConnect0(connectionId, target);
    }
    else if (connectionId >= 100 &&
             connectionId < 200)
    {
        VERIFY_IS_TRUE(sender == nullptr || sender == m_binding1);
        ValidateConnect1(connectionId, target);
    }
    else if (connectionId >= 200 &&
             connectionId < 300)
    {
        VERIFY_IS_TRUE(sender == nullptr || sender == m_binding2);
        ValidateConnect2(connectionId, target);
    }
    else
    {
        VERIFY_IS_TRUE(false);
    }
}
                        
void PageHelper::ValidateConnect0(int connectionId, ::Platform::Object^ target)
{
    switch (connectionId)
    {
        case 0: // Page
            VERIFY_IS_TRUE(m_wrPage.Resolve<Page>() == safe_cast<Page^>(target));
            break;

        case 1: // StackPanel - outer
            VERIFY_IS_TRUE(safe_cast<StackPanel^>(target)->Name == L"sp1");
            break;

        case 2: // Button
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnNotDeferred2");
            break;

        case 3: // Button - deferred
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnDeferred3");
            break;

        case 4: // ContentControl - outer
            VERIFY_IS_TRUE(safe_cast<ContentControl^>(target)->Name == L"ccouter");
            break;

        case 5: // Button - deferred
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnDeferred5");
            break;

        case 6: // Button
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnNotDeferred6");
            break;

        default:
            VERIFY_IS_TRUE(false);
            break;
    }
}

void PageHelper::ValidateConnect1(int connectionId, ::Platform::Object^ target)
{
    switch (connectionId)
    {
        case 100: // StackPanel - DT root
            VERIFY_IS_TRUE(safe_cast<StackPanel^>(target)->Name == L"sp100");
            break;

        case 101: // Button
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnNotDeferred101");
            break;

        case 102: // Button - deferred
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnDeferred102");
            break;

        case 103: // ContentControl - outer
            VERIFY_IS_TRUE(safe_cast<ContentControl^>(target)->Name == L"ccinner");
            break;

        case 104: // Button - deferred
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnDeferred104");
            break;

        case 105: // Button
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnNotDeferred105");
            break;

        default:
            VERIFY_IS_TRUE(false);
            break;
    }
}

void PageHelper::ValidateConnect2(int connectionId, ::Platform::Object^ target)
{
    switch (connectionId)
    {
        case 200: // Grid - DT root
            VERIFY_IS_TRUE(safe_cast<Grid^>(target)->Name == L"gr200");
            break;

        case 201: // Button
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnNotDeferred201");
            break;

        case 202: // Button - deferred
            VERIFY_IS_TRUE(safe_cast<Button^>(target)->Name == L"btnDeferred202");
            break;

        default:
            VERIFY_IS_TRUE(false);
            break;
    }
}

void PageHelper::Realize(unsigned stage)
{
    switch (stage)
    {
        case 0:
        {
            auto page = m_wrPage.Resolve<Page>();
            VERIFY_IS_NOT_NULL(page->FindName(L"btnDeferred3"));
            VERIFY_IS_NOT_NULL(page->FindName(L"btnDeferred5"));
            break;
        }

        case 1:
        {
            auto page = m_wrPage.Resolve<Page>();
            ContentControl^ ccouter = safe_cast<ContentControl^>(page->FindName(L"ccouter"));
            VERIFY_IS_NOT_NULL(ccouter);
            FrameworkElement^ contentTemplateRoot = safe_cast<FrameworkElement^>(ccouter->ContentTemplateRoot);
            VERIFY_IS_NOT_NULL(contentTemplateRoot);
            VERIFY_IS_NOT_NULL(contentTemplateRoot->FindName(L"btnDeferred102"));
            VERIFY_IS_NOT_NULL(contentTemplateRoot->FindName(L"btnDeferred104"));
            break;
        }

        case 2:
        {
            auto page = m_wrPage.Resolve<Page>();
            ContentControl^ ccouter = safe_cast<ContentControl^>(page->FindName(L"ccouter"));
            VERIFY_IS_NOT_NULL(ccouter);
            FrameworkElement^ contentTemplateRoot = safe_cast<FrameworkElement^>(ccouter->ContentTemplateRoot);
            VERIFY_IS_NOT_NULL(contentTemplateRoot);
            ContentControl^ ccinner = safe_cast<ContentControl^>(contentTemplateRoot->FindName(L"ccinner"));
            VERIFY_IS_NOT_NULL(ccinner);
            FrameworkElement^ innerContentTemplateRoot = safe_cast<FrameworkElement^>(ccinner->ContentTemplateRoot);
            VERIFY_IS_NOT_NULL(innerContentTemplateRoot);
            VERIFY_IS_NOT_NULL(innerContentTemplateRoot->FindName(L"btnDeferred202"));
            break;
        }
    }
}

PageWithICC::PageWithICC()
    : m_helper(ref new PageHelper)
{
    m_helper->SetPage(this);
}

void PageWithICC::Connect(int connectionId, ::Platform::Object^ target)
{
    m_helper->ValidatePageConnect(connectionId, target);
}

Microsoft::UI::Xaml::Markup::IComponentConnector^ PageWithICC::GetBindingConnector(int connectionId, ::Platform::Object^ target)
{
    return m_helper->GetBindingConnector(connectionId, target);
}

PageHelper^ PageWithICC::GetHelper()
{
    return m_helper;
}
}}}}}