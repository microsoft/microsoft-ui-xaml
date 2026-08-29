// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using DumpXBF.FileFormat.NodeStream;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DumpXBF
{
    class XamlNode 
    {
        public XamlLineInfo lineInfo;
    }

    class XamlTextValue : XamlNode
    {
        public XamlTextValue(XamlNodeInfo info)
        {
            nodeInfo = info;
        }

        public XamlNodeInfo nodeInfo;
    }

    class XamlValueNode : XamlNode
    {
        public XamlValueNode(string value)
        {
            textValue = value;
        }

        public string textValue;
    }

    class XamlProperty : XamlNode
    {
        public XamlProperty(XamlNodeInfo info)
        {
            nodeInfo = info;
            propertyValue = new List<XamlNode>();
        }

        public List<XamlNode> propertyValue;
        public XamlNodeInfo nodeInfo;
    }

    class XamlObject : XamlNode
    {
        public XamlObject(XamlNodeInfo info)
        {
            nodeInfo = info;
            properties = new List<XamlProperty>();
        }
            
        public XamlNodeInfo nodeInfo;
        public List<XamlProperty> properties;
    }
}
