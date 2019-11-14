using System;
using Windows.UI.Xaml;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    // Need to derive from DependencyObject?
    // Would implementations of this base type want to add dependency properties?
    public class ItemsViewColumnDefinitionBase: DependencyObject
    {
        public virtual string ColumnName { get; set; }

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