using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;

#if !BUILD_WINDOWS
using ElementFactoryGetArgs = Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs;
using ElementFactoryRecycleArgs = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs;
#endif

namespace DEPControlsTestApp.ItemsViewPrototype
{
    internal class TableElementFactory : Microsoft.UI.Xaml.Controls.IElementFactoryShim
    {
        public TableElementFactory(TableDefinition view)
        {
            m_view = view;
        }

        protected UIElement GetElementCore(ElementFactoryGetArgs context)
        {
            return m_view.GetElement(context);
        }

        protected void RecycleElementCore(ElementFactoryRecycleArgs context)
        {
            m_view.RecycleElement(context);
        }

        TableDefinition m_view;

        public UIElement GetElement(ElementFactoryGetArgs args)
        {
            return this.GetElementCore(args);
        }

        public void RecycleElement(ElementFactoryRecycleArgs args)
        {
            this.RecycleElementCore(args);
        }
    }
}
