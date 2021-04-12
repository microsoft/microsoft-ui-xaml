using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Shapes;

namespace Experiments
{
    [ContentProperty(Name = "ItemsSource")]
    public sealed class SegmentedControl : Control
    {
        public SegmentedControl()
        {
            this.DefaultStyleKey = typeof(SegmentedControl);
        }

        public object ItemsSource
        {
            get { return (object)GetValue(ItemsSourceProperty); }
            set { SetValue(ItemsSourceProperty, value); }
        }

        public static readonly DependencyProperty ItemsSourceProperty =
            DependencyProperty.Register("ItemsSource", typeof(object), typeof(SegmentedControl), new PropertyMetadata(0));


        public object ItemTemplate
        {
            get { return (object)GetValue(ItemTemplateProperty); }
            set { SetValue(ItemTemplateProperty, value); }
        }

        public static readonly DependencyProperty ItemTemplateProperty =
            DependencyProperty.Register("ItemTemplate", typeof(object), typeof(SegmentedControl), new PropertyMetadata(0));


        public int SelectedIndex
        {
            get { return (int)GetValue(SelectedIndexProperty); }
            set { SetValue(SelectedIndexProperty, value); }
        }

        public static readonly DependencyProperty SelectedIndexProperty =
            DependencyProperty.Register("SelectedIndex", typeof(int), typeof(SegmentedControl), new PropertyMetadata(0));


        public object SelectedItem
        {
            get { return (object)GetValue(SelectedItemProperty); }
            set { SetValue(SelectedItemProperty, value); }
        }

        public static readonly DependencyProperty SelectedItemProperty =
            DependencyProperty.Register("SelectedItem", typeof(object), typeof(SegmentedControl), new PropertyMetadata(0));




        public bool ShowPillVisual
        {
            get { return (bool)GetValue(ShowPillVisualProperty); }
            set { SetValue(ShowPillVisualProperty, value); }
        }

        public static readonly DependencyProperty ShowPillVisualProperty =
            DependencyProperty.Register("ShowPillVisual", typeof(bool), typeof(SegmentedControl), new PropertyMetadata(false));



        protected override void OnApplyTemplate()
        {
            m_repeater = GetTemplateChild("ItemsRepeater") as ItemsRepeater;
            m_repeater.ElementPrepared += OnElementPrepared;
            m_repeater.ElementClearing += OnElementClearing;
            m_animationBackPlate = GetTemplateChild("AnimationBackPlate") as Rectangle;
        }

        private void OnElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            if (args.Element is ToggleButton toggleButton)
            {
                toggleButton.Checked += OnToggleButtonCheckedOrUnchecked;
                toggleButton.Unchecked += OnToggleButtonCheckedOrUnchecked;

                if (args.Index == SelectedIndex)
                {
                    MoveSelectionTo(toggleButton, args.Index);
                }
            }
        }

        private void OnToggleButtonCheckedOrUnchecked(object sender, RoutedEventArgs e)
        {
            var toggleButton = (ToggleButton)sender;
            if (toggleButton.IsChecked.GetValueOrDefault()
                && sender != m_selectedButton)
            {
                MoveSelectionTo(toggleButton, m_repeater.GetElementIndex(toggleButton));
            }
        }

        private void OnElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            if (args.Element is ToggleButton toggleButton)
            {
                toggleButton.Checked -= OnToggleButtonCheckedOrUnchecked;
                toggleButton.Unchecked -= OnToggleButtonCheckedOrUnchecked;
            }
        }

        private void MoveSelectionTo(ToggleButton toggleButton, int index)
        {
            if (m_selectedButton != null)
            {
                m_selectedButton.IsChecked = false;
            }

            m_selectedButton = toggleButton;
            if (!m_selectedButton.IsChecked.GetValueOrDefault())
            {
                m_selectedButton.IsChecked = true;
            }

            if (SelectedIndex != index)
            {
                SelectedIndex = index;
            }
            SelectedItem = m_repeater.ItemsSourceView.GetAt(index);

            var slot = LayoutInformation.GetLayoutSlot(toggleButton);

            m_animationBackPlate.Width = slot.Width;

            if (ShowPillVisual)
            {
                m_animationBackPlate.Margin = new Thickness(slot.X, slot.Y + slot.Height + 3, 0, 0);
                m_animationBackPlate.Height = 3;
            }
            else
            {
                m_animationBackPlate.Margin = new Thickness(slot.X, slot.Y, 0, 0);
                m_animationBackPlate.Height = slot.Height;
            }
        }



        private ToggleButton m_selectedButton;
        private ItemsRepeater m_repeater;
        private Rectangle m_animationBackPlate;
    }

}
