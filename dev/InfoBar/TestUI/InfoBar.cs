using System;
using System.Windows.Input;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
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
        Informational,
        Success,
        Default,
        None
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
    public class InfoBarEventArgs : EventArgs
    {
        public bool IsHandled
        {
            get; set;
        }
    }

    public sealed class InfoBar : ContentControl
    {
        Button _actionButton;
        Button _alternateCloseButton;
        Button _closeButton;
        Border _myContainer;

        public event EventHandler<RoutedEventArgs> ActionButtonClick;
        public event TypedEventHandler<InfoBar, InfoBarEventArgs> CloseButtonClick;
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
            _alternateCloseButton = GetTemplateChild<Button>("AlternateCloseButton");
            _closeButton = GetTemplateChild<Button>("CloseButton");
            _actionButton = GetTemplateChild<Button>("ActionButton");
            _myContainer = GetTemplateChild<Border>("Container");

            UpdateButtonsState();
            UpdateSeverityState();
            OnIsOpenChanged();

            // Allows the user to override the default StatusColor and Icon of the Severity level
            if (IconSource != null)
            {
                OnIconSourceChanged();
            }
            if (StatusColor != Color.FromArgb(0, 0, 0, 0))
            {
                OnStatusColorChanged();
            }
            _alternateCloseButton.Click += new RoutedEventHandler(OnCloseButtonClick);
            _closeButton.Click += new RoutedEventHandler(OnCloseButtonClick);
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
            else if (property == ActionButtonContentProperty || property == CloseButtonContentProperty)
            {
                infoBar.UpdateButtonsState();
            }
            else if (property == IsOpenProperty)
            {
                infoBar.OnIsOpenChanged();
            }
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

        public bool ShowCloseButton
        {
            get { return (bool)GetValue(ShowCloseButtonProperty); }
            set { SetValue(ShowCloseButtonProperty, value); }
        }

        public static readonly DependencyProperty ShowCloseButtonProperty =
            DependencyProperty.Register(nameof(ShowCloseButton), typeof(bool), typeof(InfoBar), new PropertyMetadata(true));

        /* Message Title Properties
         * 
         */
        public string Title
        {
            get { return (string)GetValue(TitleProperty); }
            set { SetValue(TitleProperty, value); }
        }

        public static readonly DependencyProperty TitleProperty =
            DependencyProperty.Register(nameof(Title), typeof(string), typeof(InfoBar), new PropertyMetadata(""));

        public string Message
        {
            get { return (string)GetValue(MessageProperty); }
            set { SetValue(MessageProperty, value); }
        }

        public static readonly DependencyProperty MessageProperty =
            DependencyProperty.Register(nameof(Message), typeof(string), typeof(InfoBar), new PropertyMetadata(""));

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
        public object CloseButtonContent
        {
            get { return (object)GetValue(CloseButtonContentProperty); }
            set { SetValue(CloseButtonContentProperty, value); }
        }

        public static readonly DependencyProperty CloseButtonContentProperty =
            DependencyProperty.Register(nameof(CloseButtonContent), typeof(object), typeof(InfoBar), new PropertyMetadata(null, OnPropertyChanged));

        public Style CloseButtonStyle
        {
            get { return (Style)GetValue(CloseButtonStyleProperty); }
            set { SetValue(CloseButtonStyleProperty, value); }
        }

        public static readonly DependencyProperty CloseButtonStyleProperty =
            DependencyProperty.Register(nameof(CloseButtonStyle), typeof(Style), typeof(InfoBar), new PropertyMetadata(null));

        public ICommand CloseButtonCommand
        {
            get { return (ICommand)GetValue(CloseButtonCommandProperty); }
            set { SetValue(CloseButtonCommandProperty, value); }
        }

        public static readonly DependencyProperty CloseButtonCommandProperty =
            DependencyProperty.Register(nameof(CloseButtonCommand), typeof(ICommand), typeof(InfoBar), new PropertyMetadata(null));

        public object CloseButtonCommandParameter
        {
            get { return (object)GetValue(CloseButtonCommandParameterProperty); }
            set { SetValue(CloseButtonCommandParameterProperty, value); }
        }

        public static readonly DependencyProperty CloseButtonCommandParameterProperty =
            DependencyProperty.Register(nameof(CloseButtonCommandParameter), typeof(object), typeof(InfoBar), new PropertyMetadata(null));

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
            DependencyProperty.Register(nameof(IconSource), typeof(IconSource), typeof(InfoBar), new PropertyMetadata(default));

        // Methods that invoke the event handlers for Close Button and Action Button
        private void OnCloseButtonClick(object sender, RoutedEventArgs e)
        {
            lastCloseReason = InfoBarCloseReason.CloseButton;
            InfoBarEventArgs args = new InfoBarEventArgs();
            CloseButtonClick?.Invoke(this, args);
            //If the user sets IsHandled to true, they can override the behavior of CloseButtonClick
            if (args.IsHandled == false)
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
            // If the developer did not want to cancel the closing of the InfoBar, the InfoBar will collapse and the ClosedEvent will proceed as usual. 
            if (!args.Cancel)
            {
                alreadyRaised = true;
                _myContainer.Visibility = Visibility.Collapsed;
                IsOpen = false;
                Open(IsOpen);
                RaiseClosedEvent();
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

        // Updates Severity state of InfoBar
        void UpdateSeverityState()
        {
            if (Severity == InfoBarSeverity.Critical)
            {
                VisualStateManager.GoToState(this, "Critical", false);
            }
            else if (Severity == InfoBarSeverity.Warning)
            {
                VisualStateManager.GoToState(this, "Warning", false);
            }
            else if (Severity == InfoBarSeverity.Informational)
            {
                VisualStateManager.GoToState(this, "Informational", false);
            }
            else if (Severity == InfoBarSeverity.Success)
            {
                VisualStateManager.GoToState(this, "Success", false);
            }
            else if (Severity == InfoBarSeverity.None)
            {
                VisualStateManager.GoToState(this, "None", false);
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
                if (CloseButtonContent != null && ActionButtonContent != null)
                {
                    VisualStateManager.GoToState(this, "BothButtonsVisible", false);
                    VisualStateManager.GoToState(this, "NoDefaultCloseButton", false);
                }
                else if (CloseButtonContent != null)
                {
                    VisualStateManager.GoToState(this, "CloseButtonVisible", false);
                    VisualStateManager.GoToState(this, "NoDefaultCloseButton", false);
                }
                else if (ActionButtonContent != null)
                {
                    VisualStateManager.GoToState(this, "ActionButtonVisible", false);
                    VisualStateManager.GoToState(this, "DefaultCloseButton", false);
                    _alternateCloseButton.Visibility = Visibility.Visible;
                }
                else if (ActionButtonContent == null && CloseButtonContent == null)
                {
                    VisualStateManager.GoToState(this, "NoButtonsVisible", false);
                    VisualStateManager.GoToState(this, "DefaultCloseButton", false);
                    _closeButton.Visibility = Visibility.Collapsed;
                    _actionButton.Visibility = Visibility.Collapsed;
                    _alternateCloseButton.Visibility = Visibility.Visible;
                }
            }
            else
            {
                VisualStateManager.GoToState(this, "NoDefaultCloseButton", false);
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
            }
            else
            {
                VisualStateManager.GoToState(this, "Collapsed", false);
                _myContainer.Visibility = Visibility.Collapsed;
                IsOpen = false;
            }
        }

        // Updates the IconSource to the user's chosen icon
        void OnIconSourceChanged()
        {
            VisualStateManager.GoToState(this, "UserIconSource", false);
        }

        //Updates the StatusColor to the user's chosen color
        void OnStatusColorChanged()
        {
            VisualStateManager.GoToState(this, "UserStatusColor", false);
        }
    }
}