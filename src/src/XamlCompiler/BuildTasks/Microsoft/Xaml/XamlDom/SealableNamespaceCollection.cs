// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel; //this is only used for _baseDictionary.Contains().

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using Properties;
    using Utilities;

    internal class SealableNamespaceCollection : KeyedCollection<string, XamlDomNamespace>
    {
        public void Seal()
        {
            _isSealed = true;

            foreach (XamlDomNamespace xdns in this)
            {
                xdns.Seal();
            }
        }

        public bool IsSealed { get { return _isSealed; } }

        protected override String GetKeyForItem(XamlDomNamespace item)
        {
            return item.NamespaceDeclaration.Prefix;
        }

        protected override void InsertItem(int index, XamlDomNamespace item)
        {
            CheckSealed();
            base.InsertItem(index, item);
        }

        protected override void RemoveItem(int index)
        {
            CheckSealed();
            base.RemoveItem(index);
        }

        protected override void SetItem(int index, XamlDomNamespace item)
        {
            CheckSealed();
            base.SetItem(index, item);
        }

        protected override void ClearItems()
        {
            CheckSealed();
            base.ClearItems();
        }

        private void CheckSealed()
        {
            if (IsSealed)
            {
                throw new NotSupportedException(
                    ResourceUtilities.FormatString(XamlCompilerResources.XamlDom_SealedNamespaceCollection));
            }
        }
        private bool _isSealed;
    }
}
