// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Layout {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class CustomLayoutUserControl sealed
        : public Microsoft::UI::Xaml::Controls::UserControl
    {
    public:
        CustomLayoutUserControl() : m_invalidatesAncestor(false) { }
        void ActivateCustomLayout() { m_invalidatesAncestor = true; }

    protected:
        ::Windows::Foundation::Size ArrangeOverride(::Windows::Foundation::Size finalSize) override
        {
            if (m_invalidatesAncestor)
            {
                auto p1 = (FrameworkElement^)Parent;
                auto p2 = (FrameworkElement^)p1->Parent;
                auto p3 = (FrameworkElement^)p2->Parent;
                p3->InvalidateArrange();
            }

            return __super::ArrangeOverride(finalSize);
        }

    private:
        bool m_invalidatesAncestor;
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class ViewportUserControl sealed
        : public Microsoft::UI::Xaml::Controls::UserControl
    {
    public:
        ViewportUserControl()
        {
            SizeChanged += ref new Microsoft::UI::Xaml::SizeChangedEventHandler(this, &ViewportUserControl::OnSizeChanged);
            RegisterAsScrollPort(this);
        }

        void OnSizeChanged(Platform::Object ^sender, Microsoft::UI::Xaml::SizeChangedEventArgs ^e)
        {
            Microsoft::UI::Xaml::Media::RectangleGeometry^ clip = ref new Microsoft::UI::Xaml::Media::RectangleGeometry();
            clip->Rect = ::Windows::Foundation::Rect(0, 0, e->NewSize.Width, e->NewSize.Height);
            Clip = clip;
        }
    };

} } } } }