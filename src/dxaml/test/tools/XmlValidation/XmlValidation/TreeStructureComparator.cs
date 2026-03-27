// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.XPath;

namespace XmlValidation
{
    public class TreeStructureComparator
    {
        public TreeStructureComparator()
        {
            treeChanges = new TreeModifications();
        }

        private uint CalculateSubtreeDistance(XPathNavigator t1, XPathNavigator t2)
        {
            // This algorithms iterates over reference and validated trees comparing
            // corresponding nodes.  In case of structural differences, it will keep
            // traversing nodes in existing subtree.  For each subtree it calculates
            // a distance between subtrees which is defined as:
            // * number of subtree edits (e.g. nodes added / removed)
            // * number of node type differences in subtree (attribute Type='xxx...')
            // * number of property hash differences (e.g. any property different)

            if (t1 == null && t2 == null)
            {
                return 0;
            }

            uint distance = 0;

            if (t1 != null && t2 != null)
            {
                // Corresponding nodes exist

                XPathNodeIterator t1ChildNodes = t1.Select(Common.ElementNodeName);
                XPathNodeIterator t2ChildNodes = t2.Select(Common.ElementNodeName);

                bool t1HasMore = t1ChildNodes.MoveNext();
                bool t2HasMore = t2ChildNodes.MoveNext();

                while (t1HasMore || t2HasMore)
                {
                    XPathNavigator t1Child = (t1HasMore) ? t1ChildNodes.Current : null;
                    XPathNavigator t2Child = (t2HasMore) ? t2ChildNodes.Current : null;

                    distance += CalculateSubtreeDistance(t1Child, t2Child);

                    t1HasMore = t1ChildNodes.MoveNext();
                    t2HasMore = t2ChildNodes.MoveNext();
                }

                NodeInfo ni1 = treeComparisonContext.GetNodeInfo(t1);
                NodeInfo ni2 = treeComparisonContext.GetNodeInfo(t2);

                // distance += (Common.CompareElementType(t1, t2)) ? 0U : 1U;
                distance += (ni1.TypeHash == ni2.TypeHash) ? 0U : 1U;
                distance += (ni1.PropertiesHash == ni2.PropertiesHash) ? 0U : 1U;
            }
            else if (t1 != null)
            {
                // Only left node

                ++distance;

                // iterate over children
                XPathNodeIterator t1ChildNodes = t1.Select(Common.ElementNodeName);

                while (t1ChildNodes.MoveNext())
                {
                    distance += CalculateSubtreeDistance(t1ChildNodes.Current, null);
                }
            }
            else
            {
                // Only right node

                ++distance;

                // iterate over children
                XPathNodeIterator t2ChildNodes = t2.Select(Common.ElementNodeName);

                while (t2ChildNodes.MoveNext())
                {
                    distance += CalculateSubtreeDistance(null, t2ChildNodes.Current);
                }
            }

            return distance;
        }

        private class ChildNodeEntry
        {
            public ChildNodeEntry(XPathNavigator _node, uint _position)
            {
                node = _node;
                position = _position;
            }
            public XPathNavigator node;
            public List<Tuple<uint, ChildNodeEntry>> distances = new List<Tuple<uint, ChildNodeEntry>>();
            public uint position = UInt32.MaxValue;
        }

        private class ComparisonStats
        {
            public TreeModifications elementMoves = new TreeModifications();
            public TreeModifications elementInserts = new TreeModifications();
            public TreeModifications elementDeletes = new TreeModifications();

            public void Add(ComparisonStats other)
            {
                elementMoves.AddRange(other.elementMoves);
                elementInserts.AddRange(other.elementInserts);
                elementDeletes.AddRange(other.elementDeletes);
            }
        }

        // Main method for comparing subtrees.

        private ComparisonStats CompareSubtreesRecursive(XPathNavigator referenceNode, XPathNavigator validatedNode)
        {
            // Differences between trees

            ComparisonStats result = new ComparisonStats();

            if (referenceNode == null && validatedNode == null)
            {
                return result;
            }
            else if (referenceNode != null && validatedNode == null)
            {
                // Reference node was deleted

                result.elementDeletes.Add(
                    TreeModificationInfo.CreateDeletion(
                        0,
                        referenceNode));
                return result;
            }
            else if (referenceNode == null && validatedNode != null)
            {
                // A new node in validated tree

                result.elementInserts.Add(
                    TreeModificationInfo.CreateInsertion(
                        0,
                        validatedNode));
                return result;
            }

            Queue<ChildNodeEntry> referenceEntries = new Queue<ChildNodeEntry>();
            List<ChildNodeEntry> validatedEntries = new List<ChildNodeEntry>();
            List<ChildNodeEntry> referenceMismatches = new List<ChildNodeEntry>();

            // Create structures holding child nodes for this element and augment them with position information

            uint childIndex = 0;

            foreach (XPathNavigator childElement in referenceNode.Select(Common.ElementNodeName))
            {
                referenceEntries.Enqueue(new ChildNodeEntry(childElement, childIndex++));
            }

            childIndex = 0;

            foreach (XPathNavigator childElement in validatedNode.Select(Common.ElementNodeName))
            {
                validatedEntries.Add(new ChildNodeEntry(childElement, childIndex++));
            }

            while (referenceEntries.Count > 0)
            {
                ChildNodeEntry reference = referenceEntries.Dequeue();
                ChildNodeEntry perfectMatch = null;

                // For current reference element, try to find a match

                foreach (ChildNodeEntry validated in validatedEntries)
                {
                    uint currentDistance = CalculateSubtreeDistance(reference.node, validated.node);

                    if (currentDistance == 0)
                    {
                        // Perfect match, therefore nothing to do here.

                        perfectMatch = validated;
                        break;
                    }
                    else
                    {
                        // Wasn't a perfect match.  Retain the distance to this node, so later we can
                        // go back to find the best, non-perfect match.

                        reference.distances.Add(Tuple.Create(currentDistance, validated));
                    }
                }

                if (perfectMatch != null)
                {
                    // If this was a perfect match, it still could have been moved.
                    // If there was no move, it will be pruned later.

                    result.elementMoves.Add(
                        TreeModificationInfo.CreateMove(
                            reference.position,
                            reference.node,
                            perfectMatch.position,
                            perfectMatch.node));

                    // Don't consider this node for future matches.

                    validatedEntries.Remove(perfectMatch);
                }
                else
                {
                    // Add to mismatches, we will revisit these after all perfect matches are found.

                    referenceMismatches.Add(reference);
                }
            }

            // Process mismatches

            foreach (ChildNodeEntry reference in referenceMismatches)
            {
                ChildNodeEntry bestExistingMatch = null;

                // Sort distances, starting with the closest.

                reference.distances.Sort((v1, v2) => { return Comparer<double>.Default.Compare(v1.Item1, v2.Item1); });

                foreach (var bestMatch in reference.distances)
                {
                    // The best match might have been a perfect match for some other node, so need to check if it still is available.

                    if (validatedEntries.Contains(bestMatch.Item2))
                    {
                        // Descend into this subtree to see what changes are between the two.

                        ComparisonStats subResults = CompareSubtreesRecursive(reference.node, bestMatch.Item2.node);

                        // Heuristic used to assess if operation was a move or replaced by a different subtree:
                        // If the number of inserts and deletes is less than 50% of size of the tree, then it was a move.

                        uint referenceTreeSize = (uint)treeComparisonContext.GetNodeInfo(reference.node).SubtreeSize + 1;
                        double referenceTreeSimilarity = (double)(referenceTreeSize - subResults.elementDeletes.Count - subResults.elementInserts.Count) / referenceTreeSize;

                        if (referenceTreeSimilarity >= 0.5)
                        {
                            // It's similar enough - it was a move, not delete + insert

                            bestExistingMatch = bestMatch.Item2;
                            result.Add(subResults);
                            validatedEntries.Remove(bestMatch.Item2);
                        }
                        break;
                    }
                }

                if (bestExistingMatch == null)
                {
                    // There was no good enough match for this node, therefore it was deleted.

                    result.elementDeletes.Add(
                        TreeModificationInfo.CreateDeletion(
                            reference.position,
                            reference.node));
                }
                else
                {
                    // It was a move

                    result.elementMoves.Add(
                        TreeModificationInfo.CreateMove(
                            reference.position,
                            reference.node,
                            bestExistingMatch.position,
                            bestExistingMatch.node));
                }
            }

            // Whatever was left in validated subtree was an insert of a new node.

            foreach (ChildNodeEntry validated in validatedEntries)
            {
                result.elementInserts.Add(
                    TreeModificationInfo.CreateInsertion(
                        validated.position,
                        validated.node));
            }

            // Since we created moves for all matches, now we need to prune them after
            // adjusting for inserts and deletes.

            result.elementInserts.Sort((v1, v2) => { return Comparer<double>.Default.Compare(v2.validatedIndex, v1.validatedIndex); });

            foreach (TreeModificationInfo tmi in result.elementInserts)
            {
                foreach (TreeModificationInfo move in result.elementMoves)
                {
                    if (tmi.validatedIndex < move.validatedIndex)
                    {
                        --move.validatedIndex;
                    }
                }
            }

            result.elementDeletes.Sort((v1, v2) => { return Comparer<double>.Default.Compare(v2.referenceIndex, v1.referenceIndex); });

            foreach (TreeModificationInfo tmi in result.elementDeletes)
            {
                foreach (TreeModificationInfo move in result.elementMoves)
                {
                    if (tmi.referenceIndex < move.referenceIndex)
                    {
                        --move.referenceIndex;
                    }
                }
            }

            // Moves are actual moves only if the position changes.

            result.elementMoves = new TreeModifications(result.elementMoves.Where((t) => t.referenceIndex != t.validatedIndex));

            return result;
        }

        public bool Compare(TreeComparisonContext tcc)
        {
            treeComparisonContext = tcc;

            ComparisonStats result = CompareSubtreesRecursive(tcc.ReferenceNode, tcc.ValidatedNode);

            treeChanges.AddRange(result.elementDeletes);
            treeChanges.AddRange(result.elementInserts);
            treeChanges.AddRange(result.elementMoves);

            // Sort output according to line number.

            treeChanges.Sort((v1, v2) =>
            {
                return Comparer<uint>.Default.Compare(
                    (v1.ReferenceLineNumber != 0) ? v1.ReferenceLineNumber : v1.ValidatedLineNumber,
                    (v2.ReferenceLineNumber != 0) ? v2.ReferenceLineNumber : v2.ValidatedLineNumber);
            });

            return (result.elementDeletes.Count == 0 && result.elementInserts.Count == 0 && result.elementMoves.Count == 0);
        }

        public bool Compare(XPathNavigator referenceNode, XPathNavigator validatedNode)
        {
            return Compare(new TreeComparisonContext(referenceNode, validatedNode));
        }

        public bool Compare(string referenceTreeXML, string validatedTreeXML)
        {
            XPathDocument referenceDoc;
            XPathDocument validatedDoc;

            using (TextReader referenceTreeXMLReader = new StringReader(referenceTreeXML))
            {
                referenceDoc = new XPathDocument(referenceTreeXMLReader);
            }

            using (TextReader validatedTreeXMLReader = new StringReader(validatedTreeXML))
            {
                validatedDoc = new XPathDocument(validatedTreeXMLReader);
            }

            return Compare(
                referenceDoc.CreateNavigator().SelectSingleNode("/"),
                validatedDoc.CreateNavigator().SelectSingleNode("/"));
        }

        public TreeModifications GetChanges(bool purgeEquivalents = true)
        {
            if (purgeEquivalents)
            {
                return new TreeModifications(
                    treeChanges.Where((x) => 
                    {
                        return (x.ChangeType == TreeModificationType.Move && x.ReferenceLineNumber > x.ValidatedLineNumber) == false;
                    }));
            }
            else
            {
                return treeChanges;
            }
        }

        private TreeModifications treeChanges;
        private TreeComparisonContext treeComparisonContext;
    }
}