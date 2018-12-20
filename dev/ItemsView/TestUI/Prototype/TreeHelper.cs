using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    internal static class TreeHelper
    {
        static internal ItemsViewBase FindItemsView(FrameworkElement element)
        {
            ItemsViewBase itemsView = null;
            var parent = element as FrameworkElement;
            while (itemsView == null && parent != null && !(parent is ItemsViewBase))
            {
                parent = VisualTreeHelper.GetParent(parent) as FrameworkElement;
            }

            itemsView = parent as ItemsViewBase;

            return itemsView;
        }
    }
}
