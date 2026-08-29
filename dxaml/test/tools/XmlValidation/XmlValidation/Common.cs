// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Xml.XPath;

namespace XmlValidation
{
    public class DefaultInstance<T>
    {
        public static T Instance
        {
            get
            {
                if (instance == null)
                {
                    instance = Activator.CreateInstance<T>();
                }

                return instance;
            }
        }

        private static T instance;
    }

    // Adapter for using XPathNavigators as keys in dictionaries
    public class XPathNavigatorEqualityComparerAdapter : DefaultInstance<XPathNavigatorEqualityComparerAdapter>,
                                                         IEqualityComparer<XPathNavigator>
    {
        public bool Equals(XPathNavigator x, XPathNavigator y)
        {
            return XPathNavigator.NavigatorComparer.Equals(x, y);
        }

        public int GetHashCode(XPathNavigator obj)
        {
            return XPathNavigator.NavigatorComparer.GetHashCode(obj);
        }
    }

    public static class Common
    {
        public static bool CompareElementType(XPathNavigator referenceNode, XPathNavigator validatedNode)
        {
            XPathNavigator referenceTypeNode = referenceNode.SelectSingleNode(Common.ElementTypeNodeName);
            XPathNavigator validatedTypeNode = validatedNode.SelectSingleNode(Common.ElementTypeNodeName);

            if (referenceTypeNode != null && validatedTypeNode != null)
            {
                return (string.Compare(referenceTypeNode.Value, validatedTypeNode.Value) == 0);
            }
            else if (referenceTypeNode == null && validatedTypeNode == null)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        // XML visual tree dump constants
        public const string RootNodeName = "VisualTreeDump";

        public const string VisualRootNodeName = "VisualRoot";
        public const string PopupRootNodeName = "PopupRoot";

        public const string ElementNodeName = "Element";
        public const string ElementTypeNodeName = "@Type";

        public const string PropertyNodeName = "Property";
        public const string PropertyNameNodeName = "@Name";
        public const string PropertyValueNodeName = "@Value";
    }
}