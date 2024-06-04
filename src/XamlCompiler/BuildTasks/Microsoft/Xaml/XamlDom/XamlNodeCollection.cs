// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    internal class XamlNodeCollection<T> : IList<T> where T: XamlDomNode
    {
        private XamlDomNode _parentNode;
        private List<T> _nodes;
        private bool _isSealed;

        public XamlNodeCollection(XamlDomNode parent)
        {
            _parentNode = parent;
        }

        public int Count
        {
            get { return Nodes.Count; }
        }

        public void Seal()
        {
            _isSealed = true;
            foreach (XamlDomNode node in _nodes)
            {
                node.Seal();
            }
        }

        public bool IsSealed { get { return _isSealed; } }

        #region IList<T> Members
        public void Add(T node)
        {
            CheckSealed();
            Nodes.Add(node);
            SetParent(node);
        }
        public int IndexOf(T item)
        {
            return Nodes.IndexOf(item);
        }

        public void Insert(int index, T item)
        {
            CheckSealed();
            Nodes.Insert(index, item);
            SetParent(item);
        }

        public void RemoveAt(int index)
        {
            CheckSealed();
            SetParentToNull(Nodes[index]);
            Nodes.RemoveAt(index);
        }

        public T this[int index]
        {
            get
            {
                return Nodes[index];
            }
            set
            {
                CheckSealed();
                Nodes[index] = value;
                SetParent(value);
            }
        }

        #endregion

        #region ICollection<T> Members

        public bool IsReadOnly { get { return _isSealed; } }

        public void Clear()
        {
            CheckSealed();
            foreach (T node in Nodes)
            {
                SetParentToNull(node);
            }
            Nodes.Clear();
        }

        public bool Contains(T item)
        {
            return Nodes.Contains(item);
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            CheckSealed();
            Nodes.CopyTo(array, arrayIndex);
            // Should .Parent property be cleared on the copies?
        }

        public bool Remove(T item)
        {
            CheckSealed();
            SetParentToNull(item);
            return Nodes.Remove(item);
        }

        #endregion

        #region IEnumerable<T> Members

        public IEnumerator<T> GetEnumerator()
        {
            return Nodes.GetEnumerator();
        }

        #endregion

        #region IEnumerable Members

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return Nodes.GetEnumerator();
        }

        #endregion

        private List<T> Nodes
        {
            get
            {
                if (_nodes == null)
                {
                    _nodes = new List<T>();
                }
                return _nodes;
            }
        }

        private void SetParent(T node)
        {
            XamlDomItem itemNode = node as XamlDomItem;
            XamlDomMember propNode = node as XamlDomMember;

            if (itemNode != null)
            {
                itemNode.Parent = (XamlDomMember)_parentNode;
            }
            if (propNode != null)
            {
                propNode.Parent = (XamlDomObject)_parentNode;
            }
        }
        private static void SetParentToNull(T node)
        {
            XamlDomObject objNode = node as XamlDomObject;
            XamlDomMember propNode = node as XamlDomMember;

            if (objNode != null)
            {
                objNode.Parent = null;
            }
            if (propNode != null)
            {
                propNode.Parent = null;
            }
        }

        private void CheckSealed()
        {
            if (this.IsSealed)
            {
                throw new NotSupportedException("The MemberNode is read-only");     // Internal error
            }
        }
    }
}
