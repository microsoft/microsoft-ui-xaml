// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <set>

namespace Tests {
    namespace Native {
        namespace External {
            namespace Framework {
                namespace ComponentConnector {

                    ref class PageHelper;
                    ref class SimulatedBinding;

                    struct CallRecord
                    {
                        CallRecord()
                            : m_count(0U)
                        {}

                        void Mark(int id)
                        {
                            ++m_count;
                            VERIFY_IS_TRUE(m_called.find(id) == m_called.end());
                            m_called.insert(id);
                        }

                        unsigned m_count;
                        std::set<int> m_called;
                    };

                    ref class SimulatedBinding sealed
                        : public Microsoft::UI::Xaml::Markup::IComponentConnector
                    {
                    public:
                        virtual void Connect(int connectionId, ::Platform::Object^ target);
                        void SetHelper(PageHelper^ helper);
                        unsigned GetCallCount();
                        virtual Microsoft::UI::Xaml::Markup::IComponentConnector^ GetBindingConnector(int connectionId, ::Platform::Object^ target);

                    private:
                        Platform::WeakReference m_wrHelper;
                        CallRecord m_calls;
                    };

                    ref class PageHelper sealed
                    {
                    public:
                        PageHelper();

                        void SetReturnNullConnector();

                        void SetPage(Microsoft::UI::Xaml::Controls::Page^ page);
                        unsigned GetCallCount(unsigned level);

                        void ValidatePageConnect(int connectionId, ::Platform::Object^ target);
                        Microsoft::UI::Xaml::Markup::IComponentConnector^ GetBindingConnector(int connectionId, ::Platform::Object^ target);

                        void ValidateBindingConnect(SimulatedBinding^ sender, int connectionId, ::Platform::Object^ target);

                        void Realize(unsigned stage);

                    private:
                        void ValidateConnectPrivate(SimulatedBinding^ sender, int connectionId, ::Platform::Object^ target);
                        void ValidateConnect0(int connectionId, ::Platform::Object^ target);
                        void ValidateConnect1(int connectionId, ::Platform::Object^ target);
                        void ValidateConnect2(int connectionId, ::Platform::Object^ target);

                    private:
                        CallRecord m_calls;
                        SimulatedBinding^ m_binding0;
                        SimulatedBinding^ m_binding1;
                        SimulatedBinding^ m_binding2;
                        Platform::WeakReference m_wrPage;
                        bool m_nullConnector;
                    };

                    ref class PageWithICC sealed
                        : public Microsoft::UI::Xaml::Controls::Page
                        , public Microsoft::UI::Xaml::Markup::IComponentConnector
                    {
                    public:
                        PageWithICC();
                        virtual void Connect(int connectionId, ::Platform::Object^ target);
                        virtual Microsoft::UI::Xaml::Markup::IComponentConnector^ GetBindingConnector(int connectionId, ::Platform::Object^ target);
                        PageHelper^ GetHelper();

                    private:
                        PageHelper^ m_helper;
                    };
                }
            }
        }
    }
}
