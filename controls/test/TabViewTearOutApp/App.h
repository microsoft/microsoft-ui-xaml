#pragma once

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime

#include "App.xaml.g.h"

#pragma pop_macro("GetCurrentTime")

namespace winrt::TabViewTearOutApp::implementation
{
    struct App : AppT<App>
    {
        App();

        static void InitInstance();
        static winrt::com_ptr<App> GetInstance();

    private:
        static winrt::com_ptr<App> s_instance;
    };
}
