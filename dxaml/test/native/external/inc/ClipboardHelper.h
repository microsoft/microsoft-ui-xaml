// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TestEvent.h>
#include "XamlTailored.h"

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Common {

                    class ClipboardHelper
                    {
                    public:
                        ClipboardHelper();
                        ~ClipboardHelper();

                        void VerifyClipboardText(Platform::String^ expectedContents);

                        void WaitForContentChangedEvent()
                        {
                            LOG_OUTPUT(L"Waiting for Clipboard content changed event...");
                            m_clipboardContentChanged->WaitForDefault();
                        }

                        bool ContentChangedEventHasFired()
                        {
                            return m_clipboardContentChanged->HasFired();
                        }

                        void ResetContentChangedEvent()
                        {
                            m_clipboardContentChanged->Reset();
                        }


                    private:
                        std::shared_ptr<Event> m_clipboardContentChanged;
                        wf::EventRegistrationToken m_clipboardContentChangedToken;
                    };

                }
            }
        }
    }
}
