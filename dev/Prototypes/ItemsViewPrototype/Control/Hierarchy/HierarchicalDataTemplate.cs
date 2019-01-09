using System;
using System.Collections;
using System.Diagnostics;
using mux = Microsoft.UI.Xaml.Controls;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Markup;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    [ContentProperty(Name = nameof(HierarchicalDataTemplate.Template))]
    public sealed class HierarchicalDataTemplate : DependencyObject, mux.IElementFactoryShim
    {
        // Ideally this would be derived from something like FrameworkTemplate and have x:Bind support making
        // it unnecessary to have this Template property.
        public DataTemplate Template
        {
            get
            {
                if (m_template == null)
                {
                    return DefaultTemplate;
                }

                return m_template;
            }
            set
            {
                m_template = value;
                SetValue(TemplateProperty, value);
            }
        }

        public DataTemplate ItemTemplate
        {
            get
            {
                if (m_itemTemplate == null)
                {
                    return DefaultTemplate;
                }

                return m_itemTemplate;
            }
            set
            {
                m_itemTemplate = value;
                SetValue(ItemTemplateProperty, value);
            }
        }

        public Binding ItemsSource
        {
            get { return m_itemsSource; }
            set
            {
                m_itemsSource = value;
                SetValue(ItemsSourceProperty, value);
            }
        }

        #region Property Identifiers

        public static readonly DependencyProperty TemplateProperty =
            DependencyProperty.Register(
                nameof(Template),
                typeof(DataTemplate),
                typeof(HierarchicalDataTemplate),
                new PropertyMetadata(null, OnTemplateChanged));

        public static readonly DependencyProperty ItemTemplateProperty =
            DependencyProperty.Register(
                nameof(ItemTemplate),
                typeof(DataTemplate),
                typeof(HierarchicalDataTemplate),
                new PropertyMetadata(null, OnItemTemplateChanged));

        public static readonly DependencyProperty ItemsSourceProperty =
            DependencyProperty.Register(nameof(ItemsSource),
                typeof(Binding),
                typeof(HierarchicalDataTemplate),
                new PropertyMetadata(DependencyProperty.UnsetValue, OnItemsSourceChanged));

        #endregion

        #region Property changed handlers

        private static void OnTemplateChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((HierarchicalDataTemplate)d).m_template = e.NewValue as DataTemplate;
        }

        private static void OnItemsSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((HierarchicalDataTemplate)d).m_itemsSource = e.NewValue as Binding;
        }

        private static void OnItemTemplateChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((HierarchicalDataTemplate)d).m_itemTemplate = e.NewValue as DataTemplate;
        }

        #endregion

        #region IElementFactory implementation
        public UIElement GetElement(mux.ElementFactoryGetArgs context)
        {
            UIElement root = this.GetTemplateElement(context);

            if (ApiInformation.IsMethodPresent(typeof(DataTemplate).FullName, nameof(DataTemplate.GetElement)))
            {
                // convert context type only because this sample is using MUX rather than Windows.Internal
                Windows.UI.Xaml.ElementFactoryGetArgs args = new Windows.UI.Xaml.ElementFactoryGetArgs();
                args.Data = context.Data;
                args.Parent = context.Parent;
                root = this.Template.GetElement(args);
            }
            else
            {
                root = this.Template.LoadContent() as UIElement;
            }

            // If hierarchical properties are set then wrap the root with a StackPanel and add an ItemsRepeater
            // to generate the sub-items.
            var itemsSourceExpression = this.ReadLocalValue(ItemsSourceProperty) as BindingExpression;

            if (ItemTemplate != null && itemsSourceExpression != null)
            {
                var stack = new StackPanel();
                stack.Children.Add(root);
                var repeater = new mux.ItemsRepeater();

                var binding = this.GenerateBinding(itemsSourceExpression);
                repeater.SetBinding(mux.ItemsRepeater.ItemsSourceProperty, binding);

                repeater.ItemTemplate = this.ItemTemplate;

                // TODO: Possibly expose this for customization.
                //repeater.Layout = owner.Layout; // by default it uses a StackLayout
                stack.Children.Add(repeater);
                root = stack;
            }

            if (root is FrameworkElement)
            {
                (root as FrameworkElement).DataContext = context.Data;
            }

            return root;
        }

        public void RecycleElement(mux.ElementFactoryRecycleArgs context)
        {
            if (ApiInformation.IsMethodPresent(typeof(DataTemplate).FullName, nameof(DataTemplate.RecycleElement)))
            {
                var args = new ElementFactoryRecycleArgs();
                args.Element = context.Element;
                args.Parent = context.Parent;

                this.Template.RecycleElement(args);
            }
        }

        #endregion

        #region Helper methods

        private UIElement GetTemplateElement(mux.ElementFactoryGetArgs context)
        {
            UIElement element = null;
            if (ApiInformation.IsMethodPresent(typeof(DataTemplate).FullName, nameof(DataTemplate.GetElement)))
            {
                // convert context type only because this sample is using MUX rather than Windows.Internal
                Windows.UI.Xaml.ElementFactoryGetArgs args = new Windows.UI.Xaml.ElementFactoryGetArgs();
                args.Data = context.Data;
                args.Parent = context.Parent;
                element = this.Template.GetElement(args);
            }
            else
            {
                element = this.Template.LoadContent() as UIElement;
            }

            return element;
        }

        private Binding GenerateBinding(BindingExpression expression)
        {
            var binding = new Binding();
            binding.Converter = expression.ParentBinding.Converter;
            binding.ConverterLanguage = expression.ParentBinding.ConverterLanguage;
            binding.ConverterParameter = expression.ParentBinding.ConverterParameter;
            binding.ElementName = expression.ParentBinding.ElementName;
            binding.FallbackValue = expression.ParentBinding.FallbackValue;
            binding.Mode = expression.ParentBinding.Mode;
            if (expression.ParentBinding?.Path != null)
            {
                binding.Path = new PropertyPath(expression.ParentBinding.Path.Path);
            }
            binding.RelativeSource = expression.ParentBinding.RelativeSource;
            binding.Source = expression.ParentBinding.Source;
            binding.TargetNullValue = expression.ParentBinding.TargetNullValue;
            binding.UpdateSourceTrigger = UpdateSourceTrigger.Explicit;

            return binding;
        }

        #endregion

        private DataTemplate m_template;
        private DataTemplate m_itemTemplate;
        private Binding m_itemsSource;
        private static readonly string DefaultTemplateString = "<DataTemplate xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\"><TextBlock Text=\"{Binding}\"/></DataTemplate>";
        private static readonly DataTemplate DefaultTemplate = XamlReader.Load(DefaultTemplateString) as DataTemplate;
    }
}
