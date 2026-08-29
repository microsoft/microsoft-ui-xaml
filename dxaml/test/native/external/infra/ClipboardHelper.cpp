// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ClipboardHelper.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace WEX::Common;
using namespace WEX::Logging;
using namespace Microsoft::UI::Text;
using namespace test_infra;
using namespace MockDComp;
using namespace std;
using namespace ::Windows::ApplicationModel;
using namespace ::Windows::ApplicationModel::DataTransfer;
using namespace concurrency;


namespace Microsoft {  namespace UI { namespace Xaml {
  namespace Tests { namespace Common {
        ClipboardHelper::ClipboardHelper()
        {
            m_clipboardContentChanged = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                m_clipboardContentChangedToken = Clipboard::ContentChanged += ref new wf::EventHandler<Platform::Object^>([&](Platform::Object ^sender, Platform::Object ^args)
                {
                    LOG_OUTPUT(L"Clipboard ContentChanged Callback...");
                    DataPackageView^ clipboardContent = Clipboard::GetContent();

                    if (clipboardContent->Contains(StandardDataFormats::Html))
                    {
                        LOG_OUTPUT(L"Clipboard content contains StandardDataFormats::Html...");
                    }
                    else if (clipboardContent->Contains(StandardDataFormats::WebLink))
                    {
                        LOG_OUTPUT(L"Clipboard content contains StandardDataFormats::WebLink...");
                    }
                    else if (clipboardContent->Contains(StandardDataFormats::ApplicationLink))
                    {
                        LOG_OUTPUT(L"Clipboard content contains StandardDataFormats::ApplicationLink...");
                    }
                    else if (clipboardContent->Contains(StandardDataFormats::Rtf))
                    {
                        LOG_OUTPUT(L"Clipboard content contains StandardDataFormats::Rtf...");
                    }
                    else if (clipboardContent->Contains(StandardDataFormats::Bitmap))
                    {
                        LOG_OUTPUT(L"Clipboard content contains StandardDataFormats::Bitmap...");
                    }
                    else if (clipboardContent->Contains(StandardDataFormats::StorageItems))
                    {
                        LOG_OUTPUT(L"Clipboard content contains StandardDataFormats::StorageItems...");
                    }
                    else if (clipboardContent->Contains(StandardDataFormats::Text))
                    {
                        LOG_OUTPUT(L"Clipboard content contains StandardDataFormats::Text...");
                    }
                    else
                    {
                        LOG_OUTPUT(L"ContentChanged event fired but no valid contents found! Perhaps the clipboard was cleared.");
                    }
                    m_clipboardContentChanged->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Clearing clipboard.");
                Clipboard::Clear();
            });
            WaitForContentChangedEvent();
            ResetContentChangedEvent();
            TestServices::WindowHelper->WaitForIdle();
        }


        ClipboardHelper::~ClipboardHelper()
        {
            RunOnUIThread([&]()
            {
                Clipboard::ContentChanged -= m_clipboardContentChangedToken;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void ClipboardHelper::VerifyClipboardText(Platform::String^ expectedContents)
        {
            auto clipboardTextVerified = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                auto clipboardContent = Clipboard::GetContent();
                VERIFY_IS_TRUE(clipboardContent->Contains(StandardDataFormats::Text));
                create_task(clipboardContent->GetTextAsync()).then([&](Platform::String^ text)
                {
                    LOG_OUTPUT(L"Clipboard Text: %s", text->Data());
                    VERIFY_ARE_EQUAL(expectedContents, text);
                    clipboardTextVerified->Set();
                });

            });
            clipboardTextVerified->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

    } } } }
}