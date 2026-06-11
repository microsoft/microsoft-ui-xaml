// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class NamespaceDefinition
    {
        public static readonly NamespaceDefinition UnknownNamespace = new NamespaceDefinition()
        {
            IsUnknown = true,
            Name = "UnknownNamespace",
            TypeTableIndex = 0,
            TypeTableName = ""
        };

        private List<AttributeDefinition> _attributes = new List<AttributeDefinition>();
        private List<ClassDefinition> _classes = new List<ClassDefinition>();
        private List<DelegateDefinition> _delegates = new List<DelegateDefinition>();
        private List<EnumDefinition> _enums = new List<EnumDefinition>();
        private List<ContractDefinition> _contracts = new List<ContractDefinition>();

        public List<AttributeDefinition> Attributes
        {
            get
            {
                return _attributes;
            }
        }
        
        public List<ClassDefinition> Classes
        {
            get
            {
                return _classes;
            }
        }

        public List<DelegateDefinition> Delegates
        {
            get
            {
                return _delegates;
            }
        }

        public List<EnumDefinition> Enums
        {
            get
            {
                return _enums;
            }
        }

        public List<ContractDefinition> Contracts
        {
            get
            {
                return _contracts;
            }
        }

        public Idl.IdlNamespaceInfo IdlInfo
        {
            get;
            private set;
        }

        private string _indexName;
        public string IndexName
        {
            get
            {
                return _indexName;
            }
            set
            {
                _indexName = value;
            }
        }

        public string IndexNameWithCastToInt
        {
            get
            {
                return "static_cast<int>(" + IndexName + ")";
            }
        }

        private string _indexNameWithoutPrefix;
        public string IndexNameWithoutPrefix
        {
            get
            {
                return _indexNameWithoutPrefix;
            }
            set
            {
                _indexNameWithoutPrefix = value;
            }
        }

        public bool IsExcludedFromTypeTable
        {
            get;
            set;
        }

        public bool IsUnknown
        {
            get;
            set;
        }

        private string _name;
        public string Name
        {
            get
            {
                return _name;
            }
            set
            {
                _name = value;
            }
        }

        public IEnumerable<ClassDefinition> NonGenericReferenceClasses
        {
            get
            {
                return Classes.Where(c => !c.IsValueType && !c.IsGenericType);
            }
        }

        public int Order
        {
            get;
            set;
        }

        public int TypeTableIndex
        {
            get;
            set;
        }

        private string _typeTableName;
        public string TypeTableName
        {
            get
            {
                return _typeTableName;
            }
            set
            {
                _typeTableName = value;
            }
        }

        public IEnumerable<ClassDefinition> ReferenceClasses
        {
            get
            {
                return Classes.Where(c => !c.IsValueType);
            }
        }

        public IEnumerable<ClassDefinition> ValueTypes
        {
            get
            {
                return Classes.Where(c => c.IsValueType);
            }
        }

        public bool IsXamlNamespace
        {
            get
            {
                if (Classes.Any(c=> c.IsImported))
                {
                    return false;
                }
                else if (Delegates.Any(d => d.IsImported))
                {
                    return false;
                }
                else if (Enums.Any(e => e.IsImported))
                {
                    return false;
                }
                else
                {
                    return true;
                }
            }
        }

        public NamespaceDefinition()
        {
            IdlInfo = new Idl.IdlNamespaceInfo(this);
        }

        public NamespaceDefinition(NamespaceDefinition source)
            : this()
        {
            IdlInfo.Group = source.IdlInfo.Group;
            IsExcludedFromTypeTable = source.IsExcludedFromTypeTable;
            IndexName = source.IndexName;
            IndexNameWithoutPrefix = source.IndexNameWithoutPrefix;
            Name = source.Name;
            Order = source.Order;
            TypeTableIndex = source.TypeTableIndex;
            TypeTableName = source.TypeTableName;
        }

        public override string ToString()
        {
            return Name;
        }
    }
}
