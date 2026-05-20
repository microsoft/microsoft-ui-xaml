// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include "XamlLightTests.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>
#include <Collection.h>

using namespace Platform;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

TestBrush::TestBrush(TestLight^ light)
{
    m_light = light;
}

void TestBrush::OnConnected()
{
    const auto& wh = TestServices::WindowHelper;
    UIElement^ current = wh->WindowContent;
    UIElement^ parent = current;
    do
    {
        current = parent;
        parent = safe_cast<UIElement^>(VisualTreeHelper::GetParent(current));
    } while (parent != nullptr);

    auto lights = current->Lights;
    lights->Append(m_light);
}

void TestBrush::OnDisconnected()
{
}

TestLight::TestLight(SpotLight^ spotLight, Platform::String^ id)
{
    CompositionLight = spotLight;
    m_id = id;
    m_onConnectedElement = nullptr;
    m_onDisconnectedElement = nullptr;
}

Platform::String^ TestLight::GetId()
{
    return m_id;
}

void TestLight::OnConnected(UIElement^ newElement)
{
    m_onConnectedElement = newElement;
}

void TestLight::OnDisconnected(UIElement^ oldElement)
{
    m_onDisconnectedElement = oldElement;
}

void TestLight::VerifyElementsAndReset(
    Microsoft::UI::Xaml::UIElement^ connectedElement,
    Microsoft::UI::Xaml::UIElement^ disconnectedElement)
{
    VERIFY_ARE_EQUAL(connectedElement, m_onConnectedElement);
    VERIFY_ARE_EQUAL(disconnectedElement, m_onDisconnectedElement);

    m_onConnectedElement = nullptr;
    m_onDisconnectedElement = nullptr;
}

void TestLight::SetWUCLight(SpotLight^ wucLight)
{
    CompositionLight = wucLight;
}

void TestLight::SetNewId(Platform::String^ newId)
{
    m_id = newId;
}

Platform::String^ XamlLightTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\";
}

bool XamlLightTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool XamlLightTests::ClassCleanup()
{
    return true;
}

bool XamlLightTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool XamlLightTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void XamlLightTests::APITest()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    Canvas^ root;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        Canvas^ canvas = ref new Canvas();
        canvas->Width = 25;
        canvas->Height = 25;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        XamlLight::AddTargetElement(L"Hover", canvas);
        XamlLight::AddTargetElement(L"SomeOtherLight", canvas);

        Border^ border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        border->Child = canvas;
        XamlLight::AddTargetElement(L"Reveal", border);

        TestLight^ testLight = ref new TestLight(CreateSpotLight(), L"Dummy");

        auto lights = root->Lights;
        lights->Append(testLight);

        root->Children->Append(border);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
}

void XamlLightTests::BasicTest()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ lightTarget;
    TestLight^ testLight;
    TestLight^ testLight2;
    TestLight^ testLight3;
    unsigned int dontCare;

    SpotLight^ spotLight;

    LOG_OUTPUT(L"> Initial state - Dummy light targeting an element, Dummy2 light targeting nothing");
    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();

        lightTarget = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", lightTarget);

        Grid^ grid = CreateGrid();
        grid->Children->Append(lightTarget);

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight2 = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight3 = ref new TestLight(CreateSpotLight(), L"dUMMY");

        auto lights = root->Lights;
        lights->Append(testLight);
        lights->Append(testLight2);
        lights->Append(testLight3);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Dummy lights found their target, dUMMY light found nothing");

    LOG_OUTPUT(L"> Checking UIElement -> XamlLight map");
    auto lights = ref new Platform::Collections::Vector<XamlLight^>();
    RunOnUIThread([&]() { wh->GetLightsTargetingElement(lightTarget, lights); });
    VERIFY_ARE_EQUAL(2u, lights->Size);
    VERIFY_IS_TRUE(lights->IndexOf(testLight, &dontCare));
    VERIFY_IS_TRUE(lights->IndexOf(testLight2, &dontCare));

    LOG_OUTPUT(L"> Checking XamlLight -> UIElement lists");
    auto targets = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets3 = ref new Platform::Collections::Vector<UIElement^>();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
    });
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(lightTarget, targets->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(lightTarget, targets2->GetAt(0));
    VERIFY_ARE_EQUAL(0u, targets3->Size);

    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

void XamlLightTests::LightTargetEntersTree()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ c1;
    Canvas^ c2;
    Canvas^ c3;
    Grid^ grid;
    TestLight^ testLight;
    unsigned int dontCare;

    LOG_OUTPUT(L"> Initial state - A canvas with a targetId and a canvas without any targetId");
    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas();

        c3 = CreateCanvas();    // But won't be in the tree
        XamlLight::AddTargetElement(L"Dummy", c3);

        grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");

        auto lights = root->Lights;
        lights->Append(testLight);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights3 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    lights3->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetLightsTargetingElement(c3, lights3);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(0u, lights3->Size);
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> A new target enters the tree");
    RunOnUIThread([&]()
    {
        grid->Children->Append(c3);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    lights3->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetLightsTargetingElement(c3, lights3);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(1u, lights3->Size);
    VERIFY_ARE_EQUAL(testLight, lights3->GetAt(0));
    VERIFY_ARE_EQUAL(2u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c3, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"> An existing element gets a light target");
    RunOnUIThread([&]()
    {
        XamlLight::AddTargetElement(L"Dummy", c2);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    lights3->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetLightsTargetingElement(c3, lights3);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights3->Size);
    VERIFY_ARE_EQUAL(testLight, lights3->GetAt(0));
    VERIFY_ARE_EQUAL(3u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c2, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c3, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");
}

void XamlLightTests::LightTargetLeavesTree()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ c1;
    Canvas^ c2;
    Canvas^ c3;
    Grid^ grid;
    TestLight^ testLight;
    unsigned int dontCare;

    LOG_OUTPUT(L"> Initial state - 3 canvases with targetIds");
    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c2);

        c3 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c3);

        grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);
        grid->Children->Append(c3);

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");

        auto lights = root->Lights;
        lights->Append(testLight);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights3 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    lights3->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetLightsTargetingElement(c3, lights3);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights3->Size);
    VERIFY_ARE_EQUAL(testLight, lights3->GetAt(0));
    VERIFY_ARE_EQUAL(3u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c2, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c3, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> An existing element loses a light target");
    RunOnUIThread([&]()
    {
        XamlLight::RemoveTargetElement(L"Dummy", c2);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    lights3->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetLightsTargetingElement(c3, lights3);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(1u, lights3->Size);
    VERIFY_ARE_EQUAL(testLight, lights3->GetAt(0));
    VERIFY_ARE_EQUAL(2u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c3, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"> A target leaves the tree");
    RunOnUIThread([&]()
    {
        grid->Children->RemoveAt(0);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    lights3->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetLightsTargetingElement(c3, lights3);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(1u, lights3->Size);
    VERIFY_ARE_EQUAL(testLight, lights3->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c3, targets->GetAt(0));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");
}

void XamlLightTests::LightTargetChangesTargets()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ c1;
    Canvas^ c2;
    Canvas^ c3;
    Canvas^ c4;
    Canvas^ c5;
    Grid^ grid;
    TestLight^ testLight;
    TestLight^ testLight2;
    unsigned int dontCare;

    LOG_OUTPUT(L"> Initial state - 5 canvases with targetIds");
    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c2);
        XamlLight::AddTargetElement(L"dUMMY", c2);

        c3 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c3);

        c4 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c4);

        c5 = CreateCanvas();
        XamlLight::AddTargetElement(L"dUMMY", c5);

        grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);
        grid->Children->Append(c3);
        grid->Children->Append(c4);
        grid->Children->Append(c5);

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight2 = ref new TestLight(CreateSpotLight(), L"dUMMY");

        auto lights = root->Lights;
        lights->Append(testLight);
        lights->Append(testLight2);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights3 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights4 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights5 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    lights3->Clear();
    lights4->Clear();
    lights5->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetLightsTargetingElement(c3, lights3);
        wh->GetLightsTargetingElement(c4, lights4);
        wh->GetLightsTargetingElement(c5, lights5);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(2u, lights2->Size);
    VERIFY_IS_TRUE(lights2->IndexOf(testLight, &dontCare));
    VERIFY_IS_TRUE(lights2->IndexOf(testLight2, &dontCare));
    VERIFY_ARE_EQUAL(1u, lights3->Size);
    VERIFY_ARE_EQUAL(testLight, lights3->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights4->Size);
    VERIFY_ARE_EQUAL(testLight, lights4->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights5->Size);
    VERIFY_ARE_EQUAL(testLight2, lights5->GetAt(0));
    VERIFY_ARE_EQUAL(4u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c2, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c3, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c4, &dontCare));
    VERIFY_ARE_EQUAL(2u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    VERIFY_IS_TRUE(targets2->IndexOf(c5, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> Add a target, remove a target, add+remove a target, remove+add a target, remove+add the same target");
    RunOnUIThread([&]()
    {
        XamlLight::AddTargetElement(L"dUMMY", c1);

        XamlLight::RemoveTargetElement(L"Dummy", c2);

        XamlLight::AddTargetElement(L"dUMMY", c3);
        XamlLight::RemoveTargetElement(L"Dummy", c3);

        XamlLight::RemoveTargetElement(L"Dummy", c4);
        XamlLight::AddTargetElement(L"dUMMY", c4);

        XamlLight::RemoveTargetElement(L"dUMMY", c5);
        XamlLight::AddTargetElement(L"dUMMY", c5);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    lights3->Clear();
    lights4->Clear();
    lights5->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetLightsTargetingElement(c3, lights3);
        wh->GetLightsTargetingElement(c4, lights4);
        wh->GetLightsTargetingElement(c5, lights5);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(2u, lights1->Size);
    VERIFY_IS_TRUE(lights1->IndexOf(testLight, &dontCare));
    VERIFY_IS_TRUE(lights1->IndexOf(testLight2, &dontCare));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights3->Size);
    VERIFY_ARE_EQUAL(testLight2, lights3->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights4->Size);
    VERIFY_ARE_EQUAL(testLight2, lights4->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights5->Size);
    VERIFY_ARE_EQUAL(testLight2, lights5->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(5u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    VERIFY_IS_TRUE(targets2->IndexOf(c3, &dontCare));
    VERIFY_IS_TRUE(targets2->IndexOf(c4, &dontCare));
    VERIFY_IS_TRUE(targets2->IndexOf(c5, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void XamlLightTests::LightEntersTree()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ c1;
    Canvas^ c2;
    Grid^ grid;
    TestLight^ testLight;
    TestLight^ testLight2;
    Canvas^ root;

    LOG_OUTPUT(L"> Initial state - Two canvases with targetIds, but no lights.");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas();
        XamlLight::AddTargetElement(L"dUMMY", c2);

        grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);

        // These aren't in the tree
        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight2 = ref new TestLight(CreateSpotLight(), L"dUMMY");

        auto lights = root->Lights;

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(0u, targets->Size);
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    testLight->VerifyElementsAndReset(nullptr, nullptr);
    testLight2->VerifyElementsAndReset(nullptr, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> A new light enters an existing collection");
    RunOnUIThread([&]()
    {
        root->Lights->Append(testLight);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    testLight->VerifyElementsAndReset(root, nullptr);
    testLight2->VerifyElementsAndReset(nullptr, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"> Another new light enters the tree");
    RunOnUIThread([&]()
    {
        grid->Lights->Append(testLight2);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c2, targets2->GetAt(0));
    testLight->VerifyElementsAndReset(nullptr, nullptr);
    testLight2->VerifyElementsAndReset(grid, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");
}

void XamlLightTests::LightLeavesTree()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ c1;
    Canvas^ c2;
    Grid^ grid;
    StackPanel^ stackPanel;
    TestLight^ testLight;
    TestLight^ testLight2;
    TestLight^ testLight3;
    Canvas^ root;

    LOG_OUTPUT(L"> Initial state - Two canvases with targetIds, two lights.");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas();
        XamlLight::AddTargetElement(L"dUMMY", c2);

        grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);

        stackPanel = ref new StackPanel();

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        auto lights = root->Lights;
        lights->Append(testLight);

        testLight2 = ref new TestLight(CreateSpotLight(), L"dUMMY");
        auto lights2 = grid->Lights;
        lights2->Append(testLight2);

        testLight3 = ref new TestLight(CreateSpotLight(), L"dUMMY");    // target ID doesn't matter. There won't be anything under this light's subtree.
        auto lights3 = stackPanel->Lights;
        lights3->Append(testLight3);

        root->Children->Append(grid);
        root->Children->Append(stackPanel);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c2, targets2->GetAt(0));
    testLight->VerifyElementsAndReset(root, nullptr);
    testLight2->VerifyElementsAndReset(grid, nullptr);
    testLight3->VerifyElementsAndReset(stackPanel, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> A light leaves an existing collection");
    RunOnUIThread([&]()
    {
        root->Lights->RemoveAt(0);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(0u, targets->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c2, targets2->GetAt(0));
    testLight->VerifyElementsAndReset(nullptr, root);
    testLight2->VerifyElementsAndReset(nullptr, nullptr);
    testLight3->VerifyElementsAndReset(nullptr, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"> A light collection gets cleared");
    RunOnUIThread([&]()
    {
        grid->Lights->Clear();
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(0u, targets->Size);
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    testLight->VerifyElementsAndReset(nullptr, nullptr);
    testLight2->VerifyElementsAndReset(nullptr, grid);
    testLight3->VerifyElementsAndReset(nullptr, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    LOG_OUTPUT(L"> A subtree gets disconnected");
    RunOnUIThread([&]()
    {
        root->Children->RemoveAt(1);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking that its light got disconnected. This is important to break potential reference cycles where this light references the element that's keeping the light alive.");
    testLight3->VerifyElementsAndReset(nullptr, stackPanel);
}

void XamlLightTests::LightMovesInTree()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ c1;
    Canvas^ c2;
    Grid^ grid;
    Canvas^ root;
    TestLight^ testLight;
    TestLight^ testLight2;

    LOG_OUTPUT(L"> Initial state - Two canvases with targetIds, and two lights.");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas();
        XamlLight::AddTargetElement(L"dUMMY", c2);

        grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        auto lightCollection = root->Lights;
        lightCollection->Append(testLight);

        testLight2 = ref new TestLight(CreateSpotLight(), L"dUMMY");
        auto lightCollection2 = grid->Lights;
        lightCollection2->Append(testLight2);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c2, targets2->GetAt(0));
    testLight->VerifyElementsAndReset(root, nullptr);
    testLight2->VerifyElementsAndReset(grid, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> Remove a light from the collection on the root...");
    RunOnUIThread([&]()
    {
        root->Lights->RemoveAt(0);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(0u, targets->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c2, targets2->GetAt(0));
    testLight->VerifyElementsAndReset(nullptr, root);
    testLight2->VerifyElementsAndReset(nullptr, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"> ...and add it to a collection on a leaf.");
    RunOnUIThread([&]()
    {
        c1->Lights->Append(testLight);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c2, targets2->GetAt(0));
    testLight->VerifyElementsAndReset(c1, nullptr);
    testLight2->VerifyElementsAndReset(nullptr, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    LOG_OUTPUT(L"> Remove light from the middle of the tree...");
    RunOnUIThread([&]()
    {
        grid->Lights->Clear();
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    testLight->VerifyElementsAndReset(nullptr, nullptr);
    testLight2->VerifyElementsAndReset(nullptr, grid);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

    LOG_OUTPUT(L"> ...and add it to the root.");
    RunOnUIThread([&]()
    {
        root->Lights->Append(testLight2);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c2, targets2->GetAt(0));
    testLight->VerifyElementsAndReset(nullptr, nullptr);
    testLight2->VerifyElementsAndReset(root, nullptr);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"5");
}

void XamlLightTests::LightTargetsLTETarget()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    LOG_OUTPUT(L"> Initial state - No lights or light targets in the tree. LTE targets an element that needs a comp node. Don't crash.");

    Canvas^ root = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridView.xaml"));
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    GridView^ gridView;
    RunOnUIThread([&]() { gridView = safe_cast<GridView^>(root->FindName(L"gridView")); });

    // Force some frames between inserts into the GridView. If we insert items too fast, the AddRemoveThemeTransition gets skipped.
    // See GetSpeedOfChanges and ModernCollectionBasePanel::TransitionContextManager::IsCollectionMutatingFast.
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Add a rectangle that needs a comp node. Don't crash.");

        auto newChild = ref new xaml_shapes::Rectangle();
        newChild->Width = 50;
        newChild->Height = 50;
        newChild->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

        auto newItem = ref new xaml_controls::GridViewItem();
        newItem->Content = newChild;
        newItem->CompositeMode = ElementCompositeMode::SourceOver;  // The LTE target itself needs to have a comp node

        gridView->Items->InsertAt(0, newItem);
    });
    wh->WaitForIdle();

    // TODO: XamlLight: point a light at newItem. The LTE's container visual should be the one targeted.
}

void XamlLightTests::LightDisconnectedAfterTreeReset()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    Canvas^ c1;
    Canvas^ c2;
    Grid^ grid;
    TestLight^ testLight;
    TestLight^ testLight2;
    Canvas^ root;

    LOG_OUTPUT(L"> Initial state - Two canvases with targetIds, two lights.");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas();
        XamlLight::AddTargetElement(L"dUMMY", c2);

        grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        auto lights = root->Lights;
        lights->Append(testLight);

        testLight2 = ref new TestLight(CreateSpotLight(), L"dUMMY");
        auto lights2 = grid->Lights;
        lights2->Append(testLight2);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight2, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c2, targets2->GetAt(0));
    testLight->VerifyElementsAndReset(root, nullptr);
    testLight2->VerifyElementsAndReset(grid, nullptr);

    LOG_OUTPUT(L"> Reset the visual tree like we do during shutdown.");
    wh->ResetVisualTree();
    wh->WaitForTreeReset();
    testLight->VerifyElementsAndReset(nullptr, root);
    testLight2->VerifyElementsAndReset(nullptr, grid);
}

void XamlLightTests::CompositionLightChanges()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ c1;
    Canvas^ c2;
    TestLight^ testLight;
    TestLight^ testLight2;
    TestLight^ testLight3;

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets3 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Initial state - Two canvases with targetIds, and three lights. Only two lights are backed by a WUC light.");
    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight2 = ref new TestLight(nullptr, L"Dummy");
        testLight3 = ref new TestLight(CreateSpotLight(), L"dUMMY");

        auto lightCollection = root->Lights;
        lightCollection->Append(testLight);
        lightCollection->Append(testLight2);
        lightCollection->Append(testLight3);

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas();
        XamlLight::AddTargetElement(L"dUMMY", c2);

        Grid^ grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    targets3->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight3, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    VERIFY_ARE_EQUAL(1u, targets3->Size);
    VERIFY_ARE_EQUAL(c2, targets3->GetAt(0));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> Update CompositionLight - remove an existing WUC light, add a new WUC light, change a WUC light.");
    RunOnUIThread([&]()
    {
        testLight->SetWUCLight(nullptr);
        testLight2->SetWUCLight(CreateSpotLight());
        testLight3->SetWUCLight(CreateSpotLight());
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    targets2->Clear();
    targets3->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight2, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight3, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(0u, targets->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_ARE_EQUAL(c1, targets2->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets3->Size);
    VERIFY_ARE_EQUAL(c2, targets3->GetAt(0));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void XamlLightTests::ElementRegeneratesContent()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ c1;
    TestLight^ testLight;

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Initial state - Two canvases with targetIds, and a light.");
    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");

        auto lightCollection = root->Lights;
        lightCollection->Append(testLight);

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        Grid^ grid = CreateGrid();
        grid->Children->Append(c1);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> Update a targeted element - the new sprite visual must be targeted as well.");
    RunOnUIThread([&]()
    {
        c1->Width = 50;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, targets->Size);
    VERIFY_ARE_EQUAL(c1, targets->GetAt(0));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
}

void XamlLightTests::LightTargetsBrushBasic()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    SolidColorBrush^ brush1;
    SolidColorBrush^ brush2;
    SolidColorBrush^ brush3;
    Canvas^ c1;
    Border^ b2;
    TestLight^ testLight1;
    TestLight^ testLight2;
    unsigned int dontCare;

    LOG_OUTPUT(L"> Initial state");
    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();

        brush1 = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        brush2 = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        brush3 = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        XamlLight::AddTargetBrush(L"Dummy", brush3);

        c1 = CreateCanvas(brush1);

        b2 = CreateBorder();
        b2->Background = brush2;

        Grid^ grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(b2);

        testLight1 = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight2 = ref new TestLight(CreateSpotLight(), L"dUMMY");

        auto lights = root->Lights;
        lights->Append(testLight1);
        lights->Append(testLight2);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets1 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(b2, lights2);
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(0u, targets1->Size);
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> Add a light target to a background brush + add a targeted border brush to the tree");
    RunOnUIThread([&]()
    {
        XamlLight::AddTargetBrush(L"Dummy", brush1);
        b2->BorderBrush = brush3;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(b2, lights2);
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_IS_TRUE(lights1->IndexOf(testLight1, &dontCare));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_IS_TRUE(lights2->IndexOf(testLight1, &dontCare));
    VERIFY_ARE_EQUAL(2u, targets1->Size);
    VERIFY_IS_TRUE(targets1->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets1->IndexOf(b2, &dontCare));
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"> Add a light target to a background brush");
    RunOnUIThread([&]()
    {
        XamlLight::AddTargetBrush(L"dUMMY", brush2);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(b2, lights2);
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_IS_TRUE(lights1->IndexOf(testLight1, &dontCare));
    VERIFY_ARE_EQUAL(2u, lights2->Size);
    VERIFY_IS_TRUE(lights2->IndexOf(testLight1, &dontCare));
    VERIFY_IS_TRUE(lights2->IndexOf(testLight2, &dontCare));
    VERIFY_ARE_EQUAL(2u, targets1->Size);
    VERIFY_IS_TRUE(targets1->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets1->IndexOf(b2, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(b2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    LOG_OUTPUT(L"> Remove a light target from a border brush");
    RunOnUIThread([&]()
    {
        XamlLight::RemoveTargetBrush(L"Dummy", brush3);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(b2, lights2);
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_IS_TRUE(lights1->IndexOf(testLight1, &dontCare));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_IS_TRUE(lights2->IndexOf(testLight2, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets1->Size);
    VERIFY_IS_TRUE(targets1->IndexOf(c1, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(b2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

    LOG_OUTPUT(L"> Remove targeted brush from the tree");
    RunOnUIThread([&]()
    {
        c1->Background = nullptr;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(b2, lights2);
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_IS_TRUE(lights2->IndexOf(testLight2, &dontCare));
    VERIFY_ARE_EQUAL(0u, targets1->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(b2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"5");

    LOG_OUTPUT(L"> Set targeted brush elsewhere in the tree");
    RunOnUIThread([&]()
    {
        b2->BorderBrush = brush1;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(b2, lights2);
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(2u, lights2->Size);
    VERIFY_IS_TRUE(lights2->IndexOf(testLight1, &dontCare));
    VERIFY_IS_TRUE(lights2->IndexOf(testLight2, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets1->Size);
    VERIFY_IS_TRUE(targets1->IndexOf(b2, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(b2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"6");
}

void XamlLightTests::LightTargetsBrushShared()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    SolidColorBrush^ brush1;
    SolidColorBrush^ brush2;
    SolidColorBrush^ brush3;
    Canvas^ c1;
    Canvas^ c2;
    Canvas^ c3;
    TestLight^ testLight1;
    TestLight^ testLight2;
    Canvas^ root;
    unsigned int dontCare;
    Popup^ popup;

    LOG_OUTPUT(L"> Initial state");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        brush1 = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        XamlLight::AddTargetBrush(L"Dummy", brush1);

        brush2 = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        XamlLight::AddTargetBrush(L"dUMMY", brush2);

        brush3 = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

        c1 = CreateCanvas(brush2);
        c2 = CreateCanvas(brush3);
        c3 = CreateCanvas(brush3);

        Grid^ grid = CreateGrid();
        grid->Children->Append(CreateCanvas(brush1));
        grid->Children->Append(c1);
        grid->Children->Append(CreateCanvas(brush1));
        grid->Children->Append(c2);
        grid->Children->Append(CreateCanvas(brush1));
        grid->Children->Append(c3);

        popup = ref new Popup();
        popup->Child = CreateCanvas(brush1);
        popup->IsOpen = true;

        testLight1 = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight2 = ref new TestLight(CreateSpotLight(), L"dUMMY");

        auto lights = root->Lights;
        lights->Append(testLight1);
        lights->Append(testLight2);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto targets1 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(4u, targets1->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c1, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> Changing brushes");
    RunOnUIThread([&]()
    {
        c1->Background = brush3;
        c2->Background = brush2;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(4u, targets1->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"> Shared light leaves the tree");
    RunOnUIThread([&]()
    {
        root->Lights->RemoveAt(0);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, targets1->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    LOG_OUTPUT(L"> Shared light reenters the tree");
    RunOnUIThread([&]()
    {
        root->Lights->Append(testLight1);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(4u, targets1->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

    LOG_OUTPUT(L"> Shared brush becomes untargeted");
    RunOnUIThread([&]()
    {
        XamlLight::RemoveTargetBrush(L"Dummy", brush1);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(0u, targets1->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"5");

    LOG_OUTPUT(L"> Shared brush gets targeted again");
    RunOnUIThread([&]()
    {
        XamlLight::AddTargetBrush(L"Dummy", brush1);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(4u, targets1->Size);
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"6");

    LOG_OUTPUT(L"> Shared brush gets multitargeted");
    RunOnUIThread([&]()
    {
        XamlLight::AddTargetBrush(L"dUMMY", brush1);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
    });
    VERIFY_ARE_EQUAL(4u, targets1->Size);
    VERIFY_ARE_EQUAL(5u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"7");
}

void XamlLightTests::LightTargetsBrushShared_DifferentLights()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    SolidColorBrush^ brush;
    Canvas^ c1;
    Canvas^ c2;
    Canvas^ c3;
    TestLight^ testLight1;
    TestLight^ testLight2;
    TestLight^ testLight3;
    Canvas^ root;
    unsigned int dontCare;

    LOG_OUTPUT(L"> Initial state");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        brush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        XamlLight::AddTargetBrush(L"Dummy", brush);

        c1 = CreateCanvas(brush);
        c2 = CreateCanvas(brush);
        c3 = CreateCanvas(brush);

        Grid^ grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);
        grid->Children->Append(c3);

        testLight1 = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight2 = ref new TestLight(CreateSpotLight(), L"Dummy");
        testLight3 = ref new TestLight(CreateSpotLight(), L"Dummy");

        c1->Lights->Append(testLight1);
        c2->Lights->Append(testLight2);
        c3->Lights->Append(testLight3);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto targets1 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets3 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    targets3->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
    });
    VERIFY_ARE_EQUAL(1u, targets1->Size);
    VERIFY_IS_TRUE(targets1->IndexOf(c1, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(c2, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets3->Size);
    VERIFY_IS_TRUE(targets3->IndexOf(c3, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");
}

void XamlLightTests::BrushAndElementTargeting()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    SolidColorBrush^ borderBrush;
    SolidColorBrush^ backgroundBrush;
    Border^ b;
    TestLight^ testLight1;
    TestLight^ testLight2;
    TestLight^ testLight3;
    Canvas^ root;
    unsigned int dontCare;

    LOG_OUTPUT(L"> Initial state");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        borderBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        XamlLight::AddTargetBrush(L"light1", borderBrush);

        backgroundBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        XamlLight::AddTargetBrush(L"light2", backgroundBrush);

        b = CreateBorder();
        b->BorderBrush = borderBrush;
        b->Background = backgroundBrush;
        XamlLight::AddTargetElement(L"light3", b);

        Grid^ grid = CreateGrid();
        grid->Children->Append(b);

        testLight1 = ref new TestLight(CreateSpotLight(), L"light1");
        testLight2 = ref new TestLight(CreateSpotLight(), L"light2");
        testLight3 = ref new TestLight(CreateSpotLight(), L"light3");
        grid->Lights->Append(testLight1);
        grid->Lights->Append(testLight2);
        grid->Lights->Append(testLight3);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto targets1 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets3 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    targets3->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
    });
    VERIFY_ARE_EQUAL(1u, targets1->Size);
    VERIFY_IS_TRUE(targets1->IndexOf(b, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(b, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets3->Size);
    VERIFY_IS_TRUE(targets3->IndexOf(b, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"> Untargeting element explicitly. The brushes should still be targeted.");
    RunOnUIThread([&]()
    {
        XamlLight::RemoveTargetElement(L"light3", b);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    targets3->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
    });
    VERIFY_ARE_EQUAL(1u, targets1->Size);
    VERIFY_IS_TRUE(targets1->IndexOf(b, &dontCare));
    VERIFY_ARE_EQUAL(1u, targets2->Size);
    VERIFY_IS_TRUE(targets2->IndexOf(b, &dontCare));
    VERIFY_ARE_EQUAL(0u, targets3->Size);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"> Retargeting element explicitly and untargeting both brushes. The element should be targeted.");
    RunOnUIThread([&]()
    {
        XamlLight::AddTargetElement(L"light3", b);
        XamlLight::RemoveTargetBrush(L"light1", borderBrush);
        XamlLight::RemoveTargetBrush(L"light2", backgroundBrush);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    targets3->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
    });
    VERIFY_ARE_EQUAL(0u, targets1->Size);
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    VERIFY_ARE_EQUAL(1u, targets3->Size);
    VERIFY_IS_TRUE(targets3->IndexOf(b, &dontCare));
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

    LOG_OUTPUT(L"> Untarget everything.");
    RunOnUIThread([&]()
    {
        XamlLight::RemoveTargetElement(L"light3", b);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    targets1->Clear();
    targets2->Clear();
    targets3->Clear();
    RunOnUIThread([&]()
    {
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
    });
    VERIFY_ARE_EQUAL(0u, targets1->Size);
    VERIFY_ARE_EQUAL(0u, targets2->Size);
    VERIFY_ARE_EQUAL(0u, targets3->Size);
    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");
}

void XamlLightTests::ROEFailTest()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    Canvas^ root;
    Grid^ grid;
    Canvas^ c1;
    Canvas^ c2;
    Canvas^ c3;
    Canvas^ newCanvas;
    SpotLight^ wucLight1;
    SpotLight^ wucLight2;
    SpotLight^ wucLight3;
    SpotLight^ wucLight3Replacement;
    TestLight^ testLight1;
    TestLight^ testLight2;
    TestLight^ testLight3;

    LOG_OUTPUT(L"> Initial state - Dummy light targeting an element");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->CompositeMode = ElementCompositeMode::SourceOver;

        c1 = CreateCanvas(); XamlLight::AddTargetElement(L"light1", c1);
        c2 = CreateCanvas(); XamlLight::AddTargetElement(L"light2", c2);
        c3 = CreateCanvas(); XamlLight::AddTargetElement(L"light3", c3);

        newCanvas = CreateCanvas();
        XamlLight::AddTargetElement(L"light1", newCanvas);

        grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Children->Append(c2);
        grid->Children->Append(c3);

        wucLight1 = CreateSpotLight();
        wucLight2 = CreateSpotLight();
        wucLight3 = CreateSpotLight();
        wucLight3Replacement = CreateSpotLight();
        testLight1 = ref new TestLight(wucLight1, L"light1");
        testLight2 = ref new TestLight(wucLight2, L"light2");
        testLight3 = ref new TestLight(wucLight3, L"light3");

        auto lights = root->Lights;
        lights->Append(testLight1);
        lights->Append(testLight2);
        lights->Append(testLight3);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Close one of the WUCLight ");
    RunOnUIThread([&]()
    {
        delete wucLight1;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> A new target enters the tree and an element regenerates its content. Don't crash from the closed light.");
    RunOnUIThread([&]()
    {
        grid->Children->Append(newCanvas);

        LOG_OUTPUT(L"> A new target enters the tree, a light leaves the tree, and an element regenerates its content. Don't crash from the closed light.");
        root->Lights->RemoveAt(0);

        c1->Width = 10;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> A WUC light is closed. Then the containing XamlLight's WUC light is nulled out. Don't crash.");
    RunOnUIThread([&]()
    {
        delete wucLight2;
        testLight2->SetWUCLight(nullptr);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> A WUC light is closed. Then the containing XamlLight's WUC light is set to a new WUC light. Don't crash.");
    RunOnUIThread([&]()
    {
        delete wucLight3;
        testLight3->SetWUCLight(wucLight3Replacement);
    });
    wh->WaitForIdle();
}

void XamlLightTests::ElementZIndexChanges()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    unsigned int dontCare;
    int visualsTargeted, visualsTargeted2;

    Canvas^ c1;
    Canvas^ c2;
    SolidColorBrush^ brush;
    Canvas^ root;
    TestLight^ testLight;

    LOG_OUTPUT(L"> Initial state - Two canvases targeted by one light.");
    RunOnUIThread([&]()
    {
        brush = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        XamlLight::AddTargetBrush(L"Dummy", brush);

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas(brush);

        root = ref new Canvas();
        root->Children->Append(c1);
        root->Children->Append(c2);

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        auto lightCollection = root->Lights;
        lightCollection->Append(testLight);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    visualsTargeted = -1;
    visualsTargeted2 = -1;
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetCountOfVisualsTargeted(testLight, c1, &visualsTargeted);
        wh->GetCountOfVisualsTargeted(testLight, c2, &visualsTargeted2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(2u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c2, &dontCare));
    VERIFY_ARE_EQUAL(1, visualsTargeted);
    VERIFY_ARE_EQUAL(1, visualsTargeted2);
    testLight->VerifyElementsAndReset(root, nullptr);

    LOG_OUTPUT(L"> Changing Z-index of child elements...");
    RunOnUIThread([&]()
    {
        Canvas::SetZIndex(c1, 10);
        Canvas::SetZIndex(c2, 20);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    visualsTargeted = -1;
    visualsTargeted2 = -1;
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetCountOfVisualsTargeted(testLight, c1, &visualsTargeted);
        wh->GetCountOfVisualsTargeted(testLight, c2, &visualsTargeted2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(2u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c2, &dontCare));
    VERIFY_ARE_EQUAL(1, visualsTargeted);
    VERIFY_ARE_EQUAL(1, visualsTargeted2);
}

void XamlLightTests::ChangeLightIdTest()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    unsigned int dontCare;
    int visualsTargeted, visualsTargeted2;

    Canvas^ c1;
    Canvas^ c2;
    SolidColorBrush^ brush;
    Canvas^ root;
    TestLight^ testLight;

    LOG_OUTPUT(L"> Initial state - Two canvases targeted by one light.");
    RunOnUIThread([&]()
    {
        brush = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
        XamlLight::AddTargetBrush(L"Dummy", brush);

        c1 = CreateCanvas();
        XamlLight::AddTargetElement(L"Dummy", c1);

        c2 = CreateCanvas(brush);

        root = ref new Canvas();
        root->Children->Append(c1);
        root->Children->Append(c2);

        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        auto lightCollection = root->Lights;
        lightCollection->Append(testLight);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto lights2 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    visualsTargeted = -1;
    visualsTargeted2 = -1;
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetCountOfVisualsTargeted(testLight, c1, &visualsTargeted);
        wh->GetCountOfVisualsTargeted(testLight, c2, &visualsTargeted2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(2u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c2, &dontCare));
    VERIFY_ARE_EQUAL(1, visualsTargeted);
    VERIFY_ARE_EQUAL(1, visualsTargeted2);
    testLight->VerifyElementsAndReset(root, nullptr);

    LOG_OUTPUT(L"> Change the light's Id, and redraw the canvases.");
    RunOnUIThread([&]()
    {
        testLight->SetNewId(L"new name");
        c1->Width = 30;
        c2->Width = 30;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets. The canvases should still be targeted by the light. The new Id should not take effect until the CompositionLight changes.");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    visualsTargeted = -1;
    visualsTargeted2 = -1;
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetCountOfVisualsTargeted(testLight, c1, &visualsTargeted);
        wh->GetCountOfVisualsTargeted(testLight, c2, &visualsTargeted2);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(1u, lights2->Size);
    VERIFY_ARE_EQUAL(testLight, lights2->GetAt(0));
    VERIFY_ARE_EQUAL(2u, targets->Size);
    VERIFY_IS_TRUE(targets->IndexOf(c1, &dontCare));
    VERIFY_IS_TRUE(targets->IndexOf(c2, &dontCare));
    VERIFY_ARE_EQUAL(1, visualsTargeted);
    VERIFY_ARE_EQUAL(1, visualsTargeted2);
    testLight->VerifyElementsAndReset(nullptr, nullptr);    // We don't expect another OnConnected call

    LOG_OUTPUT(L"> Change the CompositionLight.");
    RunOnUIThread([&]()
    {
        testLight->SetWUCLight(CreateSpotLight());
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Checking targets. The canvases should no longer be targeted by the light.");
    lights1->Clear();
    lights2->Clear();
    targets->Clear();
    visualsTargeted = -1;
    visualsTargeted2 = -1;
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetLightsTargetingElement(c2, lights2);
        wh->GetElementsTargetedByLight(testLight, targets);
        wh->GetCountOfVisualsTargeted(testLight, c1, &visualsTargeted);
        wh->GetCountOfVisualsTargeted(testLight, c2, &visualsTargeted2);
    });
    VERIFY_ARE_EQUAL(0u, lights1->Size);
    VERIFY_ARE_EQUAL(0u, lights2->Size);
    VERIFY_ARE_EQUAL(0u, targets->Size);
    VERIFY_ARE_EQUAL(-1, visualsTargeted);
    VERIFY_ARE_EQUAL(-1, visualsTargeted2);
    testLight->VerifyElementsAndReset(nullptr, nullptr);    // We don't expect another OnConnected call
}

void XamlLightTests::RootIsGrid()
{
    // The light should target the popup, since it's been lifted to the CRootVisual
    RootIsGridCommon(2);
}

void XamlLightTests::RootIsGridCommon(unsigned int expectedLightTargetCount)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Grid^ root;
    Canvas^ c1;
    Popup^ popup;
    TestLight^ testLight;
    TestBrush^ testBrush;
    UIElement^ rootScrollViewer;

    LOG_OUTPUT(L"> Initial state");
    RunOnUIThread([&]()
    {
        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        testBrush = ref new TestBrush(testLight);

        root = ref new Grid();
        root->Background = testBrush;   // Will attach testLight in the brush's OnConnected

        c1 = CreateCanvas();
        root->Children->Append(c1);
        XamlLight::AddTargetElement(L"Dummy", c1);

        SolidColorBrush^ brush1 = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        XamlLight::AddTargetBrush(L"Dummy", brush1);

        popup = ref new Popup();
        popup->Child = CreateCanvas(brush1);
        popup->IsOpen = true;

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        rootScrollViewer = wh->WindowContent;
        UIElement^ parent = rootScrollViewer;
        do
        {
            rootScrollViewer = parent;
            parent = safe_cast<UIElement^>(VisualTreeHelper::GetParent(rootScrollViewer));
        } while (parent != nullptr);
    });

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    lights1->Clear();
    targets->Clear();
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetElementsTargetedByLight(testLight, targets);
    });
    VERIFY_ARE_EQUAL(1u, lights1->Size);
    VERIFY_ARE_EQUAL(testLight, lights1->GetAt(0));
    VERIFY_ARE_EQUAL(expectedLightTargetCount, targets->Size);  // Depends on whether it will target the popup
    testLight->VerifyElementsAndReset(rootScrollViewer, nullptr);

    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
}

void XamlLightTests::WindowedPopup_RemoveLightsFromPopupRoot()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Grid^ root;
    Canvas^ c1;

    Popup^ popup;
    Canvas^ c2;
    UIElement^ popupRoot;
    auto popupOpenedRegistration = CreateSafeEventRegistration(Popup, Opened);
    auto popupOpenedEvent = std::make_shared<Event>();

    Popup^ windowedPopup;

    TestLight^ testLight;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");
        root = ref new Grid();

        c1 = CreateCanvas();
        root->Children->Append(c1);

        wh->WindowContent = root;

        c2 = CreateCanvas(ref new SolidColorBrush(Microsoft::UI::Colors::Blue));

        popup = ref new Popup();
        popup->Child = c2;
        popupOpenedRegistration.Attach(popup, ref new wf::EventHandler<Object^>([&](Object^ sender, Object^ e)
        {
            LOG_OUTPUT(L"  > Finding PopupRoot.");
            popupRoot = safe_cast<UIElement^>(VisualTreeHelper::GetParent(c2));
            popupOpenedEvent->Set();
        }));
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    popupOpenedEvent->WaitForDefault();
    VERIFY_IS_NOT_NULL(popupRoot);
    LOG_OUTPUT(L"> PopupRoot found.");
    popupOpenedRegistration.Detach();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Put light on PopupRoot.");
        testLight = ref new TestLight(CreateSpotLight(), L"Dummy");
        popupRoot->Lights->Append(testLight);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Open windowed Popup.");

        Canvas^ c3 = CreateCanvas(ref new SolidColorBrush(Microsoft::UI::Colors::Green));

        windowedPopup = ref new Popup();
        windowedPopup->Child = c3;
        wh->Popup_SetWindowed(windowedPopup);
        windowedPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Lights");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing light from PopupRoot. Don't crash.");
        popupRoot->Lights->Clear();
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Didn't crash.");

    u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoLights");
}

void XamlLightTests::MuxLightsInIslands()
{
    MuxLightsCommon(true);
}

void XamlLightTests::MuxLightsInCoreWindow()
{
    MuxLightsCommon(false);
}

void XamlLightTests::MuxLightsCommon(bool isInIsland)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    SolidColorBrush^ brush1;
    Canvas^ c1;
    TestLight^ testLight1;
    TestLight^ testLight2;
    TestLight^ testLight3;
    TestLight^ testLight4;
    TestLight^ testLight5;
    TestLight^ testLight6;
    TestLight^ testLight7;

    RunOnUIThread([&]()
    {
        // In Xaml Islands mode we specifically disable lights with an id matching:
        //   "Microsoft.UI.Xaml.Media.RevealBorderLight_DarkTheme"
        //   "Microsoft.UI.Xaml.Media.RevealBorderLight_LightTheme"
        //   "Microsoft.UI.Xaml.Media.RevealHoverLight"
        //   "Microsoft.UI.Xaml.Media.XamlAmbientLight"
        // See CXamlLight::IsEnabledInXamlIsland() in product code.
        // We intentionally test using a mix of strings from this list and other strings not on this list.

        brush1 = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        XamlLight::AddTargetBrush(L"Microsoft.UI.Xaml.Media.RevealBorderLight_DarkTheme", brush1);
        XamlLight::AddTargetBrush(L"Microsoft.UI.Xaml.Media.RevealBorderLight_LightTheme", brush1);
        XamlLight::AddTargetBrush(L"Microsoft.UI.Xaml.Media.RevealBorderLight_AsdfTheme", brush1);
        XamlLight::AddTargetBrush(L"Microsoft.UI.Xaml.Media.RevealHoverLight", brush1);
        XamlLight::AddTargetBrush(L"Windows.UI.Xaml.Media.RevealHoverLight", brush1);
        XamlLight::AddTargetBrush(L"Microsoft.UI.Xaml.Media.XamlAmbientLight", brush1);
        XamlLight::AddTargetBrush(L"Microsoft.UI.Xaml.Media.XamlAmbientLigh", brush1);

        c1 = CreateCanvas(brush1);

        testLight1 = ref new TestLight(CreateSpotLight(), L"Microsoft.UI.Xaml.Media.RevealBorderLight_DarkTheme");
        testLight2 = ref new TestLight(CreateSpotLight(), L"Microsoft.UI.Xaml.Media.RevealBorderLight_LightTheme");
        testLight3 = ref new TestLight(CreateSpotLight(), L"Microsoft.UI.Xaml.Media.RevealBorderLight_AsdfTheme");
        testLight4 = ref new TestLight(CreateSpotLight(), L"Microsoft.UI.Xaml.Media.RevealHoverLight");
        testLight5 = ref new TestLight(CreateSpotLight(), L"Windows.UI.Xaml.Media.RevealHoverLight");
        testLight6 = ref new TestLight(CreateSpotLight(), L"Microsoft.UI.Xaml.Media.XamlAmbientLight");
        testLight7 = ref new TestLight(CreateSpotLight(), L"Microsoft.UI.Xaml.Media.XamlAmbientLigh");

        Grid^ grid = CreateGrid();
        grid->Children->Append(c1);
        grid->Lights->Append(testLight4);
        grid->Lights->Append(testLight5);

        Canvas^ root = ref new Canvas();
        auto lights = root->Lights;
        lights->Append(testLight1);
        lights->Append(testLight2);
        lights->Append(testLight3);
        lights->Append(testLight6);
        lights->Append(testLight7);

        root->Children->Append(grid);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto lights1 = ref new Platform::Collections::Vector<XamlLight^>();
    auto targets1 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets2 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets3 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets4 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets5 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets6 = ref new Platform::Collections::Vector<UIElement^>();
    auto targets7 = ref new Platform::Collections::Vector<UIElement^>();

    LOG_OUTPUT(L"> Checking targets");
    RunOnUIThread([&]()
    {
        wh->GetLightsTargetingElement(c1, lights1);
        wh->GetElementsTargetedByLight(testLight1, targets1);
        wh->GetElementsTargetedByLight(testLight2, targets2);
        wh->GetElementsTargetedByLight(testLight3, targets3);
        wh->GetElementsTargetedByLight(testLight4, targets4);
        wh->GetElementsTargetedByLight(testLight5, targets5);
        wh->GetElementsTargetedByLight(testLight6, targets6);
        wh->GetElementsTargetedByLight(testLight7, targets7);
    });

    if (isInIsland)
    {
        VERIFY_ARE_EQUAL(3u, lights1->Size);
        VERIFY_ARE_EQUAL(0u, targets1->Size);
        VERIFY_ARE_EQUAL(0u, targets2->Size);
        VERIFY_ARE_EQUAL(1u, targets3->Size);
        VERIFY_ARE_EQUAL(0u, targets4->Size);
        VERIFY_ARE_EQUAL(1u, targets5->Size);
        VERIFY_ARE_EQUAL(0u, targets6->Size);
        VERIFY_ARE_EQUAL(1u, targets7->Size);
        u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");
    }
    else
    {
        VERIFY_ARE_EQUAL(7u, lights1->Size);
        VERIFY_ARE_EQUAL(1u, targets1->Size);
        VERIFY_ARE_EQUAL(1u, targets2->Size);
        VERIFY_ARE_EQUAL(1u, targets3->Size);
        VERIFY_ARE_EQUAL(1u, targets4->Size);
        VERIFY_ARE_EQUAL(1u, targets5->Size);
        VERIFY_ARE_EQUAL(1u, targets6->Size);
        VERIFY_ARE_EQUAL(1u, targets7->Size);
    }
}

Canvas^ XamlLightTests::CreateCanvas(Brush^ brush)
{
    Canvas^ canvas = ref new Canvas();
    canvas->Width = 25;
    canvas->Height = 25;
    canvas->Background = brush;
    return canvas;
}

Canvas^ XamlLightTests::CreateCanvas()
{
    return CreateCanvas(ref new SolidColorBrush(Microsoft::UI::Colors::Red));
}

Grid^ XamlLightTests::CreateGrid()
{
    Grid^ grid = ref new Grid();
    grid->Width = 50;
    grid->Height = 50;
    grid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
    return grid;
}

Border^ XamlLightTests::CreateBorder()
{
    Border^ border = ref new Border();
    border->Width = 50;
    border->Height = 50;
    border->BorderThickness = ThicknessHelper::FromUniformLength(5);
    return border;
}

SpotLight^ XamlLightTests::CreateSpotLight()
{
    SpotLight^ spotLight;

    RunOnUIThread([&]()
    {
        auto canvas = ref new Canvas();
        auto compositionVisual = ElementCompositionPreview::GetElementVisual(canvas);
        auto compositionObject = safe_cast<Microsoft::UI::Composition::ICompositionObject^>(compositionVisual);
        auto compositor = compositionObject->Compositor;
        spotLight = compositor->CreateSpotLight();
    });

    return spotLight;
}

}}}}}}

