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
            bool ShouldVisitElement(string elementName);
            bool ShouldVisitProperty(string propertyName);
            bool ShouldVisitPropertyValuePair(string propertyName, string value);
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
                return node != null && _filter.ShouldVisitElement(node.GetType().FullName);
            }

            public bool ShouldVisitPropertiesForNode(DependencyObject node)
            {
                return (node as UIElement) != null && _filter.ShouldVisitElement(node.GetType().FullName);
            }

            public bool ShouldVisitProperty(PropertyInfo propertyInfo)
            {
                return _filter.ShouldVisitProperty(propertyInfo.Name);
            }

            public void VisitProperty(string propertyName, object value)
            {
                var v = _translator.PropertyValueToString(propertyName, value);
                if (_filter.ShouldVisitPropertyValuePair(propertyName, v))
                {
                    _logger.LogProperty(_indent+1, propertyName, v);
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
                            value = property.GetValue(obj: node, index: null);
                        }
                        catch (Exception)
                        {
                            value = "Exception when read " + property.Name;
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
            private static readonly string[] _propertyNamePostfixAllowedList = new string[] {"Brush", "Thickness"};
            private List<string> _propertyNameAllowedList = new List<string> {"Background", "Foreground", "Padding", "Margin", "RenderSize", "Visibility", "Name", "CornerRadius",
                "Width", "Height", "MinWidth", "MinHeight", "MaxWidth", "MaxHeight" };
            private static readonly Dictionary<string, string> _ignorePropertyValues = new Dictionary<string, string> {
                {"MinWidth","0" },
                {"MinHeight","0" },
                {"MaxWidth","∞" },
                {"MaxHeight","∞" },
            };

            public List<string> PropertyNameAllowedList 
            {
                get { return _propertyNameAllowedList; }
                set { _propertyNameAllowedList = value; }
            }

            public virtual bool ShouldVisitPropertyValuePair(string propertyName, string value)
            {
                if (_ignorePropertyValues.ContainsKey(propertyName) 
                    && _ignorePropertyValues[propertyName].Equals(value))
                {
                    return false;
                }

                return !string.IsNullOrEmpty(value) && !value.Equals("NaN") && !value.StartsWith("Exception");
            }

            public virtual bool ShouldVisitElement(string elementName)
            {
                return true;
            }

            public virtual bool ShouldVisitProperty(string propertyName)
            {
                return (_propertyNamePostfixAllowedList.Where(item => propertyName.EndsWith(item)).Count()) > 0 || _propertyNameAllowedList.Contains(propertyName);
            }
        }

        public class DefaultPropertyValueTranslator : IPropertyValueTranslator
        {
            public virtual string PropertyValueToString(string propertyName, object value)
            {
                if (value == null)
                {
                    return ValueNULL;
                }

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
                return _logger.ToString();
            }

            private StringBuilder _logger = new StringBuilder();
            private void AppendLogger(int indent, string s)
            {
                _logger.AppendLine(s.PadLeft(2*indent + s.Length));
            }
        }
    }
}
