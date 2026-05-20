// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Xml;
using System.Xml.XPath;

namespace XmlValidation
{
    // Describes modification kind.
    public enum TreeModificationType
    {
        Insertion,
        Deletion,
        Move,
        ElementTypeChange,
        PropertyChange
    }

    // Describes modification and where it occurred in document.
    public class TreeModificationInfo
    {
        private TreeModificationInfo()
        { }

        public static TreeModificationInfo CreateMove(
            uint _referenceIndex,
            XPathNavigator _referenceNode,
            uint _validatedIndex,
            XPathNavigator _validatedNode)
        {
            TreeModificationInfo tmi = new TreeModificationInfo();
            tmi.ChangeType = TreeModificationType.Move;
            tmi.referenceIndex = _referenceIndex;
            tmi.ReferenceNode = _referenceNode;
            tmi.validatedIndex = _validatedIndex;
            tmi.ValidatedNode = _validatedNode;
            return tmi;
        }

        public static TreeModificationInfo CreateDeletion(
            uint _referenceIndex,
            XPathNavigator _referenceNode)
        {
            TreeModificationInfo tmi = new TreeModificationInfo();
            tmi.ChangeType = TreeModificationType.Deletion;
            tmi.referenceIndex = _referenceIndex;
            tmi.ReferenceNode = _referenceNode;
            return tmi;
        }

        public static TreeModificationInfo CreateInsertion(
            uint _validatedIndex,
            XPathNavigator _validatedNode)
        {
            TreeModificationInfo tmi = new TreeModificationInfo();
            tmi.ChangeType = TreeModificationType.Insertion;
            tmi.validatedIndex = _validatedIndex;
            tmi.ValidatedNode = _validatedNode;
            return tmi;
        }

        public static TreeModificationInfo CreateElementTypeChange(
            XPathNavigator _referenceNode,
            XPathNavigator _validatedNode)
        {
            TreeModificationInfo tmi = new TreeModificationInfo();
            tmi.ChangeType = TreeModificationType.ElementTypeChange;
            tmi.ReferenceNode = _referenceNode;
            tmi.ValidatedNode = _validatedNode;
            return tmi;
        }

        public static TreeModificationInfo CreatePropertyChange(
            string _propertyName,
            XPathNavigator _referenceNode,
            XPathNavigator _validatedNode)
        {
            TreeModificationInfo tmi = new TreeModificationInfo();
            tmi.ChangeType = TreeModificationType.PropertyChange;
            tmi.PropertyName = _propertyName;
            tmi.ReferenceNode = _referenceNode;
            tmi.ValidatedNode = _validatedNode;
            return tmi;
        }

        public override string ToString()
        {
            switch (ChangeType)
            {
                case TreeModificationType.Move:
                    return string.Format(
                        "Element on line {0} (master file) was moved to line {1} (output file)",
                        ReferenceLineNumber,
                        ValidatedLineNumber);

                case TreeModificationType.Deletion:
                    return string.Format(
                        "Element on line {0} (master file) was deleted",
                        ReferenceLineNumber);

                case TreeModificationType.Insertion:
                    return string.Format(
                        "Element on line {0} (output file) was inserted",
                        ValidatedLineNumber);

                case TreeModificationType.ElementTypeChange:
                    return string.Format(
                        "Element type has changed\n" +
                        "  Master (line {0}): {1}\n" +
                        "  Output (line {2}): {3}",
                        ReferenceLineNumber,
                        ReferenceValue,
                        ValidatedLineNumber,
                        ValidatedValue);

                case TreeModificationType.PropertyChange:
                    return string.Format(
                        "Value of property '{0}' is incorrect\n" +
                        "  Master (line {1}): {2}\n" +
                        "  Output (line {3}): {4}",
                        PropertyName,
                        ReferenceLineNumber,
                        ReferenceValue,
                        ValidatedLineNumber,
                        ValidatedValue);

                default:
                    throw new Exception("Unknown TreeModificationType");
            }
        }

        public TreeModificationType ChangeType { get; internal set; }
        public XPathNavigator ReferenceNode { get; internal set; }
        public uint ReferenceLineNumber
        {
            get
            {
                return (ReferenceNode != null) ? (uint)((IXmlLineInfo)ReferenceNode).LineNumber : 0;
            }
        }
        public string ReferenceValue
        {
            get
            {
                return (ReferenceNode != null) ? ReferenceNode.Value : "<< missing >>";
            }
        }
        public XPathNavigator ValidatedNode { get; internal set; }
        public uint ValidatedLineNumber
        {
            get
            {
                return (ValidatedNode != null) ? (uint)((IXmlLineInfo)ValidatedNode).LineNumber : 0;
            }
        }

        public string ValidatedValue
        {
            get
            {
                return (ValidatedNode != null) ? ValidatedNode.Value : "<< missing >>";
            }
        }

        public string PropertyName { get; internal set; }

        internal uint referenceIndex;
        internal uint validatedIndex;
    }

    public class TreeModifications : List<TreeModificationInfo>
    {
        public TreeModifications() { }
        public TreeModifications(IEnumerable<TreeModificationInfo> i) : base(i) { }
    }
}
