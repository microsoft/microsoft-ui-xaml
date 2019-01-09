using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml;

namespace DEPControlsTestApp.ItemsViewPrototype
{
#pragma warning disable CS8305 // Type is for evaluation purposes only and is subject to change or removal in future updates.
    public class RecyclePool : Microsoft.UI.Xaml.Controls.RecyclePool
    {
        public static string GetReuseKey(DependencyObject obj)
        {
            return (string)obj.GetValue(ReuseKeyProperty);
        }

        public static void SetReuseKey(DependencyObject obj, string value)
        {
            obj.SetValue(ReuseKeyProperty, value);
        }

        public static readonly DependencyProperty ReuseKeyProperty =
            DependencyProperty.RegisterAttached("ReuseKey", typeof(string), typeof(RecyclePool), new PropertyMetadata(null));
    }
#pragma warning restore CS8305 // Type is for evaluation purposes only and is subject to change or removal in future updates.
}
