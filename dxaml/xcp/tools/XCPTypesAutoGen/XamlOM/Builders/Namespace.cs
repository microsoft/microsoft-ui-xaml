// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = true, Inherited = false)]
    public class NamespaceAttribute : Attribute, NewBuilders.INamespaceBuilder
    {
        public string DXamlIdlGroup
        {
            get;
            set;
        }

        public string DXamlName
        {
            get;
            set;
        }

        public string Name
        {
            get;
            set;
        }

        public string IndexName
        {
            get;
            set;
        }

        public bool IsExcludedFromDXamlTypeTable
        {
            get;
            set;
        }

        public bool IsExcludedFromTypeTable
        {
            get;
            set;
        }

        public int Order
        {
            get;
            set;
        }

        public void BuildNewNamespace(OM.NamespaceDefinition definition)
        {
            definition.IdlInfo.Group = DXamlIdlGroup;
            definition.Name = DXamlName ?? Name;
            definition.TypeTableName = definition.Name;
            definition.IndexName = IndexName;
            definition.IsExcludedFromTypeTable = IsExcludedFromDXamlTypeTable && IsExcludedFromTypeTable;
        }
    }
}
