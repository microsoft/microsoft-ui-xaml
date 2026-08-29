// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using DirectUI;

    internal delegate void XamlDomIteratorEnterNewScopeEvent(XamlDomObject obj);
    internal delegate void XamlDomIteratorExitNewScopeEvent();

    internal class XamlDomIterator
    {
        private Stack<XamlDomMember> _members = new Stack<XamlDomMember>();
        private Stack<int> _scopes = new Stack<int>();
        private XamlDomObject _owner;

        internal event XamlDomIteratorEnterNewScopeEvent EnterNewScopeCallback;
        internal event XamlDomIteratorExitNewScopeEvent ExitScopeCallback;

        internal XamlDomIterator(XamlDomObject owner)
        {
            _owner = owner;
        }

        internal virtual IEnumerable<XamlDomObject> DescendantsAndSelf()
        {
            return this.DescendantsAndSelf((XamlType)null);
        }

        internal virtual IEnumerable<XamlDomObject> DescendantsAndSelf(XamlType type)
        {
            for (int i = _owner.MemberNodes.Count - 1; i >= 0; i--)
            {
                _members.Push(_owner.MemberNodes[i]);
            }

            if (IsObjectNodeAssignable(type, _owner))
            {
                yield return _owner;
            }

            while (_members.Count > 0)
            {
                this.ShouldNotifyNamingScopeExit();

                XamlDomMember member = _members.Pop();

                this.ShouldNotifyNamingScopeEnter(member);
                
                foreach (XamlDomItem itemNode in member.Items)
                {
                    XamlDomObject objectNode = itemNode as XamlDomObject;
                    if (objectNode != null)
                    {
                        for (int i = objectNode.MemberNodes.Count - 1; i >= 0; i--)
                        {
                            _members.Push(objectNode.MemberNodes[i]);
                        }

                        if (IsObjectNodeAssignable(type, objectNode))
                        {
                            yield return objectNode;
                        }
                    }
                }
            }

            this.ShouldNotifyNamingScopeExit();
        }

        private void ShouldNotifyNamingScopeEnter(XamlDomMember member)
        {
            if (this.EnterNewScopeCallback != null)
            {
                var duiXM = member.Member as DirectUIXamlMember;
                if (duiXM!= null && duiXM.IsTemplate)
                {
                    _scopes.Push(_members.Count);
                    this.EnterNewScopeCallback(member.Parent);
                }
            }
        }

        // Check to see if the Object Node is a match for the "Type" given.
        // this is used to filter the DescendantsAndSelf() call.
        //
        private static bool IsObjectNodeAssignable(XamlType type, XamlDomObject objectNode)
        {
            // If no type is given, then all types are good.
            if (type == null)
            {
                return true;
            }

            if (!objectNode.IsGetObject)
            {
                // if it is a Start Object then compare against the object node type.
                if (objectNode.Type.CanAssignTo(type))
                {
                    return true;
                }
            }
            else
            {
                // if it is a Get Object then check the parent property node type (if any)
                if (objectNode.Parent != null && objectNode.Parent.Member.Type.CanAssignTo(type))
                {
                    return true;
                }
            }
            return false;
        }

        private void ShouldNotifyNamingScopeExit()
        {
            if (this.ExitScopeCallback != null)
            {
                while (_scopes.Count > 0 && _scopes.Peek() == _members.Count)
                {
                    _scopes.Pop();
                    this.ExitScopeCallback();
                }
            }
        }

    }
}
