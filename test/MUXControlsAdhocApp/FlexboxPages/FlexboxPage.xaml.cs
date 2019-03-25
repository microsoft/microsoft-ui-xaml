using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsAdhocApp.FlexboxPages
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class FlexboxPage : Page
    {
        public FlexboxPage()
        {
            this.InitializeComponent();
        }

        private void InvalidateButton_Click(object sender, RoutedEventArgs e)
        {
            _flexbox.InvalidateMeasure();
        }

        private void DirectionComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexbox.Direction = EnumValueFromComboBox<FlexboxDirection>(sender);
        }

        private void JustifyContentComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexbox.JustifyContent = EnumValueFromComboBox<FlexboxJustifyContent>(sender);
        }

        private void AlignItemsComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexbox.AlignItems = EnumValueFromComboBox<FlexboxAlignItems>(sender);
        }

        private void AlignContentComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexbox.AlignContent = EnumValueFromComboBox<FlexboxAlignContent>(sender);
        }

        private void AlignSelfComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            Flexbox.SetAlignSelf(_alignSelf, EnumValueFromComboBox<FlexboxAlignSelf>(sender));
            // TODO: How does an attached child property invalidate layout?
            _flexbox.InvalidateMeasure();
        }

        private T EnumValueFromComboBox<T>(object sender) where T : struct, IComparable
        {
            string selection = (string)((ComboBox)sender).SelectedItem;
            return Enum.Parse<T>(selection);
        }

        private void NavigationView_BackRequested(Microsoft.UI.Xaml.Controls.NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewBackRequestedEventArgs args)
        {
            Frame.GoBack();
        }
    }
}
