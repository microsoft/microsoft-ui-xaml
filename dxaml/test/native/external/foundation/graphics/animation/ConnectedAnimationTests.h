// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <XamlMetadataProviderOverrider.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

// Enum identifying copy options options for the source node
enum SourceCopyOption
{
    DontCopy,
    CopyWithCompNode,
    CopyWithoutCompNode
};

class ConnectedAnimationTests : public WEX::TestClass<ConnectedAnimationTests>
{
public:
    BEGIN_TEST_CLASS(ConnectedAnimationTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
     END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(BasicAPI)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(IsolateElementVisuals)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(EndToEnd)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(EndToEndCopySource)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(EndToEndCopySourceWithCompNode)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DestinationVisualUpdate)
        TEST_METHOD_PROPERTY(L"Description", L"End-to-end with update of destination element and visual")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CustomAnimations)
        TEST_METHOD_PROPERTY(L"Description", L"End-to-end test of customized animations")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SnapshotTransforms)
        TEST_METHOD_PROPERTY(L"Description", L"Verify transforms for visual snapshot.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RetainElement)
        TEST_METHOD_PROPERTY(L"Description", L"Testing retaining logic.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEvent)
        TEST_METHOD_PROPERTY(L"Description", L"Testing ConnectedAnimation.Completed event.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CoordinatedAnimationErrors)
    TEST_METHOD_PROPERTY(L"Description", L"Verify Coordinated Animation error conditions")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CoordinatedAnimations)
        TEST_METHOD_PROPERTY(L"Description", L"Testing Coordinated animations")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CoordinatedAnimationsNoScale)
        TEST_METHOD_PROPERTY(L"Description", L"Testing Coordinated animations with scale animations disabled")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HighDPI)
        TEST_METHOD_PROPERTY(L"Description", L"Testing High DPI scenarios")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VeryHighDPI)
        TEST_METHOD_PROPERTY(L"Description", L"Testing High DPI (Talkman) scenarios, GetGlobalBounds already contains scale factor.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DefaultAnimationParameters)
        TEST_METHOD_PROPERTY(L"Description", L"Testing ConnectedAnimationService.DefaultDuration/DefaultEasingFunction properties.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ContentWithOffset)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when destination element's content has offset.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TryStartWithNotLiveElement)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when destination element is not alive")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PrepareWithNotLiveElement)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when source element is not alive")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PrepareElementThenSetOpacity0)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when source element prepared and user set opacity to 0 immediately")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ElementsClipped)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when elements are clipped, the animation is respecting the clippings.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Timeout)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when source element is prepared and if it is not started within 2 secs it will be timed out.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // MOCK10_REMOVAL
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ElementsCompletelyClipped)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when elements are clipped, the animation is respecting the clippings.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RestartBeforeComplete)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when multiple animations are restarted before they are completed.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DisabledFromSettings)
        TEST_METHOD_PROPERTY(L"Description", L"Testing when animations are disabled in Settings, no connected animation should run.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ContentThemeTransition)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that destination element is always hiding during when animation is running.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SuppressScaleAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that scale animatin can be suppressed.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ConnectedPlusImplicit)
        TEST_METHOD_PROPERTY(L"Description", L"Connected Animation in combination with Implicit Animation")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ConnectedPlusThemeTransition)
        TEST_METHOD_PROPERTY(L"Description", L"Connected Animation in combination with ThemeTranstion")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ConnectedPlusThemeTransition2)
        TEST_METHOD_PROPERTY(L"Description", L"Connected Animation in combination with ThemeTranstion, 2nd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ConnectedPlusThemeTransition3)
        TEST_METHOD_PROPERTY(L"Description", L"Connected Animation in combination with Navigation ThemeTranstion")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TextAnimations)
        TEST_METHOD_PROPERTY(L"Description", L"Testing animation of text.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TextRTLAnimations)
        TEST_METHOD_PROPERTY(L"Description", L"Testing animation of text under RTL.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ImageAnimations)
        TEST_METHOD_PROPERTY(L"Description", L"Testing animation of text.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MultipleDeletes)
        TEST_METHOD_PROPERTY(L"Description", L"Testing multiple deletions of ancestors of the source")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandoffVisualInteraction)
        TEST_METHOD_PROPERTY(L"Description", L"Testing the interaction between handoff visuals and connected animations")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScrollIntoView)
        TEST_METHOD_PROPERTY(L"Description", L"Testing targting of destination element when it is being scrolled into view")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BasicConnectedAnimationConfiguration)
        TEST_METHOD_PROPERTY(L"Description", L"Testing Basic Configuration")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GravityConnectedAnimationConfiguration)
        TEST_METHOD_PROPERTY(L"Description", L"Testing Gravity Configuration")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GravityConnectedAnimationConfigurationNoShadow)
        TEST_METHOD_PROPERTY(L"Description", L"Testing Gravity Configuration with no shadow")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GravityConnectedAnimationConfigurationSameSize)
        TEST_METHOD_PROPERTY(L"Description", L"Testing Gravity Configuration with same size source and destination")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DefaultConnectedAnimationConfiguration)
        TEST_METHOD_PROPERTY(L"Description", L"Testing default Configuration")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DirectConnectedAnimationConfiguration)
        TEST_METHOD_PROPERTY(L"Description", L"Testing Direct Configuration")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimateFromContentDialog)
        TEST_METHOD_PROPERTY(L"Description", L"Testing animation from a content dialog")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimateFromPopup)
        TEST_METHOD_PROPERTY(L"Description", L"Testing animation from a Popup")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
    xaml_controls::Grid^ CreateTestPageContent(float scale = 1.0);
    xaml_animation::ConnectedAnimation^ IntializePageAndCreateCustomAnimation();
    void EndToEndHelper(SourceCopyOption sourceCopyOption = SourceCopyOption::DontCopy, float scale = 1.0);
    void ClippingHelper(float percentage);
    Microsoft::UI::Xaml::Media::Animation::ConnectedAnimation^ IntializeAndStartCoordinatedAnimation(bool disableScale);
    void StartAnimations(unsigned int nameCount, _In_count_(nameCount) LPCWSTR* sourceNames, _In_count_(nameCount) LPCWSTR* destinationNames, bool suppressScaleAnimation, _In_ LPCWSTR filetag, int filterIndex);
    void TestCoordinatedAnimations(bool disableScale);
    void ProcessConfigurationTest(_In_ xaml_animation::ConnectedAnimationConfiguration^ configuration, _In_opt_ Platform::String^ outputSuffix = L"", bool makeSame = false);

    Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride m_featureDisableDefaultConnectedAnimationConfiguration;
    Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride m_featureEnableGlobalAnimations;

};

} } } } } }

