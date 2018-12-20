using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;

#if !BUILD_WINDOWS
using ElementFactoryGetContext = Microsoft.UI.Xaml.Controls.ElementFactoryGetContext;
using ElementFactoryRecycleContext = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleContext;
#endif

namespace MUXControlsTestApp.ItemsViewPrototype
{
    internal class TableViewGenerator : ViewGenerator
    {
        public TableViewGenerator(TableDefinition view)
        {
            m_view = view;
        }

        protected override UIElement GetElementCore(ElementFactoryGetContext context)
        {
            return m_view.GetElement(context);
        }

        protected override void RecycleElementCore(ElementFactoryRecycleContext context)
        {
            m_view.RecycleElement(context);
        }

        TableDefinition m_view;
    }
}
