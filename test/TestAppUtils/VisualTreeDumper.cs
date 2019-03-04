// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;

namespace MUXControls.TestAppUtils
{
    public class VisualTreeDumper
    {
        public interface IFilter
        {
            bool ShouldLogElement(string elementName);
            bool ShouldLogProperty(string propertyName);
            bool ShouldLogPropertyValuePair(string propertyName, string value);
        }

        public interface IPropertyValueTranslator
        {
            string PropertyValueToString(string propertyName, Object value);
        }

        public interface IVisualTreeLogger
        {
            void BeginNode(int indent, string nodeName, DependencyObject obj);
            void EndNode(int indent, string nodeName, DependencyObject obj);
            void LogProperty(int indent, string propertyName, object propertyValue);
        }

        class Visitor
        {
            private IVisualTreeLogger _logger;
            private int _indent;
            private IFilter _filter;
            private IPropertyValueTranslator _translator;
            public Visitor(IFilter filter, IPropertyValueTranslator translator, IVisualTreeLogger logger)
            {
                _indent = 0;
                _filter = filter;
                _translator = translator;
                _logger = logger;
            }
            public void EndVisitNode(DependencyObject obj)
            {
                _indent--;
                _logger.EndNode(_indent, obj.GetType().FullName, obj);
            }

            public void BeginVisitNode(DependencyObject obj)
            {
                _logger.BeginNode(_indent, obj.GetType().FullName, obj);
                _indent++;
            }

            public override string ToString()
            {
                return _logger.ToString();
            }

            public bool ShouldVisitNode(DependencyObject node)
            {
                return node != null && _filter.ShouldLogElement(node.GetType().FullName);
            }

            public bool ShouldVisitPropertiesForNode(DependencyObject node)
            {
                return (node as UIElement) != null && _filter.ShouldLogElement(node.GetType().FullName);
            }

            public bool ShouldVisitProperty(PropertyInfo propertyInfo)
            {
                return _filter.ShouldLogProperty(propertyInfo.Name);
            }

            public void VisitProperty(string propertyName, object value)
            {
                var v = _translator.PropertyValueToString(propertyName, value);
                if (!_filter.ShouldLogPropertyValuePair(propertyName, v))
                {
                    _logger.LogProperty(_indent+1, propertyName, value);
                }
            }
        }

        public static string DumpToXML(DependencyObject root, IPropertyValueTranslator translator, IFilter filter, IVisualTreeLogger logger)
        {

            Visitor visitor = new Visitor(filter ?? new DefaultFilter(), 
                translator ?? new DefaultPropertyValueTranslator(),
                logger ?? new DefaultVisualTreeLogger());
            WalkThroughTree(root, visitor);

            return visitor.ToString();
        }

        private static void WalkThroughProperties(DependencyObject node, Visitor visitor)
        {
            if (visitor.ShouldVisitPropertiesForNode(node))
            {
                var properties = node.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance);
                foreach (var property in properties)
                {
                    if (visitor.ShouldVisitProperty(property))
                    {
                        Object value = null;

                        try
                        {
                            value = property.GetValue(node, null);
                        }
                        catch (Exception)
                        {
                            value = "Exception";
                        }
                        visitor.VisitProperty(property.Name, value);
                    }
                }
            }
        }
        private static void WalkThroughTree(DependencyObject node, Visitor visitor)
        {
            if (visitor.ShouldVisitNode(node))
            {
                visitor.BeginVisitNode(node);

                WalkThroughProperties(node, visitor);
                for (int i = 0; i < VisualTreeHelper.GetChildrenCount(node); i++)
                {
                    WalkThroughTree(VisualTreeHelper.GetChild(node, i), visitor);
                }

                visitor.EndVisitNode(node);
            }
        }

        public static readonly string ValueNULL = "[NULL]";
        public class DefaultFilter : IFilter
        {
            private static readonly string[] _propertyNamePostfixWhiteList = new string[] {"Brush", "Thickness"};
            private static readonly string[] _propertyNameWhiteList = new string[] {"Background", "Foreground", "Padding", "Margin", "RenderSize", "Visibility", "Name"};
            public virtual bool ShouldLogPropertyValuePair(string propertyName, string value)
            {
                return false;
            }

            public virtual bool ShouldLogElement(string elementName)
            {
                return true;
            }

            public virtual bool ShouldLogProperty(string propertyName)
            {
                return (_propertyNamePostfixWhiteList.Where(item => propertyName.EndsWith(item)).Count()) > 0 || _propertyNameWhiteList.Contains(propertyName);
            }
        }

        public class DefaultPropertyValueTranslator : IPropertyValueTranslator
        {
            public virtual string PropertyValueToString(string propertyName, object value)
            {
                if (value == null)
                    return ValueNULL;

                var brush = value as SolidColorBrush;
                if (brush != null)
                {
                    return brush.Color.ToString();
                }
                return value.ToString();
            }
        }

        public class DefaultVisualTreeLogger : IVisualTreeLogger
        {
            public void BeginNode(int indent, string nodeName, DependencyObject obj)
            {
                AppendLogger(indent, string.Format("[{0}]", nodeName));
            }

            public void EndNode(int indent, string nodeName, DependencyObject obj)
            {                
            }

            public void LogProperty(int indent, string propertyName, object propertyValue)
            {
                AppendLogger(indent, string.Format("{0}={1}", propertyName, propertyValue));
            }

            public override string ToString()
            {
                return _sb.ToString();
            }

            private StringBuilder _sb = new StringBuilder();
            private void AppendLogger(int indent, string s)
            {
                _sb.AppendLine(s.PadLeft(2*indent + s.Length));
            }
        }
    }
}
