// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Xaml;
using System.Xml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal class SaveStateXamlMember
    {
        private const string XMLNAME_Name = "Name";
        private const string XMLNAME_DeclaringTypeFullName = "DeclaringTypeFullName";

        public string Name { get; set; }
        public string DeclaringTypeFullName { get; set; }

        public override string ToString()
        {
            return String.Format("{0}.{1}", DeclaringTypeFullName, Name);
        }

        public SaveStateXamlMember(BindAssignment bindAssignment)
        {
            Name = bindAssignment.MemberName;
            DeclaringTypeFullName = bindAssignment.MemberDeclaringType.UnderlyingType.FullName;
        }

        public SaveStateXamlMember(XmlNode node)
        {
            XmlNode attribute;

            attribute = node.Attributes.GetNamedItem(XMLNAME_Name);
            if (attribute != null)
            {
                Name = attribute.Value;
            }

            attribute = node.Attributes.GetNamedItem(XMLNAME_DeclaringTypeFullName);
            if (attribute != null)
            {
                DeclaringTypeFullName = attribute.Value;
            }
        }

        public void WriteXmlElement(XmlWriter writer, string elementName)
        {
            writer.WriteStartElement(elementName);
            writer.WriteAttributeString(XMLNAME_Name, Name);
            writer.WriteAttributeString(XMLNAME_DeclaringTypeFullName, DeclaringTypeFullName);
            writer.WriteEndElement();
        }
    }

    internal class SaveStateXamlType
    {
        private const string XMLNAME_FullName = "FullName";

        public string FullName { get; set; }

        public override string ToString()
        {
            return FullName;
        }

        public SaveStateXamlType(XamlType type)
        {
            FullName = type.UnderlyingType.FullName;
        }

        public SaveStateXamlType(XmlNode node)
        {
            XmlNode attribute;

            attribute = node.Attributes.GetNamedItem(XMLNAME_FullName);
            if (attribute != null)
            {
                FullName = attribute.Value;
            }
        }

        public void WriteXmlElement(XmlWriter writer, string elementName)
        {
            writer.WriteStartElement(elementName);
            writer.WriteAttributeString(XMLNAME_FullName, FullName);
            writer.WriteEndElement();
        }
    }

    internal class SaveStatePerXamlFile
    {
        private const string XMLNAME_XamlFileName = "XamlFileName";
        private const string XMLNAME_XamlFileTimeAtLastCompileInTicks = "XamlFileTimeAtLastCompileInTicks";
        private const string XMLNAME_ClassFullName = "ClassFullName";
        private const string XMLNAME_GeneratedCodePathPrefix = "GeneratedCodePathPrefix";
        private const string XMLNAME_BindingObservableVectorTypes = "BindingObservableVectorTypes";
        private const string XMLNAME_BindingObservableMapTypes = "BindingObservableMapTypes";
        private const string XMLNAME_XamlType = "XamlType";
        private const string XMLNAME_BindingSetters = "BindingSetters";
        private const string XMLNAME_Member = "XamlMember";
        private const string XMLNAME_HasBoundEventAssignments = "HasBoundEventAssignments";

        public string FileName { get; private set; }
        public long XamlFileTimeAtLastCompile { get; set; }
        public string ClassFullName { get; set; }

        public Dictionary<string, SaveStateXamlMember> BindingSetters { get; set; }
        public Dictionary<string, SaveStateXamlType> BindingObservableVectorTypes { get; set; }
        public Dictionary<string, SaveStateXamlType> BindingObservableMapTypes { get; set; }
        public bool HasBoundEventAssignments { get; set; }
        public string GeneratedCodeFilePathPrefix { get; set; }

        private SaveStatePerXamlFile()
        {
            BindingSetters = new Dictionary<string, SaveStateXamlMember>();
            BindingObservableVectorTypes = new Dictionary<string, SaveStateXamlType>();
            BindingObservableMapTypes = new Dictionary<string, SaveStateXamlType>();
        }

        public SaveStatePerXamlFile(String fileName)
            : this()
        {
            FileName = fileName;
        }

        public SaveStatePerXamlFile(XmlNode node)
            : this()
        {
            XmlNode attribute;

            attribute = node.Attributes.GetNamedItem(XMLNAME_XamlFileName);
            if (attribute != null)
            {
                FileName = attribute.Value;
            }

            attribute = node.Attributes.GetNamedItem(XMLNAME_XamlFileTimeAtLastCompileInTicks);
            if (attribute != null)
            {
                Int64 time = 0;
                if (Int64.TryParse(attribute.Value, out time))
                {
                    XamlFileTimeAtLastCompile = time;
                }
                else
                {
                    XamlFileTimeAtLastCompile = 0;
                }
            }

            attribute = node.Attributes.GetNamedItem(XMLNAME_ClassFullName);
            if (attribute != null)
            {
                ClassFullName = attribute.Value;
            }

            attribute = node.Attributes.GetNamedItem(XMLNAME_GeneratedCodePathPrefix);
            if (attribute != null)
            {
                GeneratedCodeFilePathPrefix = attribute.Value;
            }
            
            attribute = node.Attributes.GetNamedItem(XMLNAME_HasBoundEventAssignments);
            if (attribute != null)
            {
                bool hasBoundEventAssignments = false;
                if (Boolean.TryParse(attribute.Value, out hasBoundEventAssignments))
                {
                    HasBoundEventAssignments = hasBoundEventAssignments;
                }
                else
                {
                    HasBoundEventAssignments = false;
                }
            }

            foreach (XmlNode child in node.ChildNodes)
            {
                switch (child.Name)
                {
                    case XMLNAME_BindingObservableVectorTypes:
                        foreach (XmlNode subNode in child.ChildNodes)
                        {
                            SaveStateXamlType obj = new SaveStateXamlType(subNode);
                            BindingObservableVectorTypes.Add(obj.ToString(), obj);
                        }
                        break;
                    case XMLNAME_BindingObservableMapTypes:
                        foreach (XmlNode subNode in child.ChildNodes)
                        {
                            SaveStateXamlType obj = new SaveStateXamlType(subNode);
                            BindingObservableMapTypes.Add(obj.ToString(), obj);
                        }
                        break;
                    case XMLNAME_BindingSetters:
                        foreach (XmlNode subNode in child.ChildNodes)
                        {
                            SaveStateXamlMember obj = new SaveStateXamlMember(subNode);
                            BindingSetters.Add(obj.ToString(), obj);
                        }
                        break;
                    default:
                        Debug.Assert(false, String.Format("Unknown node '{0}'", child.Name));
                        break;
                }
            }
        }

        public void WriteXmlElement(XmlWriter writer, string elementName)
        {
            writer.WriteStartElement(elementName);
            writer.WriteAttributeString(XMLNAME_XamlFileName, FileName);
            writer.WriteAttributeString(XMLNAME_ClassFullName, ClassFullName);
            writer.WriteAttributeString(XMLNAME_GeneratedCodePathPrefix, GeneratedCodeFilePathPrefix);
            writer.WriteAttributeString(XMLNAME_XamlFileTimeAtLastCompileInTicks, XamlFileTimeAtLastCompile.ToString());
            writer.WriteAttributeString(XMLNAME_HasBoundEventAssignments, HasBoundEventAssignments.ToString());

            // Write: BindingObservableVectorTypes
            if (BindingObservableVectorTypes.Values.Count > 0)
            {
                writer.WriteStartElement(XMLNAME_BindingObservableVectorTypes);
                foreach (SaveStateXamlType type in BindingObservableVectorTypes.Values)
                {
                    type.WriteXmlElement(writer, XMLNAME_XamlType);
                }
                writer.WriteEndElement();
            }

            // Write: BindingObservableMapTypes
            if (BindingObservableMapTypes.Values.Count > 0)
            {
                writer.WriteStartElement(XMLNAME_BindingObservableMapTypes);
                foreach (SaveStateXamlType type in BindingObservableMapTypes.Values)
                {
                    type.WriteXmlElement(writer, XMLNAME_XamlType);
                }
                writer.WriteEndElement();
            }

            // Write: BindingSetters
            if (BindingSetters.Values.Count > 0)
            {
                writer.WriteStartElement(XMLNAME_BindingSetters);
                foreach (SaveStateXamlMember setter in BindingSetters.Values)
                {
                    setter.WriteXmlElement(writer, XMLNAME_Member);
                }
                writer.WriteEndElement();
            }
            writer.WriteEndElement();
        }
    }
}
