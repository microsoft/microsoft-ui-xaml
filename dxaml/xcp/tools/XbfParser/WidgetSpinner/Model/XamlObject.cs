// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;

namespace Microsoft.Xaml.WidgetSpinner.Model
{
    /// <summary>
    /// Abstract representation of a XAML DependencyObject
    /// </summary>
    public class XamlObject
    {
        /// <summary>
        /// Map of xmlns prefixes to full XML namespaces defined within the scope of
        /// this object's source markup
        /// </summary>
        public IReadOnlyDictionary<string, XamlXmlNamespace> Namespaces { get; }

        /// <summary>
        /// Property-value store for this object
        /// </summary>
        public IReadOnlyDictionary<XamlProperty, object> PropertyStore { get; }

        /// <summary>
        /// Holds dictionary children of this object if its type implements IDictionary
        /// </summary>
        public IReadOnlyDictionary<object, object> DictionaryItems { get; }

        /// <summary>
        /// Holds collection children of this object if its type implements ICollection
        /// </summary>
        public IReadOnlyList<object> CollectionItems { get; }

        /// <summary>
        /// Holds descendants which have registered names (i.e. x:Name) within this object's namescope
        /// </summary>
        public IReadOnlyDictionary<string, object> NameScope { get; }

        /// <summary>
        /// This object's type
        /// </summary>
        public XamlType Type { get; internal set; }

        /// <summary>
        /// This object's markup parent (this is usually, but not necessarily, the same as DependencyObject.Parent)
        /// </summary>
        public XamlObject Parent { get; internal set; }

        internal XamlObject()
        {
            Namespaces = new ReadOnlyDictionary<string, XamlXmlNamespace>(m_internalNamespacesDictionary);
            PropertyStore = new ReadOnlyDictionary<XamlProperty, object>(m_internalPropertyStore);
            NameScope = new ReadOnlyDictionary<string, object>(m_internalNameScope);
            DictionaryItems = new ReadOnlyDictionary<object, object>(m_internalDictionary);
            CollectionItems = new ReadOnlyCollection<object>(m_internalCollection);
        }

        /// <summary>
        /// Look up in this object's namescope the object with the registered name
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public object FindName(string name)
        {
            object retval;
            m_internalNameScope.TryGetValue(name, out retval);

            return retval;
        }

        public override string ToString()
        {
            var builder = new StringBuilder();
            builder.Append($"{Type}");

            object name;
            if (PropertyStore.TryGetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.DependencyObject_Name), out name))
            {
                builder.Append($" \"{name}\"");
            }

            return builder.ToString();
        }

        internal void SetValue(XamlProperty property, object value)
        {
            object oldValue;
            if (m_internalPropertyStore.TryGetValue(property, out oldValue))
            {
                var oldValueAsXamlObject = oldValue as XamlObject;
                if (oldValueAsXamlObject != null)
                {
                    oldValueAsXamlObject.Parent = null;
                }
            }

            // Currently "last write wins". This implies no support for conditional XAML per se
            // (that information is simply thrown away)
            m_internalPropertyStore[property] = value;

            var xamlObject = value as XamlObject;
            if (xamlObject != null)
            {
                xamlObject.Parent = this;
            }
        }

        internal void AddCollectionItem(object item)
        {
            m_internalCollection.Add(item);
            SetParentOnCollectionChildIfNecessary(item, this);
        }

        internal void RemoveCollectionItem(object item)
        {
            if (m_internalCollection.Remove(item))
            {
                SetParentOnCollectionChildIfNecessary(item, null);
            }
        }

        internal void AddDictionaryItem(object key, object item)
        {
            RemoveDictionaryItem(key);
            m_internalDictionary[key] = item;
            SetParentOnCollectionChildIfNecessary(item, this);
        }

        internal void RemoveDictionaryItem(object key)
        {
            object oldItem;
            if (m_internalDictionary.TryGetValue(key, out oldItem))
            {
                SetParentOnCollectionChildIfNecessary(oldItem, null);
                m_internalDictionary.Remove(key);
            }
        }

        internal void AddNamespace(string prefix, XamlXmlNamespace xamlXmlNamespace)
        {
            m_internalNamespacesDictionary[prefix] = xamlXmlNamespace;
        }

        internal void RegisterName(string name, object namedObject)
        {
            m_internalNameScope[name] = namedObject;
        }

        internal void UnregisterName(string name)
        {
            m_internalNameScope.Remove(name);
        }

        internal void ClearRegisteredNames()
        {
            m_internalNameScope.Clear();
        }

        private static void SetParentOnCollectionChildIfNecessary(object child, XamlObject parent)
        {
            var childAsXamlObject = child as XamlObject;
            if (childAsXamlObject != null)
            {
                childAsXamlObject.Parent = parent;
            }
        }

        protected List<object> m_internalCollection = new List<object>();
        protected Dictionary<object, object> m_internalDictionary = new Dictionary<object, object>();
        protected Dictionary<string, XamlXmlNamespace> m_internalNamespacesDictionary = new Dictionary<string, XamlXmlNamespace>();
        protected Dictionary<XamlProperty, object> m_internalPropertyStore = new Dictionary<XamlProperty, object>();
        protected Dictionary<string, object> m_internalNameScope = new Dictionary<string, object>();
    }
}
