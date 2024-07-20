#include "pch.h"
#include "common.h"
#include "RadioButtonsTestHooks.h"

#include "RadioButtonsTestHooks.properties.cpp"

com_ptr<RadioButtonsTestHooks> RadioButtonsTestHooks::s_testHooks{};

com_ptr<RadioButtonsTestHooks> RadioButtonsTestHooks::EnsureGlobalTestHooks()
{
    static bool s_initialized = []() {
        s_testHooks = winrt::make_self<RadioButtonsTestHooks>();
        return true;
    }();
    return s_testHooks;
}

void RadioButtonsTestHooks::SetTestHooksEnabled(const winrt::RadioButtons& radioButtons, bool enabled)
{
    if (radioButtons)
    {
        winrt::get_self<RadioButtons>(radioButtons)->SetTestHooksEnabled(enabled);
    }
}

void RadioButtonsTestHooks::NotifyLayoutChanged(const winrt::RadioButtons& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_layoutChangedEventSource)
    {
        hooks->m_layoutChangedEventSource(sender, nullptr);
    }
}

winrt::event_token RadioButtonsTestHooks::LayoutChanged(winrt::TypedEventHandler<winrt::RadioButtons, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_layoutChangedEventSource.add(value);
}

void RadioButtonsTestHooks::LayoutChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_layoutChangedEventSource.remove(token);
}

int RadioButtonsTestHooks::GetRows(const winrt::RadioButtons& radioButtons)
{
    if (radioButtons)
    {
        return winrt::get_self<RadioButtons>(radioButtons)->GetRows();
    }
    return -1;
}

int RadioButtonsTestHooks::GetColumns(const winrt::RadioButtons& radioButtons)
{
    if (radioButtons)
    {
        return winrt::get_self<RadioButtons>(radioButtons)->GetColumns();
    }
    return -1;
}

int RadioButtonsTestHooks::GetLargerColumns(const winrt::RadioButtons& radioButtons)
{
    if (radioButtons)
    {
        return winrt::get_self<RadioButtons>(radioButtons)->GetLargerColumns();
    }
    return -1;
}
