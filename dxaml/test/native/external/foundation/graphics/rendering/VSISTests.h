// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <Microsoft.UI.Xaml.media.dxinterop.h>
#include <DXGI1_2.h>
#include <Dxgi1_3.h>
#include <D3D11.h>
#include <D2d1_1.h>

using namespace Microsoft::WRL;
using namespace Microsoft::UI::Xaml::Media::Imaging;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

// VSIS : Virtual Surface Image Source
class VSISTests : public WEX::TestClass<VSISTests>
{
public:
    BEGIN_TEST_CLASS(VSISTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\Microsoft.UI.Xaml.media.dxinterop.idl")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(Basics1)
        TEST_METHOD_PROPERTY(L"Description", L"Render a VSIS using ISurfaceImageSourceNative")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Advanced1)
        TEST_METHOD_PROPERTY(L"Description", L"Render a VSIS in a ScrollViewer using ISurfaceImageSourceNative")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MultiVSISBounds)
        TEST_METHOD_PROPERTY(L"Description", L"Validate bounds of VSIS in shared ancestor scenario")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NegativeCases)
        TEST_METHOD_PROPERTY(L"Description", L"Validate various expected failure conditions")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RegenerateVisual)
        TEST_METHOD_PROPERTY(L"Description", L"Tests that a VSIS doesn't regenerate its SpriteVisual unless the surface changes.")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // MockDComp isn't injected on OneCore, so we can't count the number of sprite visuals cleaned up
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Visual count mismatch
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;

    unsigned int VerifySpriteVisualsCleanedUp(MockDComp::IMockDCompDevice2^ mockDevice2, unsigned int expected);
};

} } } } } }

