#include "pch.h"
#include "Class.h"
#include "Class.g.cpp"

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the public WinUI documentation.

namespace winrt::WinUISnoopTap::implementation
{
    int32_t Class::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void Class::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
