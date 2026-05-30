// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class AttributeDefinition : TypeDefinition
    {
        private List<PropertyDefinition> _properties = new List<PropertyDefinition>();

        public bool AllowMultiple
        {
            get;
            set;
        }

        public string AttributeName
        {
            get
            {
                if (Name.EndsWith("Attribute"))
                {
                    return Name.Remove(Name.Length - "Attribute".Length).ToLower();
                }
                return Name.ToLower();
            }
        }

        public Idl.IdlAttributeInfo IdlAttributeInfo
        {
            get;
            private set;
        }

        public override Idl.IdlTypeInfo IdlTypeInfo
        {
            get
            {
                return IdlAttributeInfo;
            }
        }

        public List<PropertyDefinition> Properties
        {
            get
            {
                return _properties;
            }
        }

        public AttributeTargets ValidOn
        {
            get;
            set;
        }

        public string ValidOnString
        {
            get
            {
                var targets = new List<string>();
                var flags = ValidOn;

                if (ValidOn.HasFlag(AttributeTargets.Property)) 
                {
                    targets.Add("target_property");
                    flags &= ~AttributeTargets.Property;
                }

                if (ValidOn.HasFlag(AttributeTargets.Class)) 
                {
                    targets.Add("target_runtimeclass");
                    flags &= ~AttributeTargets.Class;
                }

                if (ValidOn.HasFlag(AttributeTargets.Struct)) 
                {
                    targets.Add("target_struct");
                    flags &= ~AttributeTargets.Struct;
                }

                if (flags == 0) 
                {
                    return String.Join(", ", targets);
                }
                else 
                {
                    // Expand as needed...
                    throw new NotImplementedException();
                }
            }
        }

        public AttributeDefinition()
        {
            IdlAttributeInfo = new Idl.IdlAttributeInfo(this);
            IsExcludedFromTypeTable = true;
            ValidOn = AttributeTargets.Class;
        }
    }
}
