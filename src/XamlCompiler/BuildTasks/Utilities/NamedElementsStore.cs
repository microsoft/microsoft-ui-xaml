// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Diagnostics;

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    using XamlDom;

    internal class NamedElementsStore
    {
        private Stack<HashSet<string>> _scopeStack = new Stack<HashSet<string>>();

        internal NamedElementsStore()
        {
            _scopeStack.Push(new HashSet<string>());
        }

        internal void EnterNewScope(XamlDomObject member)
        {
            _scopeStack.Push(new HashSet<string>());
        }

        internal void ExitCurrentScope()
        {
            _scopeStack.Pop();
        }

        internal void AddNamedElement(string name)
        {
            _scopeStack.Peek().Add(name);
        }

        internal bool IsNameAlreadyUsed(string name)
        {
            Debug.Assert(_scopeStack.Count > 0);
            return _scopeStack.Peek().Contains(name);
        }
    }
}
