using Microsoft.UI.Xaml.Controls;
using Selector = Microsoft.UI.Xaml.Controls.Primitives.Selector;
using System;
using Windows.UI.Xaml;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public enum ContentOrientation
    {
        Horizontal,
        Vertical,
        Both
    }

    // TODO: all protected methods on the base to avoid having these properties show up in a derived type.
    public class ViewDefinitionBase : DependencyObject
    {
        public object /*should use IElementFactory on 17763 and later*/ GetElementFactory()
        {
            return GetElementFactoryCore();
        }

        public ContentOrientation GetContentOrientation()
        {
            return GetContentOrientationCore();
        }

        public VirtualizingLayout GetLayout()
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

        protected virtual object /*should use IElementFactory on 17763 and later*/ GetElementFactoryCore()
        {
            throw new NotImplementedException();
        }

        protected virtual ContentOrientation GetContentOrientationCore()
        {
            return ContentOrientation.Vertical;
        }

        protected virtual VirtualizingLayout GetLayoutCore()
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
