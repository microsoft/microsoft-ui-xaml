using Microsoft.UI.Xaml.Controls;
using System;
using Windows.UI.Xaml;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    // TODO: all protected methods on the base to avoid having these properties show up in a derived type.
    public class ViewDefinitionBase : DependencyObject
    {
        public ViewGenerator GetViewGenerator()
        {
            return GetViewGeneratorCore();
        }

        public VirtualizingLayoutBase GetLayout()
        {
            return GetLayoutCore();
        }

        public UIElement GetHeader()
        {
            return GetHeaderCore();
        }

        public RecyclePool GetRecyclePool()
        {
            return m_recyclePool;
        }

        internal void RecyclePool(RecyclePool value)
        {
            m_recyclePool = value;
        }

        internal void Selector(Selector value)
        {
            m_selector = value;
        }

        public Selector Selector()
        {
            return m_selector;
        }

        protected virtual ViewGenerator GetViewGeneratorCore()
        {
            throw new NotImplementedException();
        }

        protected virtual VirtualizingLayoutBase GetLayoutCore()
        {
            throw new NotImplementedException();
        }

        protected virtual UIElement GetHeaderCore()
        {
            throw new NotImplementedException();
        }

        

        RecyclePool m_recyclePool;
        Selector m_selector;
    }
}
