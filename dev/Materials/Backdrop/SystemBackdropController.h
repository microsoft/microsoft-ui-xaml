#pragma once

namespace SystemBackdropComponentInternal
{
    struct ISystemBackdropController
    {
        virtual ~ISystemBackdropController() {}

        virtual void Activate() = 0;
        virtual void Deactivate() = 0;
        virtual void SetHighContrast(bool isHighContrast) = 0;
        virtual void UpdateTheme(winrt::Windows::UI::Xaml::ElementTheme theme) = 0;
    };
}
