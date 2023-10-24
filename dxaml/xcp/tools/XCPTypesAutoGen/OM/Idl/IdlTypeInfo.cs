// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Text;

namespace OM.Idl
{
    public abstract class IdlTypeInfo : IIdlSelectorInfo
    {
        private TypeDefinition _owner;

        /// <summary>
        /// Whether this type should not be emitted for lifted codegen.
        /// </summary>
        public bool ExcludeFromLiftedCodegen
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the IDL file name in which this type is defined. May be null if the type is not imported from an external library.
        /// </summary>
        public string FileName
        {
            get;
            set;
        }

        public virtual string FullName
        {
            get
            {
                if (_owner.IsPrimitive)
                {
                    return Name;
                }

                return DeclaringNamespace + "."  + Name;
            }
        }

        public string DeclaringNamespace => _owner.DeclaringNamespace.Name;

        public string GenericName
        {
            get
            {
                if (!_owner.IsGenericType)
                {
                    return Name;
                }

                StringBuilder builder = new StringBuilder(Name);
                builder.Append('<');
                bool first = true;
                foreach (TypeReference arg in _owner.GenericArguments)
                {
                    if (!first)
                    {
                        builder.Append(", ");
                    }
                    first = false;
                    builder.Append(arg.IdlInfo.GenericFullName);
                }

                // Avoid generating ">>", because it will confuse the compiler.
                if (builder[builder.Length - 1] == '>')
                {
                    builder.Append(' ');
                }
                builder.Append('>');
                return builder.ToString();
            }
        }

        public string GenericFullName
        {
            get
            {
                if (_owner.IsPrimitive)
                {
                    return GenericName;
                }

                // Types in Windows.Foundation.Collections don't need to be fully qualified
                // (except, for some reason, W.F.C.IPropertySet)
                string declaringNamespaceName = DeclaringNamespace;
                if (declaringNamespaceName == "Windows.Foundation.Collections" && GenericName != "IPropertySet")
                {
                    return GenericName;
                }
                else
                {
                    return declaringNamespaceName + "." + GenericName;
                }
            }
        }

        public string Group
        {
            get;
            set;
        }

        public abstract bool HasRuntimeClass
        {
            get;
        }

        public bool IsExcluded
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether the type is defined in the current library, but its interface is imported from a different library. Not to 
        /// // be confused with TypeDefinition.IsImported.
        /// Example: Windows.UI.Color.
        /// </summary>
        public bool IsImported
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether this type only exists in the private IDL.
        /// </summary>
        public bool IsPrivateIdlOnly
        {
            get;
            set;
        }

        private string _name;

        public virtual string Name
        {
            get
            {
                return _name;
            }
            set
            {
                _name = value;

                bool ownerIsInterface = _owner != null && _owner is ClassDefinition && (_owner as ClassDefinition).IsInterface;
            }
        }

        public virtual int Version
        {
            get
            {
                return 1;
            }
        }

        protected IdlTypeInfo(TypeDefinition owner)
        {
            _owner = owner;
        }
    }
}
