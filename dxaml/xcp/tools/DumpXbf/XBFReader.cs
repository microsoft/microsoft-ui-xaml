// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using DumpXBF.FileFormat.Metadata;
using DumpXBF.FileFormat.NodeStream;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DumpXBF
{
    class XBFReader
    {
        public XBFReader(string filePath, bool verbose)
        {
            FileInfo f = new FileInfo(filePath);
            br = new BinaryReader(f.OpenRead(), Encoding.Unicode);
            if (!ReadHeader())
            {
                Console.WriteLine("ERROR: Invalid Header!");
                return;
            }

            if (!ReadTables())
            {
                return;
            }

            if (!ReadNodes())
            {
                return;
            }

            verboseMode = verbose;
        }

        public void Dump()
        {
            if (verboseMode)
            {
                DumpHeader();
                Console.WriteLine();
                DumpStringTable();
                Console.WriteLine();
                DumpAssemblyTable();
                Console.WriteLine();
                DumpTypeNamespaceTable();
                Console.WriteLine();
                DumpTypeTable();
                Console.WriteLine();
                DumpPropertyTable();
                Console.WriteLine();
                DumpXmlNamespaceTable();
                Console.WriteLine();
            }

            DumpNodes();
        }

        private bool ReadTables()
        {
            if (!ReadStringTable())
            {
                Console.WriteLine("ERROR: Failed to read string table!");
                return false;
            }

            if (!ReadAssemblyTable())
            {
                Console.WriteLine("ERROR: Failed to read assembly table!");
                return false;
            }

            if (!ReadTypeNamespaceTable())
            {
                Console.WriteLine("ERROR: Failed to read type namespace table!");
                return false;
            }

            if (!ReadTypeTable())
            {
                Console.WriteLine("ERROR: Failed to read type table!");
                return false;
            }

            if (!ReadPropertyTable())
            {
                Console.WriteLine("ERROR: Failed to read property table!");
                return false;
            }

            if (!ReadXmlNamespaceTable())
            {
                Console.WriteLine("ERROR: Failed to read xml namespace table!");
                return false;
            }

            return true;
        }

        private bool ReadNodes()
        {
            Stack<XamlObject> typeStack = new Stack<XamlObject>();
            Stack<XamlProperty> propertyStack = new Stack<XamlProperty>();
            uint currentLinePosition = 0;
            uint currentLineNumber   = 0;

            while (br.BaseStream.Length != br.BaseStream.Position)
            {
                XamlNodeType nodeType = (XamlNodeType)br.ReadByte();
                XamlLineInfo lineInfo = new XamlLineInfo();
                XamlNodeInfo nodeInfo = new XamlNodeInfo();

                switch (nodeType)
                {
                    case XamlNodeType.xntLineInfo:
                        lineInfo = ReadLineInfo(currentLineNumber, currentLinePosition);
                        break;

                    case XamlNodeType.xntLineInfoAbsolute:
                        lineInfo = ReadLineInfoAbsolute();
                        break;

                    case XamlNodeType.xntNamespace:
                        XamlNamespaceInfo namespaceInfo = ReadXamlNamespaceNode();
                        break;

                    case XamlNodeType.xntStartObject:
                        nodeInfo = ReadXamlNode();
                        XamlObject xamlObject = new XamlObject(nodeInfo);
                        xamlObject.lineInfo = lineInfo;
                        if (xamlTreeRoot == null)
                        {
                            xamlTreeRoot = xamlObject;
                        }
                        typeStack.Push(xamlObject);
                        break;

                    case XamlNodeType.xntEndObject:
                        if (propertyStack.Count > 0)
                        {
                            XamlProperty parentProperty = propertyStack.Peek();
                            parentProperty.propertyValue.Add(typeStack.Peek());
                        }

                        typeStack.Pop();
                        break;

                    case XamlNodeType.xntStartProperty:
                        nodeInfo = ReadXamlNode();
                        XamlProperty xamlProperty = new XamlProperty(nodeInfo);
                        xamlProperty.lineInfo = lineInfo;
                        propertyStack.Push(xamlProperty);
                        break;

                    case XamlNodeType.xntEndProperty:
                        if (typeStack.Count > 0)
                        {
                            XamlObject parentObject = typeStack.Peek();
                            parentObject.properties.Add(propertyStack.Peek());
                        }
                        propertyStack.Pop();
                        break;

                    case XamlNodeType.xntValue:
                    case XamlNodeType.xntText:
                        XamlNode xamlTextValue = null;

                        if (nodeType == XamlNodeType.xntText)
                        {
                            nodeInfo = ReadXamlNode();
                            xamlTextValue = new XamlTextValue(nodeInfo);
                        }
                        else
                        {
                            xamlTextValue = ReadXamlValueNode();
                        }

                        if (propertyStack.Count > 0)
                        {
                            XamlProperty parentProperty = propertyStack.Peek();
                            parentProperty.propertyValue.Add(xamlTextValue);
                        }
                        break;
                }
            }

            return true;
        }

        private bool ReadHeader()
        {
            Header xbfHeader = new Header();
            xbfHeader.magicNumber = br.ReadBytes(4);

            if (xbfHeader.magicNumber[0] != 0x58 && xbfHeader.magicNumber[1] != 0x42 && xbfHeader.magicNumber[2] != 0x46 && xbfHeader.magicNumber[3] == 0x00)
            {
                Console.WriteLine("ERROR: Invalid File Format Magic Number!");
                return false;
            }

            xbfHeader.metadataSize = br.ReadUInt32();
            xbfHeader.nodeSize = br.ReadUInt32();
            xbfHeader.majorFileVersion = br.ReadUInt32();
            xbfHeader.minorFileVersion = br.ReadUInt32();
            xbfHeader.stringTableOffset = br.ReadUInt64();
            xbfHeader.assemblyTableOffset = br.ReadUInt64();
            xbfHeader.typeNamespaceTableOffset = br.ReadUInt64();
            xbfHeader.typeTableOffset = br.ReadUInt64();
            xbfHeader.propertyTableOffset = br.ReadUInt64();
            xbfHeader.xmlNamespaceTableOffset = br.ReadUInt64();
            xbfHeader.hash = br.ReadChars(32);

            fileHeader = xbfHeader;
            return true;
        }
        
        private bool ReadStringTable()
        {
            StringTable table = new StringTable();
            table.tableCount = br.ReadUInt32();
            table.stringEntry = new string[table.tableCount];

            for (int i = 0; i < table.tableCount; i++)
            {
                uint length = br.ReadUInt32();
                char[] value = br.ReadChars((int)length);
                table.stringEntry[i] = new string(value);
            }

            stringTable = table;
            return true;
        }

        private bool ReadAssemblyTable()
        {
            AssemblyTable table = new AssemblyTable();
            table.tableCount = br.ReadUInt32();
            table.assemblyEntry = new AssemblyEntry[table.tableCount];

            for (int i = 0; i < table.tableCount; i++)
            {
                table.assemblyEntry[i].providerKind = (XamlTypeInfoProviderKind)br.ReadUInt32();
                table.assemblyEntry[i].stringId = br.ReadUInt32();
            }

            assemblyTable = table;
            return true;
        }

        private bool ReadTypeNamespaceTable()
        {
            TypeNamespaceTable table = new TypeNamespaceTable();
            table.tableCount = br.ReadUInt32();
            table.typeNamespaceEntry = new TypeNamespaceEntry[table.tableCount];

            for (int i = 0; i < table.tableCount; i++)
            {
                table.typeNamespaceEntry[i].assemblyId = br.ReadUInt32();
                table.typeNamespaceEntry[i].stringId = br.ReadUInt32();
            }

            typeNamespaceTable = table;
            return true;
        }

        private bool ReadTypeTable()
        {
            TypeTable table = new TypeTable();
            table.tableCount = br.ReadUInt32();
            table.typeEntry = new TypeEntry[table.tableCount];

            for (int i = 0; i < table.tableCount; i++)
            {
                table.typeEntry[i].typeFlags = (PersistedXamlTypeFlags)br.ReadUInt32();
                table.typeEntry[i].typeNamespaceId = br.ReadUInt32();
                table.typeEntry[i].stringId = br.ReadUInt32();
            }

            typeTable = table;
            return true;
        }

        private bool ReadPropertyTable()
        {
            PropertyTable table = new PropertyTable();
            table.tableCount = br.ReadUInt32();
            table.propertyEntry = new PropertyEntry[table.tableCount];

            for (int i = 0; i < table.tableCount; i++)
            {
                table.propertyEntry[i].propertyFlags = (PersistedXamlPropertyFlags)br.ReadUInt32();
                table.propertyEntry[i].typeId = br.ReadUInt32();
                table.propertyEntry[i].stringId = br.ReadUInt32();
            }

            propertyTable = table;
            return true;
        }

        private bool ReadXmlNamespaceTable()
        {
            XmlNamespaceTable table = new XmlNamespaceTable();
            table.tableCount = br.ReadUInt32();
            table.xmlNamespaceEntry = new XmlNamespaceEntry[table.tableCount];

            for (int i = 0; i < table.tableCount; i++)
            {
                table.xmlNamespaceEntry[i].stringId = br.ReadUInt32();
            }

            xmlNamespaceTable = table;
            return true;
        }

        private void DumpHeader()
        {
            Console.WriteLine("HEADER:");
            Console.WriteLine("-------");
            Console.WriteLine("Magic Number: " + ByteArrayToString(fileHeader.magicNumber));
            Console.WriteLine("Metadata Size: " + fileHeader.metadataSize);
            Console.WriteLine("Node Stream Size: " + fileHeader.nodeSize);
            Console.WriteLine("Major File Version: " + fileHeader.majorFileVersion);
            Console.WriteLine("Minor File Version: " + fileHeader.minorFileVersion);
            Console.WriteLine("String Table Offset: " + fileHeader.stringTableOffset);
            Console.WriteLine("Assembly Table Offset: " + fileHeader.assemblyTableOffset);
            Console.WriteLine("Type Namespace Table Offset: " + fileHeader.typeNamespaceTableOffset);
            Console.WriteLine("Type Table Offset: " + fileHeader.typeTableOffset);
            Console.WriteLine("Property Table Offset: " + fileHeader.propertyTableOffset);
            Console.WriteLine("Xml Namespace Table Offset: " + fileHeader.xmlNamespaceTableOffset);
            Console.WriteLine("Hash: " + ByteArrayToString(Encoding.ASCII.GetBytes(new string(fileHeader.hash))));
        }

        private void DumpStringTable()
        {
            string header = "STRING TABLE (" + stringTable.tableCount + " entries)";
            Console.WriteLine(header);
            Console.WriteLine(new string('-', header.Length));
            for (int i = 0; i < stringTable.tableCount; i++)
            {
                Console.WriteLine(i + ": " + stringTable.stringEntry[i]);
            }
        }

        private void DumpAssemblyTable()
        {
            string header = "ASSEMBLY TABLE (" + assemblyTable.tableCount + " entries)";
            Console.WriteLine(header);
            Console.WriteLine(new string('-', header.Length));
            for (int i = 0; i < assemblyTable.tableCount; i++)
            {
                string providerKind = assemblyTable.assemblyEntry[i].providerKind.ToString();
                string assemblyName = GetStringFromId(assemblyTable.assemblyEntry[i].stringId);
                Console.WriteLine(i + ": " + assemblyName + "," + providerKind);
            }
        }

        private void DumpTypeNamespaceTable()
        {
            string header = "TYPE NAMESPACE TABLE (" + typeNamespaceTable.tableCount + " entries)";
            Console.WriteLine(header);
            Console.WriteLine(new string('-', header.Length));
            for (int i = 0; i < typeNamespaceTable.tableCount; i++)
            {
                string assemblyName = GetAssemblyNameFromId(typeNamespaceTable.typeNamespaceEntry[i].assemblyId);
                string typeNamespaceName = GetStringFromId(typeNamespaceTable.typeNamespaceEntry[i].stringId);
                Console.WriteLine(i + ": " + typeNamespaceName + " [" + assemblyName + "]");
            }
        }

        private void DumpTypeTable()
        {
            string header = "TYPE TABLE (" + typeTable.tableCount + " entries)";
            Console.WriteLine(header);
            Console.WriteLine(new string('-', header.Length));
            for (int i = 0; i < typeTable.tableCount; i++)
            {
                PersistedXamlTypeFlags typeFlags = typeTable.typeEntry[i].typeFlags;
                string typeNamespaceName = GetTypeNamespaceNameFromId(typeTable.typeEntry[i].typeNamespaceId);
                string typeName = GetStringFromId(typeTable.typeEntry[i].stringId);
                Console.WriteLine(i + ": " + typeName + " [" + typeNamespaceName + "] Flags: " + typeFlags.ToString());
            }
        }

        private void DumpPropertyTable()
        {
            string header = "PROPERTY TABLE (" + propertyTable.tableCount + " entries)";
            Console.WriteLine(header);
            Console.WriteLine(new string('-', header.Length));
            for (int i = 0; i < propertyTable.tableCount; i++)
            {
                PersistedXamlPropertyFlags propertyFlags = propertyTable.propertyEntry[i].propertyFlags;
                string typeName = GetTypeNameFromId(propertyTable.propertyEntry[i].typeId);
                string propertyName = GetStringFromId(propertyTable.propertyEntry[i].stringId);
                Console.WriteLine(i + ": " + propertyName + " [" + typeName + "] Flags: " + propertyFlags.ToString());
            }
        }

        private void DumpXmlNamespaceTable()
        {
            string header = "XML NAMESPACE TABLE (" + xmlNamespaceTable.tableCount + " entries)";
            Console.WriteLine(header);
            Console.WriteLine(new string('-', header.Length));
            for (int i = 0; i < xmlNamespaceTable.tableCount; i++)
            {
                string xmlNamespaceName = GetStringFromId(xmlNamespaceTable.xmlNamespaceEntry[i].stringId);
                Console.WriteLine(i + ": " + xmlNamespaceName);
            }
        }

        private void DumpNodes()
        {
            Console.WriteLine("NODE STREAM");
            Console.WriteLine("-----------");

            DumpNode(xamlTreeRoot, 0);
        }

        private void DumpNode(XamlObject xamlObject, int level)
        {
            string indent = new string(' ', level * 4);
            string typeName = GetTypeNameFromId(xamlObject.nodeInfo.nodeId);
            List<string> simpleProperties = new List<string>();
            bool hasComplexProperties = false;

            for (int i = 0; i < xamlObject.properties.Count; i++)
            {
                XamlProperty property = xamlObject.properties[i];
                if (property.propertyValue.Count == 1 && ((property.propertyValue[0] is XamlTextValue) || (property.propertyValue[0] is XamlValueNode)))
                {
                    string propertyName = GetPropertyNameFromId(property.nodeInfo.nodeId);
                    string propertyValue = string.Empty;
                    if (property.propertyValue[0] is XamlTextValue)
                    {
                        XamlNodeInfo xamlNodeInfo = (property.propertyValue[0] as XamlTextValue).nodeInfo;
                        propertyValue = GetStringFromId(xamlNodeInfo.nodeId);
                    }
                    else
                    {
                        propertyValue = (property.propertyValue[0] as XamlValueNode).textValue;
                    }

                    simpleProperties.Add(propertyName + "=\"" + propertyValue + "\"");
                }
                else if (property.propertyValue.Count >= 1 && property.propertyValue[0] is XamlObject)
                {
                    hasComplexProperties = true;
                }
            }

            if (simpleProperties.Count > 0)
            {
                Console.WriteLine(indent + "<" + typeName);
            }
            else
            {
                Console.Write(indent + "<" + typeName);
            }
            for (int i = 0; i < simpleProperties.Count; i++)
            {
                if (i != simpleProperties.Count - 1)
                {
                    Console.WriteLine(indent + "  " + simpleProperties[i]);
                }
                else
                {
                    Console.Write(indent + "  " + simpleProperties[i]);
                }
            }

            if (hasComplexProperties)
            {
                Console.WriteLine(">");
            }

            for (int i = 0; i < xamlObject.properties.Count; i++)
            {
                XamlProperty property = xamlObject.properties[i];
                if (property.propertyValue.Count >= 1 && property.propertyValue[0] is XamlObject)
                {
                    string propertyName = GetPropertyNameFromId(property.nodeInfo.nodeId);

                    Console.WriteLine(indent + "  <" + typeName + "." + propertyName + ">");

                    for (int j = 0; j < property.propertyValue.Count; j++)
                    {
                        if (property.propertyValue[j] is XamlObject)
                        {
                            XamlObject xamlCurrentObject = property.propertyValue[j] as XamlObject;
                            DumpNode(xamlCurrentObject, level + 1);
                        }
                        else if (property.propertyValue[j] is XamlTextValue)
                        {
                            XamlTextValue xamlTextValue = property.propertyValue[j] as XamlTextValue;
                            String value = GetStringFromId(xamlTextValue.nodeInfo.nodeId);
                            if (!String.IsNullOrWhiteSpace(value))
                            {
                                Debug.Fail("Unexpected State!");
                            }
                        }
                        else
                        {
                            Debug.Fail("Unexpected State!");
                        }
                    }

                    Console.WriteLine(indent + "  </" + typeName + "." + propertyName + ">");
                }
            }

            if (hasComplexProperties)
            {
                Console.WriteLine(indent + "</" + typeName + ">");
            }
            else
            {
                Console.WriteLine("/>");
            }
        }

        private XamlLineInfo ReadLineInfo(uint lineNumber, uint linePosition)
        {
            XamlLineInfo lineInfo = new XamlLineInfo();
            int lineNumberDelta = br.ReadInt16();
            int linePositionDelta = br.ReadInt16();

            lineInfo.lineNumber = (uint)((int)lineNumber + lineNumberDelta);
            if (lineNumberDelta > 0)
            {
                lineInfo.linePosition = (uint)linePositionDelta;
            }
            else
            {
                lineInfo.linePosition = (uint)((int)(lineInfo.linePosition + linePositionDelta));
            }

            return lineInfo;
        }

        private XamlLineInfo ReadLineInfoAbsolute()
        {
            XamlLineInfo lineInfo = new XamlLineInfo();
            lineInfo.lineNumber = br.ReadUInt32();
            lineInfo.linePosition = br.ReadUInt32();

            return lineInfo;
        }

        private XamlNodeInfo ReadXamlNode()
        {
            XamlNodeInfo nodeInfo = new XamlNodeInfo();
            nodeInfo.nodeId = br.ReadUInt32();
            nodeInfo.nodeFlags = br.ReadUInt32();

            return nodeInfo;
        }

        private XamlValueNode ReadXamlValueNode()
        {
            XamlValueNode textValue = null;
            PersistedXamlValueNodeType valueNodeType = (PersistedXamlValueNodeType)br.ReadByte();

            switch (valueNodeType)
            {
                case PersistedXamlValueNodeType.IsBoolFalse:
                    textValue = new XamlValueNode("True");
                    break;

                case PersistedXamlValueNodeType.IsBoolTrue:
                    textValue = new XamlValueNode("False");
                    break;

                case PersistedXamlValueNodeType.IsFloat:
                case PersistedXamlValueNodeType.IsKeyTime:
                case PersistedXamlValueNodeType.IsLengthConverter:
                case PersistedXamlValueNodeType.IsDuration:
                    float floatValue = br.ReadSingle();
                    textValue = new XamlValueNode(floatValue.ToString());
                    break;

                case PersistedXamlValueNodeType.IsSigned:
                    int singleValue = br.ReadInt32();
                    textValue = new XamlValueNode(singleValue.ToString());
                    break;

                case PersistedXamlValueNodeType.IsCString:
                    uint length = br.ReadUInt32();
                    char[] value = br.ReadChars((int)length);
                    
                    textValue = new XamlValueNode(new string(value));
                    break;

                case PersistedXamlValueNodeType.IsColor:
                    uint colorValue = br.ReadUInt32();
                    textValue = new XamlValueNode(colorValue.ToString());
                    break;

                case PersistedXamlValueNodeType.IsThickness:
                    float left = br.ReadSingle();
                    float top = br.ReadSingle();
                    float right = br.ReadSingle();
                    float bottom = br.ReadSingle();
                    textValue = new XamlValueNode(left.ToString() + "," + top.ToString() + "," + right.ToString() + "," + bottom.ToString());
                    break;

                case PersistedXamlValueNodeType.IsGridLength:
                    uint type = br.ReadUInt32();
                    float gridValue = br.ReadSingle();
                    string typeString = null;
                    if (type == 0)
                    {
                        typeString = "Auto";
                    }
                    else if (type == 1)
                    {
                        typeString = "Pixel";
                    }
                    else if (type == 2)
                    {
                        typeString = "Star";
                    }
                    textValue = new XamlValueNode(typeString + "," + gridValue.ToString());
                    break;
            }

            return textValue;
        }

        private XamlNamespaceInfo ReadXamlNamespaceNode()
        {
            XamlNamespaceInfo xamlNamespaceInfo = new XamlNamespaceInfo();
            xamlNamespaceInfo.nodeInfo = ReadXamlNode();

            uint length = br.ReadUInt32();
            xamlNamespaceInfo.name = new string(br.ReadChars((int)length));

            return xamlNamespaceInfo;
        }

        private string GetAssemblyNameFromId(uint id)
        {
            return GetStringFromId(assemblyTable.assemblyEntry[id].stringId);
        }

        private string GetTypeNamespaceNameFromId(uint id)
        {
            return GetStringFromId(typeNamespaceTable.typeNamespaceEntry[id].stringId);
        }

        private string GetTypeNameFromId(uint id)
        {
            return GetStringFromId(typeTable.typeEntry[id].stringId);
        }

        private string GetXmlNamespaceNameFromId(uint id)
        {
            return GetStringFromId(xmlNamespaceTable.xmlNamespaceEntry[id].stringId);
        }

        private string GetPropertyNameFromId(uint id)
        {
            return GetStringFromId(propertyTable.propertyEntry[id].stringId);
        }

        private string GetStringFromId(uint id)
        {
            return stringTable.stringEntry[id];
        }


        private static string ByteArrayToString(byte[] ba)
        {
            string hex = BitConverter.ToString(ba);
            return hex;
        }

        public BinaryReader br;
        public bool verboseMode;

        public XamlObject xamlTreeRoot;
        public Header fileHeader;
        public StringTable stringTable;
        public AssemblyTable assemblyTable;
        public TypeNamespaceTable typeNamespaceTable;
        public TypeTable typeTable;
        public PropertyTable propertyTable;
        public XmlNamespaceTable xmlNamespaceTable;
    }
}
