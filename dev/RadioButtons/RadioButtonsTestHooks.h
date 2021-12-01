#pragma once

#include "RadioButtons.h"

#include "RadioButtonsTestHooks.g.h"

class RadioButtonsTestHooks :
    public winrt::implementation::RadioButtonsTestHooksT<RadioButtonsTestHooks>
{
public:
    static com_ptr<RadioButtonsTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<RadioButtonsTestHooks> EnsureGlobalTestHooks();

    static void SetTestHooksEnabled(const winrt::RadioButtons& radioButtons, bool enabled);

    static int GetRows(const winrt::RadioButtons& radioButtons);
    static int GetColumns(const winrt::RadioButtons& radioButtons);
    static int GetLargerColumns(const winrt::RadioButtons& radioButtons);

    static void NotifyLayoutChanged(const winrt::RadioButtons& sender);
    static winrt::event_token LayoutChanged(winrt::TypedEventHandler<winrt::RadioButtons, winrt::IInspectable> const& value);
    static void LayoutChanged(winrt::event_token const& token);

private:
    static com_ptr<RadioButtonsTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::RadioButtons, winrt::IInspectable>> m_layoutChangedEventSource;
};
