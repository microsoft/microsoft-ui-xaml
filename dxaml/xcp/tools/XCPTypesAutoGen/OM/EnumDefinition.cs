// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class EnumDefinition : TypeDefinition
    {
        private List<EnumVersion> _versions = new List<EnumVersion>();
        private List<EnumValueDefinition> _values = new List<EnumValueDefinition>();

        public string AbiImplementationName
        {
            get
            {
                return OMContext.DefaultImplementationNamespace + '.' + Name;
            }
        }

        public string CoreCreationMethodName
        {
            get
            {
                return "Create";
            }
        }

        public string CoreDefaultConstructor
        {
            get
            {
                if (IsExcludedFromTypeTable)
                {
                    throw new InvalidOperationException("Enum does not exist in type table.");
                }

                return "OnCoreCreate" + Name;
            }
        }

        public string CoreDefaultConstructorPointer
        {
            get
            {
                if (!IsExcludedFromTypeTable)
                {
                    return "&" + CoreDefaultConstructor;
                }
                return string.Empty;
            }
        }

        public bool GenerateInCore
        {
            get
            {
                return Values.Any();
            }
        }

        public Idl.IdlEnumInfo IdlEnumInfo
        {
            get;
            private set;
        }

        public override Idl.IdlTypeInfo IdlTypeInfo
        {
            get
            {
                return IdlEnumInfo;
            }
        }

        public bool IsAutomationEnum
        {
            get;
            set;
        }

        public bool IsAutomationCoreEnum
        {
            get;
            set;
        }

        public bool IsVersionProjection
        {
            get;
            internal set;
        }

        public string UIAComment
        {
            get;
            set;
        }

        public string UIAName
        {
            get;
            set;
        }

        public List<EnumValueDefinition> Values
        {
            get
            {
                return _values;
            }
        }

        public string ValueTableArrayName
        {
            get
            {
                return "sat" + Name;
            }
        }

        /// <summary>
        /// Gets the version this class represents. If IsVersionProjection is false, this will return 0.
        /// </summary>
        public int Version
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the velocity version this enum represents. If this value is nonzero then the enum is
        /// part of a velocity feature.
        /// </summary>
        public int VelocityVersion
        {
            get;
            set;
        }

        public List<EnumVersion> Versions
        {
            get
            {
                return _versions;
            }
        }

        public XamlEnumFlags XamlEnumFlags
        {
            get;
            set;
        }

        public int PhoneEnumIndex
        {
            get;
            set;
        }

        public EnumDefinition()
        {
            IdlEnumInfo = new Idl.IdlEnumInfo(this);
            XamlEnumFlags = new XamlEnumFlags();
            PhoneEnumIndex = -1;
        }

        public EnumVersion GetVersion(int version)
        {
            return Versions.Single(v => v.Version == version);
        }

        public string GetNativeTypeSpecification()
        {
            return IsCompact() ? ": uint8_t" : "";
        }

        public bool IsCompact()
        {
            return Values.Max(v => (uint)v.Value) <= 255u;
        }
    }
}
