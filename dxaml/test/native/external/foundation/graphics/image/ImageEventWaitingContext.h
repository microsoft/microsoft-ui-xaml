// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <memory>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

    class ImageEventWaitingContext
    {
    public:
        void Attach(xaml_imaging::BitmapImage ^bitmapImage)
        {
            m_openedRegistration.Attach(
                bitmapImage,
                ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"ImageOpened event fired");
                m_opened = true;
                m_imageEvent->Set();
            }));

            m_failedRegistration.Attach(
                bitmapImage,
                ref new xaml::ExceptionRoutedEventHandler([&](Platform::Object^ sender, xaml::ExceptionRoutedEventArgs^)
            {
                LOG_OUTPUT(L"ImageFailed event fired");
                m_failed = true;
                m_imageEvent->Set();
            }));
        }

        void Detach()
        {
            m_openedRegistration.Detach();
            m_failedRegistration.Detach();
        }

        void WaitOpened()
        {
            LOG_OUTPUT(L"Waiting for an image event");
            m_imageEvent->WaitForDefault();
            VERIFY_IS_FALSE(m_failed);
            VERIFY_IS_TRUE(m_opened);
        }

        void WaitFailed()
        {
            LOG_OUTPUT(L"Waiting for an image event");
            m_imageEvent->WaitForDefault();
            VERIFY_IS_TRUE(m_failed);
            VERIFY_IS_FALSE(m_opened);
        }

        bool IsOpened() const
        {
            return m_opened;
        }

        bool IsFailed() const
        {
            return m_failed;
        }

        void ResetState()
        {
            m_opened = false;
            m_failed = false;
        }

    private:
        SafeEventRegistrationType(xaml_imaging::BitmapImage, ImageOpened) m_openedRegistration =
            CreateSafeEventRegistration(xaml_imaging::BitmapImage, ImageOpened);
        SafeEventRegistrationType(xaml_imaging::BitmapImage, ImageFailed) m_failedRegistration =
            CreateSafeEventRegistration(xaml_imaging::BitmapImage, ImageFailed);
        std::shared_ptr<Common::Event> m_imageEvent = std::make_shared<Common::Event>();
        bool m_opened = false;
        bool m_failed = false;
    };

}}}}}}}
