// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace CustomControls {

        public ref class MyButton sealed : public Microsoft::UI::Xaml::Controls::ContentControl
        {
        public:
            Platform::Object^ GetLastOldContent()
            {
                return m_lastOldContent;
            }

            Platform::Object^ GetLastNewContent()
            {
                return m_lastNewContent;
            }

            virtual void OnContentChanged(Platform::Object^ oldContent, Platform::Object^ newContent) override
            {
                m_lastOldContent = oldContent;
                m_lastNewContent = newContent;
            }
            
        private:
            Platform::Object^ m_lastOldContent;
            Platform::Object^ m_lastNewContent;
        };

}
