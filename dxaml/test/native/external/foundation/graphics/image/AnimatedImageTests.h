// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        class AnimatedImageTests : public WEX::TestClass<AnimatedImageTests>
        {
        public:
            BEGIN_TEST_CLASS(AnimatedImageTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"02f4717a-a120-494b-a28e-9a2cbd41ca58;bd1463b3-e5f2-4d54-9394-63a431c53a6e;d04573b8-e899-4822-bb72-9f4743c89d36")

#if defined(_WIN64) // Disable the test on 64-bit build because of floating point differences between x87 and sse instructions
                TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
#endif
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(BitmapImageInitialState)
                TEST_METHOD_PROPERTY(L"Description", L"Checks default state of the newly created BitmapImage")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StaticBitmapImageStateBmp)
                TEST_METHOD_PROPERTY(L"Description", L"Checks state of BitmapImage loaded from bmp")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StaticBitmapImageStateGif)
                TEST_METHOD_PROPERTY(L"Description", L"Checks state of BitmapImage loaded from static gif")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NoAutoPlay)
                TEST_METHOD_PROPERTY(L"Description", L"Checks it's not playing when AutoPlay=false")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SimpleImageElement)
                TEST_METHOD_PROPERTY(L"Description", L"Renders an animated image.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // IsPlaying by default
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimatedSetUri)
                TEST_METHOD_PROPERTY(L"Description", L"See if a URI based image animates.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimatedSetSource)
                TEST_METHOD_PROPERTY(L"Description", L"See if a SetSource based image animates.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimatedSetSourceNotInTree)
                TEST_METHOD_PROPERTY(L"Description", L"SetSource is called on the image not being in the live tree.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimatedSetSourceAsyncPreLiveTree)
                TEST_METHOD_PROPERTY(L"Description", L"See if a SetSourceAsync based image animates (before it is live in tree).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimatedSetSourceAsyncPostLiveTree)
                TEST_METHOD_PROPERTY(L"Description", L"See if a SetSourceAsync based image animates (after it is live in tree).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimatedSWBrush)
                TEST_METHOD_PROPERTY(L"Description", L"Validate animated image decode path for SW surface.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlateauScale)
                TEST_METHOD_PROPERTY(L"Description", L"Check that animation is not interrupted by plateau scale change.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // TODO: ETWWaiterProxy does not detect an ImageAnimationEndInfo_value event in WPF hosting mode.
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")

            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeviceLost)
                // Disable on OneCore due to timing issues looking for animation completed event.
                TEST_METHOD_PROPERTY(L"Description", L"Check that animation is not interrupted by device lost.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            Platform::String^ GetImagePath(Platform::String ^imageFileName) const;
        };

    } } }
} } } }

