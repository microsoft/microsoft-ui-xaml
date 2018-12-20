using System.Collections.Generic;
using Windows.Foundation;
using Windows.System;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Input;
using Windows.Devices.Input;
using Microsoft.UI.Xaml.Controls;
using System.Diagnostics;
using System;
using Windows.UI.Xaml.Automation.Peers;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public enum SelectionMode
    {
        None,
        Single,
        SingleToggle,
        Multiple,
        Extended,
        Custom
    }

    /// <summary>
    /// Selector is an attempt at enabling a "has-a" relationship with controls that do selection
    /// instead of the current "is-a" approach (e.g. ListView is a Selector).  This requires separating
    /// the policy and interactions that drives selection from a specific control.  The Selector has a
    /// Delegate property that is a SelectorDelegateBase.  It is responsible for providing the policy
    /// (a.k.a selection mode such as single, multi, extended).
    /// </summary>
    /// <remarks>
    /// This class is like a GestureRecognizer, but... for selection.  It attaches to various input
    /// handlers and contains the logic to trigger a selection action.  It could potentially be a "behavior",
    /// but can only work with elements that can provide a selection index and a way to be told they
    /// should toggle their selected/unselected state.  This prototype uses an ISelectable interface, but
    /// interfaces don't version well from a platform API perspective so it would likely be something different
    /// when productized.
    /// When an element is registered we'll attach appropriate input handlers to call us back so we can
    /// update the selection model's state based on the policy implemented by the delegate.  We hold a reference
    /// to the element so that if the state changes programmatically we can update all the registered elements
    /// to which our current selection policy is applied.
    /// </remarks>
    public class Selector
    {
        public Selector()
        {
            this.Mode = SelectionMode.Single;
            this.Model.SelectionChanged += OnSelectionChanged;
        }

        public SelectionModel Model
        {
            get
            {
                if (m_selectionModel == null)
                {
                    m_selectionModel = new SelectionModel();
                }

                return m_selectionModel;
            }
        }

        public object Source
        {
            get
            {
                return Model.Source;
            }

            set
            {
                // Source is changing, let go of all the currently tracked selectables.
                foreach (var selectable in m_selectables)
                {
                    Unregister(selectable as UIElement);
                }

                m_selectables.Clear();

                Model.Source = value;
            }
        }

        public bool CanSelectMultiple
        {
            get
            {
                return this.DelegateInternal is ExtendedSelector || this.DelegateInternal is MultipleSelector;
            }
        }

        public bool IsSelectionRequired
        {
            get
            {
                return this.DelegateInternal is SingleSelector;
            }
        }

        public SelectionMode Mode
        {
            get { return m_selectionMode; }
            set
            {
                if (m_selectionMode != value)
                {
                    m_selectionMode = value;
                    // The SelectionModel.SingleSelect property doesn't play well with having specific handlers
                    // implement the policy.  We'll disable it by default and then only enable it for the two specific
                    // selection policies on which it's useful.
                    this.Model.SingleSelect = false;
                    switch (m_selectionMode)
                    {
                        case SelectionMode.None:            // No selection
                            this.Model.ClearSelection();
                            DelegateInternal = new NullSelector();
                            break;
                        case SelectionMode.Single:          // Select only one item and select on focus received (e.g. a simple combobox)
                            this.Model.SingleSelect = true;
                            var index = this.Model.SelectedIndex;
                            if (index == null)
                            {
                                // nothing is selected and there is data, select the first item
                                if (this.Model.Source != null)
                                {
                                    var source = this.Model.Source as ItemsSourceView;
                                    if ((source == null && DataSource.CreateFrom(this.Model.Source).GetSize() > 0) ||
                                       (source != null && source.GetSize() > 0))
                                    {
                                        this.Model.Select(0);
                                    }

                                }
                            }
                            else
                            {
                                this.Model.SelectAt(index);
                            }

                            DelegateInternal = new SingleSelector();
                            break;
                        case SelectionMode.SingleToggle:    // Select only one item, but only when invoked and not on focus received (e.g. a NavigationView / hamburger menu)
                            this.Model.SingleSelect = true;
                            DelegateInternal = new SingleSelector() { FollowFocus = false };
                            break;
                        case SelectionMode.Multiple:        // Select multiple items, but not on focus received.  Toggle selection on invoke. (e.g. a multi-select combobox)
                            DelegateInternal = new MultipleSelector();
                            break;
                        case SelectionMode.Extended:        // Single select behavior where Shift goes into range selection mode and Ctrl goes into multiple selection mode
                            DelegateInternal = new ExtendedSelector();
                            break;
                        case SelectionMode.Custom:          // Rely on a custom SelectorDelegateBase implementation to mutate the model when a selection action is recognized
                            DelegateInternal = this.Delegate;
                            break;
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the Delegate to use to mutate the current selection state based on a custom policy.
        /// </summary>
        public SelectorDelegateBase Delegate
        {
            get
            {
                return m_delegate;
            }
            set
            {
                if (value != m_delegate)
                {
                    m_delegate = value;
                    if (this.Mode == SelectionMode.Custom && this.DelegateInternal != m_delegate)
                    {
                        this.DelegateInternal = m_delegate;
                    }
                }
            }
        }

        #region UIElement registration methods

        #region Attached properties

        /// <summary>
        /// This is a reference to the UIElement that is acting as the container of selectable elements.
        /// </summary>
        /// <remarks>
        /// If we enabled an attached behavior-like approach then this would be the UIElement to which the
        /// Selector is attached.  Ideally, we could have the automation peer for that UIElement recognize
        /// a Selector is attached and then do the right thing for accessibility relying on the attached
        /// Selector's current state (SelectionModel) to respond to requests that come through UIAutomation.
        /// Getting this to "just work" would require work in the framework for something like
        /// FrameworkElementAutomationPeer to be smarter.
        ///
        /// For now, it's up to the automation peer of the selection container to do the right thing.
        /// The ItemsControl has its ItemsControlAutomationPeer ask this Selector for an automation ISelectionProvider
        /// instance.  The SelectorAutomationProvider that it returns derives from AutomationPeer directly
        /// instead of FrameworkElementAutomationPeer since the Selector isn't a FrameworkElement.  AutomationPeer requires
        /// that it implement a RawProvider property that returns an IRawElementProviderSimple.  I'm
        /// trying to fake things out a bit and have the SelectorAutomationProvider just return the IRawElementProviderSimple
        /// for this Container instead of being null.
        /// </remarks>
        public UIElement SelectionContainer
        {
            get { return m_containerElement; }
            set
            {
                if (m_containerElement != value)
                {
                    m_containerElement = value;
                    //this.OnPropertyChanged("Container");
                }
            }
        }

        #endregion

        public void RegisterSelectable(UIElement element)
        {
            if (element as ISelectable == null) return;

            element.GettingFocus += Element_GettingFocus;
            element.LosingFocus += Element_LosingFocus;
            element.KeyDown += Element_KeyDown;
            element.PointerReleased += Element_PointerReleased;
            element.AddHandler(UIElement.PointerPressedEvent, (PointerEventHandler)Element_PointerPressed, true);
            element.AddHandler(UIElement.ContextRequestedEvent, (TypedEventHandler<UIElement, ContextRequestedEventArgs>)Element_ContextRequested, true);

            m_selectables.Add((ISelectable)element);
        }

        public void UnregisterSelectable(UIElement element)
        {
            if (element as ISelectable == null) return;

            Unregister(element);
            m_selectables.Remove((ISelectable)element);
        }

        public void InvalidateSelectable(UIElement element)
        {
            var selectable = element as ISelectable;
            if (selectable == null) return;


            var index = ((IRepeatedIndex)selectable).GetIndexPath();
            EnsurePathIsValid(index);
            selectable.IsSelected = this.Model.IsSelectedAt(index) ?? false;
        }

        #endregion

        private bool IsCtrlDown
        {
            get
            {
                return Window.Current.CoreWindow.GetAsyncKeyState(VirtualKey.Control).HasFlag(CoreVirtualKeyStates.Down);
            }
        }

        private bool IsShiftDown
        {
            get
            {
                return Window.Current.CoreWindow.GetAsyncKeyState(VirtualKey.Shift).HasFlag(CoreVirtualKeyStates.Down);
            }
        }

        /// <summary>
        /// We'd make it easy to switch the selection mode by setting a property and keep the
        /// implementation for Single, Multiple, etc. as internal-only.  Someone could provide a custom
        /// implementation, assign it to the public Delegate property and then set the Mode property to Custom
        /// to have it be used.
        /// </summary>
        private SelectorDelegateBase DelegateInternal
        {
            get { return m_delegateInternal; }
            set
            {
                if (value != m_delegateInternal)
                {
                    if (m_delegateInternal != null)
                    {
                        m_delegateInternal.Model = null;
                    }

                    m_delegateInternal = value;
                    if (m_delegateInternal != null)
                    {
                        m_delegateInternal.Model = this.Model;
                    }
                }
            }
        }

        // We'll listen to a variety of input events to detect selection and trigger potential updates to the
        // model as a user interacts with the element.  The goal is to abstract the interaction so that the logic
        // can be re-usable and avoid something like Windows.UI.Xaml.Controls.Primivites.SelectorItem where a control
        // must derive from it in order to get selection support.


        // Trigger the selection logic when receiving focus.  We rely on GettingFocus instead of GotFocus since it
        // appears to happens synchronously whereas GotFocus may fire asynchronously after one or more ticks have passed.
        // In cases where a user holds down the arrow key and causes scrolling we want any changes in selection state
        // to occur and be reflected synchronously.  Users expect an item to visually change state in sync with keyboard
        // focus.  Having it potentially happen after the focus rect has moved 1 or more items ahead would look jarring.
        private void Element_GettingFocus(UIElement sender, GettingFocusEventArgs args)
        {
            var selectable = sender as IRepeatedIndex;
            if (args.Handled || selectable == null) return;

            if (args.FocusState == FocusState.Keyboard)
            {
                if (args.OldFocusedElement is Windows.UI.Xaml.Controls.Primitives.Popup &&
                    args.Direction == FocusNavigationDirection.None)
                    // Focus coming from a flyout or context menu that was opened via Shift+F10 while the
                    // item had focus, but may not have previously been selected.  Skip changing anything here.
                    return;

                if (args.OldFocusedElement == null && args.Direction == FocusNavigationDirection.None)
                    // An edge case...  The item is currently focused and not selected but other items
                    // are selected.  The user switches to a different window and then switches back to
                    // the current window causing the item to lose and then get focus again.
                    // Skip any automatic triggering of selection via keyboard activation since from
                    // the user's perspective focus within that app didn't leave.  Coming back shouldn't
                    // automatically change its selection state and potentially affect others.
                    return;

                if (args.Direction == FocusNavigationDirection.Previous || args.Direction == FocusNavigationDirection.Next)
                    // Current behavior on a list is that Tab and Shift+Tab return focus to the item that previously held it without
                    // changing any selection state.
                    return;

                if (this.DelegateInternal != null)
                {
                    this.DelegateInternal.OnFocusedAction(selectable.GetIndexPath(), IsCtrlDown, IsShiftDown);
                }
            }
        }

        // Want to avoid selection on touch when holding triggers the popup and then the user lifts their
        // finger.  Focus would just move to a flyout or context menu.
        private void Element_LosingFocus(UIElement sender, LosingFocusEventArgs args)
        {
            if (args.Direction == FocusNavigationDirection.None
                && args.NewFocusedElement is Windows.UI.Xaml.Controls.Primitives.Popup)
            {
                m_lastPressedSelectableElement = null;
            }
        }

        // Handle Ctrl+A and Space/Enter when the item already has focus
        private void Element_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            var selectable = sender as IRepeatedIndex;
            if (e.Handled || selectable == null) return;

            if (e.Key == VirtualKey.Enter || e.Key == VirtualKey.Space)
            {
                if (this.DelegateInternal != null)
                {
                    this.DelegateInternal.OnPrimaryInteractionAction(selectable.GetIndexPath(), IsCtrlDown, IsShiftDown);
                }
            }
            else if (e.Key == VirtualKey.A && IsCtrlDown)
            {
                if (this.DelegateInternal != null)
                {
                    this.DelegateInternal.SelectAll();
                }
            }
        }

        private void Element_ContextRequested(UIElement sender, ContextRequestedEventArgs args)
        {
            // If a context menu was shown we want to avoid selecting it
            var selectable = sender as ISelectable;
            if (!args.Handled || selectable == null) return;

            if (sender == m_lastPressedSelectableElement)
            {
                m_lastPressedSelectableElement = null;
            }
        }

        // Trigger selection for pointer interactions after the pointer is released.  It
        // could move outside the item and then be released as a type of cancel action by
        // the user.
        private void Element_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            var selectable = sender as IRepeatedIndex;
            if (e.Handled || selectable == null) return;

            // Do nothing if the element isn't the same one on which the pointer press was initiated.
            if (sender == m_lastPressedSelectableElement)
            {
                if (this.DelegateInternal != null)
                {
                    this.DelegateInternal.OnPrimaryInteractionAction(selectable.GetIndexPath(), IsCtrlDown, IsShiftDown);
                }
            }

            m_lastPressedSelectableElement = null;
        }

        // For mouse we'll select on left-mouse clicks when the pointer is released
        private void Element_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            var point = e.GetCurrentPoint(sender as UIElement);
            if (point.PointerDevice.PointerDeviceType != PointerDeviceType.Mouse
                || point.Properties.IsLeftButtonPressed)
            {
                m_lastPressedSelectableElement = sender as UIElement;
            }
        }

        private void RaiseAutomationEvent(ISelectable selectable, bool oldValue)
        {
            // AutomationEvents.PropertyChanged is used as a value that means dont raise anything
            AutomationEvents eventToRaise =
                oldValue ?
                    (Model.SingleSelect ? AutomationEvents.PropertyChanged : AutomationEvents.SelectionItemPatternOnElementRemovedFromSelection) :
                    (Model.SingleSelect ? AutomationEvents.SelectionItemPatternOnElementSelected : AutomationEvents.SelectionItemPatternOnElementAddedToSelection);

            if (eventToRaise != AutomationEvents.PropertyChanged && AutomationPeer.ListenerExists(eventToRaise))
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(selectable as UIElement);
                peer.RaiseAutomationEvent(eventToRaise);
            }
        }

        private void OnSelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            Debug.WriteLine("Updating Selection status...");
            foreach (var selectable in m_selectables)
            {
                var index = ((IRepeatedIndex)selectable).GetIndexPath();
                EnsurePathIsValid(index);
                bool wasSelected = selectable.IsSelected;
                selectable.IsSelected = this.Model.IsSelectedAt(index) ?? false;
                // Debug.WriteLine(index + ":" + selectable.IsSelected.ToString());
                if (wasSelected != selectable.IsSelected)
                {
                    RaiseAutomationEvent(selectable, wasSelected);
                }
            }
        }

        private void EnsurePathIsValid(IndexPath index)
        {
            bool isValid = true;
            for (int i = 0; i < index.GetSize(); i++)
            {
                if (index.GetAt(i) < 0)
                {
                    isValid = false;
                    break;
                }
            }

            if (!isValid)
            {
                throw new InvalidOperationException("Index path is invalid. Item has been recycled");
            }
        }

        private void Unregister(UIElement element)
        {
            element.GettingFocus -= Element_GettingFocus;
            element.LosingFocus -= Element_LosingFocus;
            element.KeyDown -= Element_KeyDown;
            element.PointerReleased -= Element_PointerReleased;
            element.PointerPressed -= Element_PointerPressed;
            element.RemoveHandler(UIElement.PointerPressedEvent, (PointerEventHandler)Element_PointerPressed);
        }

        private SelectionModel m_selectionModel;
        private SelectionMode m_selectionMode;
        private UIElement m_containerElement;
        private SelectorDelegateBase m_delegateInternal;
        private SelectorDelegateBase m_delegate;
        private UIElement m_lastPressedSelectableElement = null;
        private IList<ISelectable> m_selectables = new List<ISelectable>();
    }
}