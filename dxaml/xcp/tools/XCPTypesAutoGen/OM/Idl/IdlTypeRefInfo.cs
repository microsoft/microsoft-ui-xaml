// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices.ComTypes;
namespace OM.Idl
{
    public class IdlTypeRefInfo : IIdlSelectorInfo
    {
        private TypeReference _owner;

        public string IdlParameterName
        {
            get
            {
                if (_owner.IsVoid)
                {
                    throw new InvalidOperationException("This is a 'void' type reference.");
                }

                if (_owner.IsReturnType)
                {
                    throw new InvalidOperationException("This is a return type.");
                }

                StringBuilder builder = new StringBuilder();
                if (_owner.IsOut)
                {
                    builder.Append("out ");
                }

                builder.Append(_owner.IdlTypeName);

                // Write parameter name.
                builder.Append(' ');
                Debug.Assert(!string.IsNullOrEmpty(_owner.Name));
                builder.Append(_owner.Name);

                return builder.ToString();
            }
        }

        public string AnnotatedParameterName
        {
            get
            {
                if (_owner.IsVoid)
                {
                    throw new InvalidOperationException("This is a 'void' type reference.");
                }

                if (_owner.IsReturnType)
                {
                    throw new InvalidOperationException("This is a return type.");
                }

                StringBuilder builder = new StringBuilder();

                string countParameterName = _owner.CountParameterName ?? _owner.Name + "Count";

                // Write annotations.
                if (_owner.IsArray)
                {
                    // [in] UINT {countParameterName}, [in, size_is({countParameterName})]
                    builder.Append("[in] UINT ");
                    builder.Append(countParameterName);
                    builder.Append(", [in, size_is(");
                    builder.Append(countParameterName);
                    builder.Append(")] ");
                }
                else if (_owner.IsOut)
                {
                    builder.Append("[out] ");
                }
                else
                {
                    builder.Append("[in] ");
                }

                // Write type name.
                if (_owner.IsNullable)
                {
                    builder.Append("Windows.Foundation.IReference<");
                }

                builder.Append(Type.IdlTypeInfo.GenericFullName);

                if (_owner.IsNullable)
                {
                    builder.Append(">*");
                }

                // Write pointers.
                if (!Type.IsValueType)
                {
                    builder.Append('*');
                }

                if (_owner.IsOut)
                {
                    builder.Append('*');
                }

                // Write parameter name.
                builder.Append(' ');
                Debug.Assert(!string.IsNullOrEmpty(_owner.Name));
                builder.Append(_owner.Name);

                // Write array indexer.
                if (_owner.IsArray)
                {
                    builder.Append("[]");
                }

                return builder.ToString();
            }
        }

        public string AnnotatedReturnParameterName
        {
            get
            {
                if (_owner.IsVoid)
                {
                    throw new InvalidOperationException("This is a 'void' type reference.");
                }

                StringBuilder builder = new StringBuilder();

                string countParameterName = _owner.CountParameterName ?? (_owner.IsReturnType ? "returnValueCount" : "count");

                // Write annotations.
                if (_owner.IsArray)
                {
                    // [out] UINT* {countParameterName}, [out, size_is(,*{countParameterName}), retval]
                    builder.Append("[out] UINT* ");
                    builder.Append(countParameterName);
                    builder.Append(", [out, size_is(,*");
                    builder.Append(countParameterName);
                    if (_owner.IsOut)
                    {
                        builder.Append(")] ");
                    }
                    else
                    {
                        builder.Append("), retval] ");
                    }
                }
                else if (_owner.IsOut)
                {
                    // Exists for compat only: in Windows 8 ITextRangeProvider.GetBoundingRectangles used an out parameter for its return value by accident.
                    builder.Append("[out] ");
                }
                else
                {
                    builder.Append("[out, retval] ");
                }

                // Write type name.
                if (_owner.IsNullable)
                {
                    builder.Append("Windows.Foundation.IReference<");
                }
                builder.Append(Type.IdlTypeInfo.GenericFullName);
                if (_owner.IsNullable)
                {
                    builder.Append(">*");
                }

                // Write pointers.
                if (!Type.IsValueType)
                {
                    builder.Append('*');
                }

                if (_owner.IsArray)
                {
                    builder.Append('*');
                }

                builder.Append('*');

                // Write parameter name.
                builder.Append(' ');
                if (string.IsNullOrEmpty(_owner.Name))
                {
                    // Only allow the Name to be null for return types.
                    Debug.Assert(_owner.IsReturnType);
                    builder.Append("returnValue");
                }
                else
                {
                    builder.Append(_owner.Name);
                }

                return builder.ToString();
            }
        }

        public string ArgumentName
        {
            get
            {
                return _owner.Name;
            }
        }

        /// <summary>
        /// Should only be used for type references for AttributeDefinition.Properties.
        /// </summary>
        public string AttributePropertyName
        {
            get;
            set;
        }

        public string GenericFullName
        {
            get
            {
                if (_owner.IsVoid)
                {
                    throw new InvalidOperationException("This is a 'void' type reference.");
                }

                StringBuilder builder = new StringBuilder();
                if (_owner.IsNullable)
                {
                    builder.Append("Windows.Foundation.IReference<");
                }
                builder.Append(Type.IdlTypeInfo.GenericFullName);
                if (_owner.IsNullable)
                {
                    builder.Append(">");
                }
                return builder.ToString();
            }
        }

        public TypeDefinition Type
        {
            get;
            set;
        }

        internal IdlTypeRefInfo(TypeReference owner)
        {
            _owner = owner;
        }

        public bool IsExcluded
        {
            get;
            set;
        }

        int IIdlSelectorInfo.Version
        {
            get
            {
                return _owner.Version;
            }
        }
    }
}
