// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        namespace TestImageEnums
        {
            public enum class LoadApi
            {
                SetSource,
                SetSourceAsync,
                SetSourceAsyncTwice,
                Uri
            };

            public enum class ParentElement
            {
                Image,
                Border,
                Ellipse
            };
        }

        ref class TestImage sealed
        {
        public:

            property Platform::String^ ImagePath
            {
                Platform::String^ get() { return m_pImagePath; }
                void set(Platform::String^ value) { m_pImagePath = value; }
            }

            property xaml_media::Stretch Stretch
            {
                xaml_media::Stretch get() { return m_stretchMode; }
                void set(xaml_media::Stretch value) { m_stretchMode = value; }
            }

            property bool AutoPlay
            {
                bool get() { return m_autoPlay; }
                void set(bool value) { m_autoPlay = value; }
            }

            property bool BitmapCache
            {
                bool get() { return m_bitmapCache; }
                void set(bool value) { m_bitmapCache = value; }
            }

            property bool DecodeToRenderSize
            {
                bool get() { return m_decodeToRenderSize; }
                void set(bool value) { m_decodeToRenderSize = value; }
            }

            property TestImageEnums::LoadApi LoadApi
            {
                TestImageEnums::LoadApi get() { return m_loadApi; }
                void set(TestImageEnums::LoadApi value) { m_loadApi = value; }
            }

            property int DecodePixelWidth
            {
                int get() { return m_decodePixelWidth; }
                void set(int value) { m_decodePixelWidth = value; }
            }

            property int DecodePixelHeight
            {
                int get() { return m_decodePixelHeight; }
                void set(int value) { m_decodePixelHeight = value; }
            }
            
            property xaml_imaging::DecodePixelType DecodePixelType
            {
                xaml_imaging::DecodePixelType get() { return m_decodePixelType; }
                void set(xaml_imaging::DecodePixelType value) { m_decodePixelType = value; }
            }

            property double Opacity
            {
                double get() { return m_opacity; }
                void set(double value) { m_opacity = value; }
            }

            property xaml::Thickness NineGrid
            {
                xaml::Thickness get() { return m_nineGrid; }
                void set(xaml::Thickness value) { m_nineGrid = value; }
            }

            property wf::Size ElementSize
            {
                wf::Size get() { return m_elementSize; }
                void set(wf::Size value) { m_elementSize = value; }
            }

            property TestImageEnums::ParentElement ParentElement
            {
                TestImageEnums::ParentElement get() { return m_parentElement; }
                void set(TestImageEnums::ParentElement value) { m_parentElement = value; }
            }

            property xaml_imaging::BitmapCreateOptions BitmapCreateOptions
            {
                xaml_imaging::BitmapCreateOptions get() { return m_createOptions; }
                void set(xaml_imaging::BitmapCreateOptions value) { m_createOptions = value; }
            }

            property bool TrimAndRestoreHardwareResources
            {
                bool get() { return m_trimAndRestoreHardwareResources; }
                void set(bool value) { m_trimAndRestoreHardwareResources = value; }
            }

            property int ImageEventWaitTime
            {
                int get() { return m_imageEventWaitTime; }
                void set(int value) { m_imageEventWaitTime = value; }
            }

            property bool FailureExpected
            {
                bool get() { return m_failureExpected; }
                void set(bool value) { m_failureExpected = value; }
            }

            TestImage();

            // Future features to add:
            // - Support for border brushes
            // - Support for transforms (this is a bit tricky since the actual transform object like ScaleTransform must
            //   be created after the panel that hosts it is created)

        private:

            Platform::String^ m_pImagePath;
            xaml_media::Stretch m_stretchMode;
            bool m_autoPlay;
            bool m_bitmapCache;
            bool m_decodeToRenderSize;
            int m_decodePixelWidth;
            int m_decodePixelHeight;
            xaml_imaging::DecodePixelType m_decodePixelType;
            double m_opacity;
            xaml::Thickness m_nineGrid;
            wf::Size m_elementSize;
            TestImageEnums::LoadApi m_loadApi;
            TestImageEnums::ParentElement m_parentElement;
            xaml_imaging::BitmapCreateOptions m_createOptions;
            bool m_trimAndRestoreHardwareResources;
            int m_imageEventWaitTime;
            bool m_failureExpected;
        };

        ref class ImageTestEngine sealed
        {
        public:

            property wf::Size WindowSize
            {
                wf::Size get() { return m_windowSize; }
                void set(wf::Size value) { m_windowSize = value; }
            }

            property float ZoomScale
            {
                float get() { return m_zoomScale; }
                void set(float value) { m_zoomScale = value; }
            }

            property Microsoft::UI::Xaml::Tests::Common::DCompRendering DCompRenderingMode
            {
                Microsoft::UI::Xaml::Tests::Common::DCompRendering get() { return m_dcompRenderingMode; }
                void set(Microsoft::UI::Xaml::Tests::Common::DCompRendering value) { m_dcompRenderingMode = value; }
            }

            property MockDComp::SurfaceIdMode MockDCompSurfaceIdMode
            {
                MockDComp::SurfaceIdMode get() { return m_mockDCompSurfaceIdMode; }
                void set(MockDComp::SurfaceIdMode value) { m_mockDCompSurfaceIdMode = value; }
            }

            property MockDComp::SurfaceComparison MockDCompVerification
            {
                MockDComp::SurfaceComparison get() { return m_mockDCompVerification; }
                void set(MockDComp::SurfaceComparison value) { m_mockDCompVerification = value; }
            }

            void AddTestImage(TestImage^ testImage)
            {
                m_testImages.push_back(testImage);
            }

            ImageTestEngine();

            void Execute();

            // Future features to add:
            // - Add a clip setting to test CulledFromRenderWalk
            // - Add ETW event checking/counting (with payload?)
            // - Add visibility toggling... not sure how to expose such a method/property
            // - Add ability to write out xaml for parsing instead of programmatically adding things to the panel element
            // - Add ability to change whether all images are loaded and then checked for ImageOpened (should be configurable, currently it waits for each image to load)
            // - Add ability to reference the same BitmapImage in multiple Image containers
            // - Add support for BorderBrush
            // - Add support for Shape Stroke
            // - Add tests for WriteableBitmap

        private:

            static xaml_imaging::BitmapImage^ SetupBitmapImage(TestImage^ pTestImage);
            static xaml::FrameworkElement^ SetupFrameworkElement(TestImage^ pTestImage, xaml_imaging::BitmapImage^ pBitmapImage);
            static void SetupSource(TestImage^ pTestImage, xaml_imaging::BitmapSource^ pBitmapSource);

            std::vector<TestImage^> m_testImages;
            wf::Size m_windowSize;
            float m_zoomScale;
            Microsoft::UI::Xaml::Tests::Common::DCompRendering m_dcompRenderingMode;
            MockDComp::SurfaceComparison m_mockDCompVerification;
            MockDComp::SurfaceIdMode m_mockDCompSurfaceIdMode;
        };

    } } }
} } } }
