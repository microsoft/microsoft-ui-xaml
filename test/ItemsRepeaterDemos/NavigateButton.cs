using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterDemos
{
    class NavigateButton: Button
    {
        private Type target;
        public Type Target 
        {
            get
            {
                return target;
            }

            set
            {
                target = value;
                this.Content = value.Name;
            }
        }

        public NavigateButton()
        {
            this.Click += NavigateButton_Click;
        }

        private void NavigateButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            FrameworkElement current = (FrameworkElement)Parent;
            while(!(current is Frame))
            {
                current = (FrameworkElement)current.Parent;
            }

            (current as Frame).Navigate(Target);
        }
    }
}
