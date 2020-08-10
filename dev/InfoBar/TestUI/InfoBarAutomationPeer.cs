using MUXControlsTestApp;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;

namespace InfoBar_TestUI
{
    public class InfoBarAutomationPeer : FrameworkElementAutomationPeer
    {
        public InfoBarAutomationPeer(FrameworkElement owner) : base(owner)
        {

        }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.StatusBar;
        }

        protected override string GetClassNameCore()
        {
            return nameof(InfoBar);
        }

        public void RaiseWindowOpenedEvent(string displayString)
        {
            AutomationPeer autPeer = this;
            autPeer.RaiseNotificationEvent(AutomationNotificationKind.Other, AutomationNotificationProcessing.CurrentThenMostRecent, displayString, "InfoBarOpenedActivityId");

            if (AutomationPeer.ListenerExists(AutomationEvents.WindowOpened))
            {
                RaiseAutomationEvent(AutomationEvents.WindowOpened);
            }
        }

        public void RaiseWindowClosedEvent(string displayString)
        {
            AutomationPeer autPeer = this;
            autPeer.RaiseNotificationEvent(AutomationNotificationKind.Other, AutomationNotificationProcessing.CurrentThenMostRecent, displayString, "InfoBarClosedActivityId");
            if (AutomationPeer.ListenerExists(AutomationEvents.WindowClosed))
            {
                RaiseAutomationEvent(AutomationEvents.WindowClosed);
            }
        }

        InfoBar GetInfoBar()
        {
            UIElement owner = Owner;
            return owner as InfoBar;
        }
    }
}
