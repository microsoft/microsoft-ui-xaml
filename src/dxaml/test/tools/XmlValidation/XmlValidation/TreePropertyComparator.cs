// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Xml.XPath;

namespace XmlValidation
{
    // Describes a set of properties to be validated for nodes which are evaluated to be applicable.
    public class Rule
    {
        private string expression;
        private bool isWhitelist = true;
        private Dictionary<string, IStringEqualityComparer> enumeratedProperties;
        private Dictionary<string, IStringEqualityComparer> otherComparators;

        // Evaluate if given element matches XPath expression of this rule.
        public bool IsApplicable(XPathNavigator element)
        {
            return element.Matches(expression);
        }

        // See if property should be validated by this rule, and if so validate it.
        public bool TryValidateProperty(string propertyName, string referenceValue, string validatedValue, out bool result)
        {
            bool applicable;
            IStringEqualityComparer comparator;

            result = false;

            if (isWhitelist)
            {
                // If it's white-listed then check if it is in the validated collection and fetch the comparator.
                applicable = enumeratedProperties.TryGetValue(propertyName, out comparator);
            }
            else
            {
                applicable = !enumeratedProperties.ContainsKey(propertyName);

                // For black-listed, since there is no way to specify comparators, find them in Comparison node.
                otherComparators.TryGetValue(propertyName, out comparator);

                if (comparator == null)
                {
                    comparator = DefaultStringEqualityComparer.Instance;
                }
            }

            if (applicable)
            {
                result = comparator.Equals(referenceValue, validatedValue);
                return true;
            }
            else
            {
                return false;
            }
        }

        private static IStringEqualityComparer ParseComparer(XPathNavigator node)
        {
            XPathNavigator compareAsNode = node.SelectSingleNode("@CompareAs");

            if (compareAsNode != null)
            {
                switch (compareAsNode.Value)
                {
                    case "double":
                        {
                            XPathNavigator ignoreRoundoffNode = node.SelectSingleNode("@IgnoreRoundoff");
                            bool ignoreRoundoff = false;

                            if (ignoreRoundoffNode != null)
                            {
                                ignoreRoundoff = ignoreRoundoffNode.ValueAsBoolean;
                            }

                            return new DoubleEqualityComparer() { UseTolerance = ignoreRoundoff };
                        }

                    default:
                        throw new Exception("Unknown CompareAs type.");
                }
            }
            else
            {
                return DefaultStringEqualityComparer.Instance;
            }
        }

        public void Load(XPathNavigator ruleNode)
        {
            expression = ruleNode.SelectSingleNode("@Applicability").Value;

            XPathNavigator inclusionNode = ruleNode.SelectSingleNode("@Inclusion");
            isWhitelist = true;

            if (inclusionNode != null)
            {
                if (string.Compare(inclusionNode.Value, "Whitelist") == 0)
                {
                    isWhitelist = true;
                }
                else if (string.Compare(inclusionNode.Value, "Blacklist") == 0)
                {
                    isWhitelist = false;
                }
                else
                {
                    throw new Exception("Invalid value for Inclusion attribute.");
                }
            }
            else
            {
                // default is whitelist
                isWhitelist = true;
            }

            enumeratedProperties = new Dictionary<string, IStringEqualityComparer>();

            foreach (XPathNavigator propertyNode in ruleNode.Select("Property"))
            {
                string propertyName = propertyNode.SelectSingleNode("@Name").Value;
                enumeratedProperties[propertyName] = ParseComparer(propertyNode);
            }

            if (!isWhitelist)
            {
                // Allow specifying comparators for validated elements not excluded by black-list.
                otherComparators = new Dictionary<string, IStringEqualityComparer>();

                foreach (XPathNavigator comparatorNode in ruleNode.Select("Comparison"))
                {
                    string propertyName = comparatorNode.SelectSingleNode("@Name").Value;
                    otherComparators[propertyName] = ParseComparer(comparatorNode);
                }
            }
        }
    }

    public class Rules
    {
        private List<Rule> rules = new List<Rule>();

        public Rule FindApplicableRule(XPathNavigator element)
        {
            foreach (Rule rule in rules)
            {
                if (rule.IsApplicable(element))
                {
                    return rule;
                }
            }

            return null;
        }

        public void Load(XPathNavigator rulesNode)
        {
            foreach (XPathNavigator ruleNode in rulesNode.Select("Rule"))
            {
                Rule r = new Rule();
                r.Load(ruleNode);
                rules.Add(r);
            }
        }
    }

    public class TreePropertyComparator
    {
        public TreePropertyComparator()
        {
            treeChanges = new TreeModifications();
        }

        private class PropertyValues
        {
            public string referenceValue;
            public XPathNavigator referenceNode;
            public string validatedValue;
            public XPathNavigator validatedNode;
        }

        private void PopulatePropertyValueDictionary(Dictionary<string, PropertyValues> dictionary, XPathNavigator node, Func<XPathNavigator, PropertyValues, int> setter)
        {
            foreach (XPathNavigator nav in node.Select(Common.PropertyNodeName))
            {
                PropertyValues pv;
                string propertyName = nav.SelectSingleNode(Common.PropertyNameNodeName).Value;

                if (!dictionary.TryGetValue(propertyName, out pv))
                {
                    pv = new PropertyValues();
                    dictionary[propertyName] = pv;
                }
                setter(nav.SelectSingleNode(Common.PropertyValueNodeName), pv);
            }
        }

        private TreeModifications ValidateElementProperties(XPathNavigator referenceNode, XPathNavigator validatedNode)
        {
            TreeModifications result = new TreeModifications();

            // Try fast comparison first...

            if (treeComparisonContext.GetNodeInfo(referenceNode).PropertiesHash != treeComparisonContext.GetNodeInfo(validatedNode).PropertiesHash)
            {
                // If there are differences and hash comparison failed then do the full comparison.

                // Create map for faster matching by property names.

                Dictionary<string, PropertyValues> pvDictionary = new Dictionary<string, PropertyValues>();

                PopulatePropertyValueDictionary(
                    pvDictionary,
                    referenceNode,
                    (node, pv) => 
                    {
                        pv.referenceNode = node;
                        pv.referenceValue = node.Value;
                        return 0;
                    });

                PopulatePropertyValueDictionary(
                    pvDictionary,
                    validatedNode,
                    (node, pv) =>
                    {
                        pv.validatedNode = node;
                        pv.validatedValue = node.Value;
                        return 0;
                    });

                // Find rule applicable for this node.

                Rule elementRule = currentRules.FindApplicableRule(referenceNode);

                if (elementRule != null)
                {
                    foreach (string propertyName in pvDictionary.Keys)
                    {
                        bool passed = true;
                        PropertyValues pv = pvDictionary[propertyName];

                        // Validate all properties which should be validated.

                        if (elementRule.TryValidateProperty(propertyName, pv.referenceValue, pv.validatedValue, out passed))
                        {
                            if (!passed)
                            {
                                result.Add(
                                    TreeModificationInfo.CreatePropertyChange(
                                        propertyName,
                                        pv.referenceNode,
                                        pv.validatedNode));
                            }
                        }
                    }
                }
            }

            return result;
        }

        private TreeModifications CompareRecursive(XPathNavigator referenceNode, XPathNavigator validatedNode)
        {
            TreeModifications result = new TreeModifications();

            if (referenceNode == null || validatedNode == null)
            {
                return result;
            }

            TreeModificationInfo modificationInfo = null;

            if (knownTreeStructureDifferences != null)
            {
                // See if the node has been moved...

                if (knownTreeStructureDifferences.TryGetValue(referenceNode, out modificationInfo))
                {
                    if (modificationInfo.ChangeType == TreeModificationType.Move)
                    {
                        validatedNode = modificationInfo.ValidatedNode;
                    }
                }
            }

            if (!Common.CompareElementType(referenceNode, validatedNode))
            {
                result.Add(TreeModificationInfo.CreateElementTypeChange(
                    referenceNode.SelectSingleNode(Common.ElementTypeNodeName),
                    validatedNode.SelectSingleNode(Common.ElementTypeNodeName)));
            }

            result.AddRange(ValidateElementProperties(referenceNode, validatedNode));

            XPathNodeIterator referenceNodeChildren = referenceNode.Select(Common.ElementNodeName);
            XPathNodeIterator validatedNodeChildren = validatedNode.Select(Common.ElementNodeName);

            while (referenceNodeChildren.MoveNext() && validatedNodeChildren.MoveNext())
            {
                result.AddRange(CompareRecursive(referenceNodeChildren.Current, validatedNodeChildren.Current));
            }

            return result;
        }

        public bool Compare(Rules rules, TreeComparisonContext tcx)
        {
            treeComparisonContext = tcx;

            if (KnownTreeStructureDifferences != null &&
                KnownTreeStructureDifferences.Count > 0)
            {
                // Change list into mapping for faster lookups.

                knownTreeStructureDifferences = new Dictionary<XPathNavigator, TreeModificationInfo>(XPathNavigatorEqualityComparerAdapter.Instance);

                KnownTreeStructureDifferences.ForEach(
                    (tmi) =>
                    {
                        if (tmi.ReferenceNode != null)
                        {
                            knownTreeStructureDifferences[tmi.ReferenceNode] = tmi;
                        }

                        if (tmi.ValidatedNode != null)
                        {
                            knownTreeStructureDifferences[tmi.ValidatedNode] = tmi;
                        }
                    });
            }
            else
            {
                knownTreeStructureDifferences = null;
            }

            currentRules = rules;
            TreeModifications currentChanges = CompareRecursive(tcx.ReferenceNode, tcx.ValidatedNode);
            treeChanges.AddRange(currentChanges);
            return currentChanges.Count == 0;
        }

        public bool Compare(Rules rules, XPathNavigator referenceNode, XPathNavigator validatedNode)
        {
            return Compare(rules, new TreeComparisonContext(referenceNode, validatedNode));
        }

        public TreeModifications GetChanges()
        {
            return treeChanges;
        }

        // This is used to specify tree differences, so we can make best effort in trying to match moved nodes and still find property differences.
        public TreeModifications KnownTreeStructureDifferences { get; set; }

        private TreeModifications treeChanges;
        private Rules currentRules;
        private Dictionary<XPathNavigator, TreeModificationInfo> knownTreeStructureDifferences = null;
        private TreeComparisonContext treeComparisonContext;
    }
}