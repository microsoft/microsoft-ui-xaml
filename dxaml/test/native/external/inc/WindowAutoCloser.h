// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class WindowAutoCloser
{
public:
    WindowAutoCloser() {}
    ~WindowAutoCloser()
    {
        Close();
    }

    void Attach(Microsoft::UI::Xaml::Window^ window)
    {
        VERIFY_IS_NULL(m_window);
        m_window = window;
    }

    void Close()
    {
        if (m_window != nullptr)
        {
            Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
            {
                m_window->Close();
            });
            m_window = nullptr;
        }
    }

    Microsoft::UI::Xaml::Window^ operator->() { return m_window; }
    Microsoft::UI::Xaml::Window^ get() { return m_window; }


private:
    Microsoft::UI::Xaml::Window^ m_window = nullptr;
};
