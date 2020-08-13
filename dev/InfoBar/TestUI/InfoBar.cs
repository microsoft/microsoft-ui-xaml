using System;
using System.Windows.Input;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Automation.Peers;
using Windows.Foundation;

namespace MUXControlsTestApp
{
    public enum InfoBarCloseReason
    {
        CloseButton,
        Programattic
    }

    public enum InfoBarSeverity
    {
        Critical,
        Warning,
        Success,
        Default
    }

    public class InfoBarClosedEventArgs : EventArgs
    {
        public InfoBarCloseReason Reason
        {
            get; set;
        }
    }

    public class InfoBarClosingEventArgs : EventArgs
    {
        public InfoBarCloseReason Reason
        {
            get; set;
        }
        public bool Cancel
        {
            get; set;
        }
    }

    public class CloseButtonClickEventArgs : EventArgs
    {
        public bool Handled
        {
            get; set;
        }
    }

    public sealed class InfoBar : ContentControl
    {
        Button _actionButton;
        Button _closeButton;
        TextBlock _title;
        TextBlock _message;
        HyperlinkButton _hyperlinkButton;
        IconSourceElement _standardIcon;
        IconSourceElement _userIcon;
        Grid _contentRootGrid;
        Border _myContainer;

        public event EventHandler<RoutedEventArgs> ActionButtonClick;
        public event TypedEventHandler<InfoBar, CloseButtonClickEventArgs> CloseButtonClick;
        public event TypedEventHandler<InfoBar, InfoBarClosedEventArgs> Closed;
        public event TypedEventHandler<InfoBar, InfoBarClosingEventArgs> Closing;

        private InfoBarCloseReason lastCloseReason = InfoBarCloseReason.Programattic;
        private bool alreadyRaised = false;

        public InfoBar()
        {
            this.DefaultStyleKey = typeof(InfoBar);
        }

        T GetTemplateChild<T>(string name) where T : DependencyObject
        {
            var child = GetTemplateChild(name) as T;
            return child;
        }

        protected override void OnApplyTemplate()
        {
            _actionButton = GetTemplateChild<Button>("ActionButton");
            _title = GetTemplateChild<TextBlock>("Title");
            _message = GetTemplateChild<TextBlock>("Message");
            _hyperlinkButton = GetTemplateChild<HyperlinkButton>("HyperlinkButton");
            _standardIcon = GetTemplateChild<IconSourceElement>("StandardIcon");
            _userIcon = GetTemplateChild<IconSourceElement>("UserIcon");
            _contentRootGrid = GetTemplateChild<Grid>("ContentRootGrid");
            _closeButton = GetTemplateChild<Button>("CloseButton");
            _actionButton = GetTemplateChild<Button>("ActionButton");
            _myContainer = GetTemplateChild<Border>("Container");

            UpdateButtonsState();
            UpdateSeverityState();
            OnIsOpenChanged();
            UpdateMargins();

            if (_closeButton != null)
            {
                _closeButton.Click += OnCloseButtonClick;
            }
            
            _actionButton.Click += (s, e) => ActionButtonClick?.Invoke(s, e);
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            DependencyProperty property = e.Property;
            InfoBar infoBar = d as InfoBar;
            if (property == SeverityProperty)
            {
                infoBar.UpdateSeverityState();
            }
            else if (property == ActionButtonContentProperty || property == ShowCloseButtonProperty)
            {
                infoBar.UpdateButtonsState();
            }
            else if (property == IsOpenProperty)
            {
                infoBar.OnIsOpenChanged();
            }
            else if (property == IconSourceProperty)
            {
                infoBar.OnIconChanged();
            }
            else if (property == HyperlinkButtonContentProperty)
            {
                infoBar.OnHyperlinkButtonContentChanged();
            }
            infoBar.UpdateMargins();
        }

        /* Open Properties
         * 
         */
        public bool IsOpen
        {
            get { return (bool)GetValue(IsOpenProperty); }
            set { SetValue(IsOpenProperty, value); }
        }

        public static readonly DependencyProperty IsOpenProperty =
            DependencyProperty.Register(nameof(IsOpen), typeof(bool), typeof(InfoBar), new PropertyMetadata(false, OnPropertyChanged));

        public static readonly DependencyProperty ShowCloseButtonProperty =
            DependencyProperty.Register(nameof(ShowCloseButton), typeof(bool), typeof(InfoBar), new PropertyMetadata(true, OnPropertyChanged));

        /* Message Title Properties
         * 
         */
        public string Title
        {
            get { return (string)GetValue(TitleProperty); }
            set { SetValue(TitleProperty, value); }
        }

        public static readonly DependencyProperty TitleProperty =
            DependencyProperty.Register(nameof(Title), typeof(string), typeof(InfoBar), new PropertyMetadata("", OnPropertyChanged));

        public string Message
        {
            get { return (string)GetValue(MessageProperty); }
            set { SetValue(MessageProperty, value); }
        }

        public static readonly DependencyProperty MessageProperty =
            DependencyProperty.Register(nameof(Message), typeof(string), typeof(InfoBar), new PropertyMetadata("", OnPropertyChanged));

        /* Action Button Properties
         * 
         */
        public object ActionButtonContent
        {
            get { return (object)GetValue(ActionButtonContentProperty); }
            set { SetValue(ActionButtonContentProperty, value); }
        }

        public static readonly DependencyProperty ActionButtonContentProperty =
            DependencyProperty.Register(nameof(ActionButtonContent), typeof(object), typeof(InfoBar), new PropertyMetadata(null, OnPropertyChanged));

        public Style ActionButtonStyle
        {
            get { return (Style)GetValue(ActionButtonStyleProperty); }
            set { SetValue(ActionButtonStyleProperty, value); }
        }

        public static readonly DependencyProperty ActionButtonStyleProperty =
            DependencyProperty.Register(nameof(ActionButtonStyle), typeof(Style), typeof(InfoBar), new PropertyMetadata(null));

        public ICommand ActionButtonCommand
        {
            get { return (ICommand)GetValue(ActionButtonCommandProperty); }
            set { SetValue(ActionButtonCommandProperty, value); }
        }

        public static readonly DependencyProperty ActionButtonCommandProperty =
            DependencyProperty.Register(nameof(ActionButtonCommand), typeof(ICommand), typeof(InfoBar), new PropertyMetadata(null));

        public object ActionButtonCommandParameter
        {
            get { return (object)GetValue(ActionButtonCommandParameterProperty); }
            set { SetValue(ActionButtonCommandParameterProperty, value); }
        }

        public static readonly DependencyProperty ActionButtonCommandParameterProperty =
            DependencyProperty.Register(nameof(ActionButtonCommandParameter), typeof(object), typeof(InfoBar), new PropertyMetadata(null));

        /* Close Button Properties
         * 
         */
        public ICommand CloseButtonCommand
        {
            get { return (ICommand)GetValue(CloseButtonCommandProperty); }
            set { SetValue(CloseButtonCommandProperty, value); }
        }
        
        public static readonly DependencyProperty CloseButtonCommandProperty =
            DependencyProperty.Register(nameof(CloseButtonCommand), typeof(ICommand), typeof(InfoBar), new PropertyMetadata(null));

        public bool ShowCloseButton
        {
            get { return (bool)GetValue(ShowCloseButtonProperty); }
            set { SetValue(ShowCloseButtonProperty, value); }
        }

        public object CloseButtonCommandParameter
        {
            get { return (object)GetValue(CloseButtonCommandParameterProperty); }
            set { SetValue(CloseButtonCommandParameterProperty, value); }
        }

        public static readonly DependencyProperty CloseButtonCommandParameterProperty =
            DependencyProperty.Register(nameof(CloseButtonCommandParameter), typeof(object), typeof(InfoBar), new PropertyMetadata(null));

        /* Hyperlink Properties 
         * 
         */
        public Object HyperlinkButtonContent
        {
            get { return (object)GetValue(HyperlinkButtonContentProperty); }
            set { SetValue(HyperlinkButtonContentProperty, value); }
        }

        public static readonly DependencyProperty HyperlinkButtonContentProperty =
            DependencyProperty.Register(nameof(HyperlinkButtonContent), typeof(object), typeof(InfoBar), new PropertyMetadata(null, OnPropertyChanged));

        /* Severity-Related Properties
        * 
        */
        public InfoBarSeverity Severity
        {
            get { return (InfoBarSeverity)GetValue(SeverityProperty); }
            set { SetValue(SeverityProperty, value); }
        }

        public static readonly DependencyProperty SeverityProperty =
            DependencyProperty.Register(nameof(Severity), typeof(InfoBarSeverity), typeof(InfoBar), new PropertyMetadata(InfoBarSeverity.Default, OnPropertyChanged));

        public Color StatusColor
        {
            get { return (Color)GetValue(StatusColorProperty); }
            set { SetValue(StatusColorProperty, value); }
        }

        public static readonly DependencyProperty StatusColorProperty =
            DependencyProperty.Register(nameof(StatusColor), typeof(Color), typeof(InfoBar), new PropertyMetadata(Color.FromArgb(0, 0, 0, 0)));

        public IconSource IconSource
        {
            get { return (IconSource)GetValue(IconSourceProperty); }
            set { SetValue(IconSourceProperty, value); }
        }

        public static readonly DependencyProperty IconSourceProperty =
            DependencyProperty.Register(nameof(IconSource), typeof(IconSource), typeof(InfoBar), new PropertyMetadata(default, OnPropertyChanged));

        // Methods that invoke the event handlers for Close Button and Action Button
        private void OnCloseButtonClick(object sender, RoutedEventArgs e)
        {
            lastCloseReason = InfoBarCloseReason.CloseButton;
            CloseButtonClickEventArgs args = new CloseButtonClickEventArgs();
            CloseButtonClick?.Invoke(this, args);
            if (args.Handled == false)
            {
                RaiseClosingEvent();
                alreadyRaised = false;
            }
        }

        void RaiseClosingEvent()
        {
            InfoBarClosingEventArgs args = new InfoBarClosingEventArgs();
            args.Reason = lastCloseReason;

            Closing?.Invoke(this, args);

            if (!args.Cancel)
            {
                alreadyRaised = true;
                IsOpen = false;
                Open(IsOpen);
            }
            else
            {
                // The developer has changed the Cancel property to true, indicating that they wish to Cancel the
                // closing of this tip, so we need to revert the IsOpen property to true.
                IsOpen = true;
            }
        }

        void RaiseClosedEvent()
        {
            InfoBarClosedEventArgs args = new InfoBarClosedEventArgs();
            args.Reason = lastCloseReason;
            Closed?.Invoke(this, args);
        }

        // Determines whether a custom icon or the severity level's icon is being used
        void OnIconChanged()
        {
            if (IconSource != null)
            {
                VisualStateManager.GoToState(this, "UserIconVisible", false);
            }
            else
            {
                VisualStateManager.GoToState(this, "StandardIconVisible", false);
            }
        }

        // Updates the visibility of the hyperlinkbutton
        private void OnHyperlinkButtonContentChanged()
        {
            if (HyperlinkButtonContent != null)
            {
                VisualStateManager.GoToState(this, "HyperlinkButtonVisible", false);
            }
            else
            {
                VisualStateManager.GoToState(this, "HyperlinkButtonCollapsed", false);
            }
        }

        // Updates Severity state of InfoBar
        void UpdateSeverityState()
        {
            OnIconChanged();
            if (Severity == InfoBarSeverity.Critical)
            {
                VisualStateManager.GoToState(this, "Critical", false);
            }
            else if (Severity == InfoBarSeverity.Warning)
            {
                VisualStateManager.GoToState(this, "Warning", false);
            }
            else if (Severity == InfoBarSeverity.Success)
            {
                VisualStateManager.GoToState(this, "Success", false);
            }
            else
            {
                VisualStateManager.GoToState(this, "Default", false);
            }
        }

        // Updates visibility of buttons
        void UpdateButtonsState()
        {
            if (ShowCloseButton)
            {
                if (ActionButtonContent != null)
                {
                    VisualStateManager.GoToState(this, "BothButtonsVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(this, "CloseButtonVisible", false);
                }
            }
            else
            {
                if (ActionButtonContent != null)
                {
                    VisualStateManager.GoToState(this, "ActionButtonVisible", false);
                }
                else
                {
                    VisualStateManager.GoToState(this, "NoButtonsVisible", false);
                }
            }
        }

        //Updates the margins depending on the prescence of certain visual components
        void UpdateMargins()
        {
            if (_standardIcon != null)
            {
                if ((Title != null && Title != "") || (Message != null && Message != "") || ActionButtonContent != null || HyperlinkButtonContent != null || ShowCloseButton == true)
                {
                    VisualStateManager.GoToState(this, "StandardIconRightMargin", false);
                }
                else
                {
                    VisualStateManager.GoToState(this, "StandardIconNoRightMargin", false);
                }
            }
            if (_userIcon != null)
            {
                if ((Title != null && Title != "") || (Message != null && Message != "") || ActionButtonContent != null || HyperlinkButtonContent != null || ShowCloseButton == true)
                {
                    VisualStateManager.GoToState(this, "UserIconRightMargin", false);
                }
                else
                {
                    VisualStateManager.GoToState(this, "UserIconNoRightMargin", false);
                }
            }
            if (_title != null)
            {
                if (Title != null && Title != "")
                {
                    if ((_standardIcon != null || _userIcon != null) && (Message != null && Message != ""))
                    {
                        VisualStateManager.GoToState(this, "TitleRightMargin", false);
                    }
                    else
                    {
                        VisualStateManager.GoToState(this, "TitleNoRightMargin", false);
                    }
                }
                else
                {
                    VisualStateManager.GoToState(this, "TitleNoMargin", false);
                }
            }
            if (_message != null)
            {
                if (Message != null && Message != "")
                {
                    if (ActionButtonContent != null || HyperlinkButtonContent != null)
                    {
                        VisualStateManager.GoToState(this, "MessageRightMargin", false);
                    }
                    else
                    {
                        VisualStateManager.GoToState(this, "MessageNoRightMargin", false);
                    }
                }
                else
                {
                    VisualStateManager.GoToState(this, "MessageNoMargin", false);
                }
            }
            if (_actionButton != null)
            {
                if (ActionButtonContent != null || HyperlinkButtonContent != null)
                {
                    if (HyperlinkButtonContent != null)
                    {
                        VisualStateManager.GoToState(this, "ActionButtonRightMarginHyperlinkAdjacent", false);
                    }
                    else
                    {
                        if (ShowCloseButton != false)
                        {
                            VisualStateManager.GoToState(this, "ActionButtonRightMarginCloseButtonAdjacent", false);
                        }
                        else
                        {
                            VisualStateManager.GoToState(this, "ActionButtonNoRightMargin", false);
                        }
                    }
                }
                else
                {
                    VisualStateManager.GoToState(this, "ActionButtonNoMargin", false);
                }
            }
        }

        // Updates if InfoBar is opened
        void OnIsOpenChanged()
        {
            if (IsOpen)
            {
                lastCloseReason = InfoBarCloseReason.Programattic;
                Open(IsOpen);

            }
            else if (!alreadyRaised)
            {

                RaiseClosingEvent();
                alreadyRaised = false;
            }
        }

        // Opens or closes the InfoBar
        private void Open(bool value)
        {
            if (value)
            {
                VisualStateManager.GoToState(this, "Visible", false);
                IsOpen = true;
                InfoBarAutomationPeer infoBarPeer = FrameworkElementAutomationPeer.FromElement(this) as InfoBarAutomationPeer ?? null;
                infoBarPeer.RaiseWindowOpenedEvent(Title + " " + Message);
            }
            else
            {
                VisualStateManager.GoToState(this, "Collapsed", false);
                IsOpen = false;
                RaiseClosedEvent();
                InfoBarAutomationPeer infoBarPeer = FrameworkElementAutomationPeer.FromElement(this) as InfoBarAutomationPeer ?? null;
                infoBarPeer.RaiseWindowOpenedEvent("InfoBar Dismissed");
            }
        }

        //Creates an InfoBarAutomationPeer
        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new InfoBarAutomationPeer(this);
        }
    }
}