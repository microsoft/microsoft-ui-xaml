using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public class PrototypePager_ElementFactory : IElementFactory
    {
        public PrototypePager_ElementFactory()
        {
        }

        public UIElement GetElement(ElementFactoryGetArgs args)
        {

            Type dataType = args.Data.GetType();

            if (dataType.Equals(typeof(Button)))
            {
                var button = args.Data as Button;
                button.Style = (Style)App.Current.Resources["NumberPanelNotSelectedButtonStyle"];
                return button;
            }
            else
            {
                return new Button() {
                    Content = args.Data,
                    Tag = args.Data,
                    Style = (Style)App.Current.Resources["NumberPanelNotSelectedButtonStyle"],
                };
            }
        }

        public void RecycleElement(ElementFactoryRecycleArgs args)
        { 
        }
    }
}
