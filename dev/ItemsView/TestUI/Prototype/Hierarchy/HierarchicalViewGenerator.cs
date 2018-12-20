using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    [ContentProperty(Name = "Templates")]
    public class HierarchicalViewGenerator : ElementFactory
    {

        public delegate void SelectTemplateEventHandler(object sender, SelectTemplateArgs args);

        public event SelectTemplateEventHandler SelectTemplate;

        private Dictionary<string, DataTemplate> _templates;

        public Dictionary<string, DataTemplate> Templates
        {
            get
            {
                if (_templates == null)
                {
                    _templates = new Dictionary<string, DataTemplate>();
                }

                return _templates;
            }
        }

        protected override UIElement GetElementCore(Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs context)
        {
            var owner = context.Parent as ItemsRepeater;
            var args = new SelectTemplateArgs() { Owner = owner };
            SelectTemplate(this, args);
            var template = Templates[args.TemplateKey];
            UIElement root = null;

            var data = context.Data;

            var hierarchicalTemplate = template as HierarchicalDataTemplate;
            if (hierarchicalTemplate != null && hierarchicalTemplate.ItemsSource != null)
            {
                var element = hierarchicalTemplate.Template.LoadContent() as UIElement;
                var stack = new StackPanel();
                stack.Children.Add(element);
                var repeater = new ItemsRepeater();
                // TODO: Ability to bind to a property here is not available in uwp unfortunately. so currently
                // only is-a model is supported.
                repeater.ItemsSource = data;
                repeater.ItemTemplate = this;
                // TODO: Possibly expose this for customization.
                repeater.Layout = owner.Layout;
                stack.Children.Add(repeater);
                root = stack;
            }
            else
            {
                var element = template.LoadContent() as UIElement;
                root = element;
            }

            (root as FrameworkElement).DataContext = data;

            return root;
        }

        protected override void RecycleElementCore(Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs context)
        {
            // No recycling yet.
        }
    }

    public class SelectTemplateArgs : EventArgs
    {
        public int Index { get; set; }

        public ItemsRepeater Owner { get; set; }

        public string TemplateKey { get; set; }

    }
}
