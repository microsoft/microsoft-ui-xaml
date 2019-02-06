using System.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Interop;
using Windows.UI.Xaml.Media;

namespace Utilities
{
    public static class Utilities
    {
        public static bool IsRS4OrHigher
        {
            get
            {
                return Windows.Foundation.Metadata.ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 6);
            }
        }

        public static object GetItemAtIndex(IEnumerable items, int index)
        {
            object item = null;
            IBindableVectorView bindableVectorView = items as IBindableVectorView;
            if (bindableVectorView != null)
            {
                item = bindableVectorView.GetAt((uint)index);
            }
            else
            {
                IList list = items as IList;
                if (list != null)
                {
                    item = list[index];
                }
            }
            return item;
        }

        public static T GetChildOfType<T>(DependencyObject rootElement) where T : class
        {
            if (rootElement != null)
            {
                int childCount = VisualTreeHelper.GetChildrenCount(rootElement);
                for (int i = 0; i < childCount; i++)
                {
                    DependencyObject current = VisualTreeHelper.GetChild(rootElement, i);
                    T t = current as T;
                    if (t != null)
                    {
                        return t;
                    }
                }

                for (int i = 0; i < childCount; i++)
                {
                    DependencyObject current = VisualTreeHelper.GetChild(rootElement, i);
                    T t = GetChildOfType<T>(current);
                    if (t != null)
                    {
                        return t;
                    }
                }
            }

            return null;
        }
    }
}
