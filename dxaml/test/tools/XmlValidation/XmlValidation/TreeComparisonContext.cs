// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;
using System.Text;
using System.Xml.XPath;

namespace XmlValidation
{
    public class NodeInfo
    {
        // Holds number of elements in this subtree (including self)
        public uint SubtreeSize     { get; internal set; }

        // Hash of property key-name pairs for quick comparison
        public int PropertiesHash   { get; internal set; }

        // Hash of XAML type
        public int TypeHash         { get; internal set; }
    }

    // Stores precalculated information shared by tree structure and property comparators.
    public class TreeComparisonContext
    {
        public TreeComparisonContext(XPathNavigator referenceNode, XPathNavigator validatedNode)
        {
            ReferenceNode = referenceNode;
            ValidatedNode = validatedNode;

            // Cache NodeInfos by node path
            nodeInfos = new Dictionary<XPathNavigator, NodeInfo>(XPathNavigatorEqualityComparerAdapter.Instance);

            if (referenceNode != null)
            {
                CacheNodeInfo(referenceNode);
            }

            if (validatedNode != null)
            {
                CacheNodeInfo(validatedNode);
            }
        }

        public NodeInfo GetNodeInfo(XPathNavigator node)
        {
            return nodeInfos[node];
        }

        private NodeInfo CacheNodeInfo(XPathNavigator node)
        {
            XPathNavigator typeNav = node.SelectSingleNode(Common.ElementTypeNodeName);

            NodeInfo nodeInfo = new NodeInfo()
            {
                SubtreeSize = 1,
                PropertiesHash = CalculatePropertiesHash(node),
                TypeHash = (typeNav != null) ? typeNav.Value.GetHashCode() : 0
            };

            foreach (XPathNavigator child in node.Select(Common.ElementNodeName))
            {
                NodeInfo childNodeInfo = CacheNodeInfo(child);
                nodeInfo.SubtreeSize += childNodeInfo.SubtreeSize;
            }

            nodeInfos[node] = nodeInfo;

            return nodeInfo;
        }

        private int CalculatePropertiesHash(XPathNavigator node)
        {
            StringBuilder sb = new StringBuilder();

            // Assumes properties are in the same order.
            foreach (XPathNavigator nav in node.Select(Common.PropertyNodeName))
            {
                sb.Append(nav.SelectSingleNode(Common.PropertyNameNodeName).Value);
                sb.Append(nav.SelectSingleNode(Common.PropertyValueNodeName).Value);
            }

            return sb.ToString().GetHashCode();
        }

        public XPathNavigator ReferenceNode { get; internal set; }
        public XPathNavigator ValidatedNode { get; internal set; }

        private Dictionary<XPathNavigator, NodeInfo> nodeInfos;
    }
}