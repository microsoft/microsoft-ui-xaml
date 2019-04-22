using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsAdhocApp.FlexboxPages
{
    public sealed partial class FlexboxPlaygroundPage : Page
    {
        public FlexboxPlaygroundPage()
        {
            this.InitializeComponent();
        }

        private void InvalidateButton_Click(object sender, RoutedEventArgs e)
        {
            _flexbox.InvalidateMeasure();
        }

        private void DirectionComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexboxLayout.Direction = EnumValueFromComboBox<FlexboxDirection>(sender);
        }

        private void WrapComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexboxLayout.Wrap = EnumValueFromComboBox<FlexboxWrap>(sender);
        }

        private void JustifyContentComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexboxLayout.JustifyContent = EnumValueFromComboBox<FlexboxJustifyContent>(sender);
        }

        private void AlignItemsComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexboxLayout.AlignItems = EnumValueFromComboBox<FlexboxAlignItems>(sender);
        }

        private void AlignContentComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _flexboxLayout.AlignContent = EnumValueFromComboBox<FlexboxAlignContent>(sender);
        }

        private void AlignSelfComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            FlexboxLayout.SetAlignSelf(_alignSelf, EnumValueFromComboBox<FlexboxAlignSelf>(sender));
        }

        private T EnumValueFromComboBox<T>(object sender) where T : struct, IComparable
        {
            string selection = (string)((ComboBox)sender).SelectedItem;
            return Enum.Parse<T>(selection);
        }
    }
}
