// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PhasingTests.g.h"
#include "MyItem.h"

namespace winrt::BindTestbed::implementation
{
    struct PhasingTests : PhasingTestsT<PhasingTests>
    {
        PhasingTests();
        void InitializeValues();
        void Reset_Click(IInspectable const&, wux::RoutedEventArgs const&);
        void Reload_Click(IInspectable const&, wux::RoutedEventArgs const&);
        void StackPanel_PointerReleased(IInspectable const&, wux::Input::PointerRoutedEventArgs const&);
        void SlowPhasing_UnChecked(IInspectable const&, wux::RoutedEventArgs const&);
        void SlowPhasing_Checked(IInspectable const&, wux::RoutedEventArgs const&);
        void PhasedTemplate_UnChecked(IInspectable const&, wux::RoutedEventArgs const&);
        void PhasedTemplate_Checked(IInspectable const&, wux::RoutedEventArgs const&);

        void MyGridView_ContainerContentChanging(
            wuxc::ListViewBase const& sender, 
            wuxc::ContainerContentChangingEventArgs const& args);

    private:
        bool Initialized = false;
        int itemsCount = 10;
        void wait(uint32_t msTime);
        wfc::IObservableVector<IInspectable> myItems{ nullptr };
        event_token cccEventToken;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct PhasingTests : PhasingTestsT<PhasingTests, implementation::PhasingTests>
    {
    };
}
