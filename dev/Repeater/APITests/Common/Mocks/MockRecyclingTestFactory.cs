using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Text;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks
{
    public class MockRecyclingTestFactory : RecyclingElementFactory
    {

        public List<object> DataContextsInGetElement = new List<object>();

        protected override UIElement GetElementCore(Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs args)
        {
            var uiElement = base.GetElementCore(args);
            if(uiElement is FrameworkElement frameworkElement)
            {
                DataContextsInGetElement.Add(frameworkElement.DataContext);
            }
            return base.GetElementCore(args);
        }

        protected override void RecycleElementCore(Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs args)
        {
            base.RecycleElementCore(args);
        }

    }
}
