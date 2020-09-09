using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public class PrototypePagerElementFactory : ElementFactory
    {
        protected override UIElement GetElementCore(Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs args)
        {
            if (args.Data == null)
            {
                return null;
            }
            Type dataType = args.Data.GetType();

            if (dataType == typeof(int))
            {
                return new Button() {
                    Content = args.Data,
                    Tag = args.Data,
                    Style = (Style)App.Current.Resources["NumberPanelButtonStyle"],
                };
            }
            else
            {
                (args.Data as FrameworkElement).MinWidth = (double)App.Current.Resources["NumberPanelButtonWidth"];
                return (UIElement)args.Data;
            }
        }

        protected override void RecycleElementCore(Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs args)
        { 
        }
    }
}
