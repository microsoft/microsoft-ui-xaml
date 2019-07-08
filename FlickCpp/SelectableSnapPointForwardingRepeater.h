#pragma once

#include "sal.h"

namespace FlickCpp
{
    [Windows::UI::Xaml::Data::Bindable]
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class SelectableSnapPointForwardingRepeater sealed
        : public Microsoft::UI::Xaml::Controls::ItemsRepeater
        , Windows::UI::Xaml::Controls::Primitives::IScrollSnapPointsInfo
    {
    public:

        SelectableSnapPointForwardingRepeater();
        virtual ~SelectableSnapPointForwardingRepeater();

        // IScrollSnapPointsInfo
        virtual property bool AreHorizontalSnapPointsRegular { bool get() { return true; } }
        virtual property bool AreVerticalSnapPointsRegular { bool get() { return true; } }

        virtual event Windows::Foundation::EventHandler<Platform::Object^>^ HorizontalSnapPointsChanged;
        virtual event Windows::Foundation::EventHandler<Platform::Object^>^ VerticalSnapPointsChanged;

        virtual Windows::Foundation::Collections::IVectorView<float>^ GetIrregularSnapPoints(Windows::UI::Xaml::Controls::Orientation orientation, Windows::UI::Xaml::Controls::Primitives::SnapPointsAlignment alignment);
        virtual float GetRegularSnapPoints(Windows::UI::Xaml::Controls::Orientation orientation, Windows::UI::Xaml::Controls::Primitives::SnapPointsAlignment alignment, _Out_ float *offset);

        // SelectableSnapPointForwardingRepeater
        property int RepeatCount
        {
            int get() { return static_cast<int>(Windows::UI::Xaml::DependencyObject::GetValue(RepeatCountProperty)); }
            void set(int value)
            {
                if (value < 0)
                {
                    throw ref new Platform::InvalidArgumentException("RepeatCount must be a non-negative integer");
                }

                Windows::UI::Xaml::DependencyObject::SetValue(RepeatCountProperty, value);
            }
        }

        property Platform::Object^ SelectedItem
        {
            Platform::Object^ get() { return static_cast<Platform::Object^>(Windows::UI::Xaml::DependencyObject::GetValue(SelectedItemProperty)); }
            void set(Platform::Object^ value) { Windows::UI::Xaml::DependencyObject::SetValue(SelectedItemProperty, value); }
        }

        property int SelectedIndex
        {
            int get() { return m_selectedIndex; }
            void set(int value) { m_selectedIndex = value; }
        }

        void SetSelectedItemToNone();

        static property Windows::UI::Xaml::DependencyProperty^ RepeatCountProperty
        {
            Windows::UI::Xaml::DependencyProperty^ get() { return s_repeatCountProperty; }
        }

        static property Windows::UI::Xaml::DependencyProperty^ SelectedItemProperty
        {
            Windows::UI::Xaml::DependencyProperty^ get() { return s_selectedItemProperty; }
        }

        static property int SelectedIndexValueWhenNoItemIsSelected
        {
            int get() { return s_selectedIndexValueWhenNoItemIsSelected; }
        }
    private:
        static void RegisterDependencyProperties();

        static Windows::UI::Xaml::DependencyProperty^ s_repeatCountProperty;
        static Windows::UI::Xaml::DependencyProperty^ s_selectedItemProperty;
        static const int s_selectedIndexValueWhenNoItemIsSelected;

        int m_selectedIndex = FlickCpp::SelectableSnapPointForwardingRepeater::SelectedIndexValueWhenNoItemIsSelected;
    };
}
