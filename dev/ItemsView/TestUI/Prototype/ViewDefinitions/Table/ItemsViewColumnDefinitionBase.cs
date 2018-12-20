using System;
using Windows.UI.Xaml;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    // Need to derive from DependencyObject since implimentations of this 
    // base type might want to add dependency properties.
    public class ItemsViewColumnDefinitionBase: DependencyObject
    {
        protected ItemsViewColumnDefinitionBase()
        {
        }

        public UIElement GetHeader()
        {
            return GetHeaderCore();
        }

        public UIElement GetCellContent()
        {
            return GetCellContentCore();
        }

        protected virtual UIElement GetHeaderCore()
        {
            throw new NotImplementedException();
        }

        protected virtual UIElement GetCellContentCore()
        {
            throw new NotImplementedException();
        }
    }
}