// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using System.Text.RegularExpressions;

namespace OM
{
    public class TypeReference
    {
        public string AbiArgumentName
        {
            get
            {
                StringBuilder builder = new StringBuilder();

                int dimensions = 0;

                if (IsNullable)
                {
                    dimensions++;
                }
                if (!IdlInfo.Type.IsValueType)
                {
                    dimensions++;
                }
                if (IsOut)
                {
                    dimensions++;
                }
                if (IsArray)
                {
                    builder.Append(CountParameterName ?? (Name + "Count"));
                    builder.Append(", ");
                }

                builder.Append(Helper.ToPointerName(Name, dimensions));
                return builder.ToString();
            }
        }

        public string AbiImplementationFullName
        {
            get
            {
                ClassDefinition typeAsClass = IdlInfo.Type as ClassDefinition;
                if (typeAsClass != null)
                {
                    return typeAsClass.AbiImplementationFullName;
                }

                EnumDefinition typeAsEnum = IdlInfo.Type as EnumDefinition;
                if (typeAsEnum != null)
                {
                    return typeAsEnum.AbiImplementationName;
                }
                return Helper.GetAbiRuntimeClassFullName(IdlInfo.Type);
            }
        }

        public string AbiReturnArgumentName
        {
            get
            {
                StringBuilder builder = new StringBuilder();

                // We're a return parameter, so initialize to 1.
                int dimensions = 1;

                if (IsNullable)
                {
                    dimensions++;
                }
                if (!IdlInfo.Type.IsValueType)
                {
                    dimensions++;
                }
                if (IsOut)
                {
                    dimensions++;
                }
                if (IsArray)
                {
                    builder.Append(Helper.ToPointerName(CountParameterName ?? (Name + "Count")));
                    builder.Append(", ");
                }

                builder.Append(Helper.ToPointerName(Name, dimensions));
                return builder.ToString();
            }
        }

        public string IdlTypeName
        {
            get
            {
                if (IsVoid)
                {
                    return "void";
                }

                var builder = new StringBuilder();
                if (IsNullable)
                {
                    builder.Append("Windows.Foundation.IReference<");
                }

                if (!string.IsNullOrEmpty(IdlInfo.Type.CorrectedTypeName))
                {
                    builder.Append(IdlInfo.Type.CorrectedTypeName);
                }
                else if (IdlInfo.Type.IsGenericType)
                {
                    builder.Append(IdlInfo.Type.FriendlyGenericFullName);
                }
                else
                {
                    builder.Append(IdlInfo.Type.FullName);
                }

                if (IsNullable)
                {
                    builder.Append(">");
                }

                if (IsArray)
                {
                    builder.Append("[]");
                }

                return builder.ToString();
            }
        }

        public string AbiFullName
        {
            get
            {
                if (IsVoid)
                {
                    return "void";
                }

                StringBuilder builder = new StringBuilder();

                if (IsNullable)
                {
                    builder.Append(Helper.PrefixAbi("Windows.Foundation.IReference<")); 
                    builder.Append(Helper.GetAbiReferenceFullName(IdlInfo.Type));
                }
                else 
                {
                    builder.Append(Helper.GetAbiReferenceFullName(IdlInfo.Type));
                }
                if (IsNullable)
                {
                    builder.Append(">");
                }

                return builder.ToString();
            }
        }

        public string AbiFullNameNoABIPrefix => Regex.Replace(AbiFullName, @"\bABI(.|::)", "");

        public override string ToString()
        {
            return AbiFullName;
        }
        public string AbiParameterName
        {
            get
            {
                int dimensions = 0;
                if (IsNullable)
                {
                    dimensions++;
                }
                if (!IdlInfo.Type.IsValueType)
                {
                    dimensions++;
                }
                if (IsOut)
                {
                    dimensions++;
                }
                return Helper.ToPointerName(Name, dimensions);
            }
        }

        public string AbiReturnParameterName
        {
            get
            {
                // We're a return parameter, so initialize to 1.
                int dimensions = 1;

                if (IsNullable)
                {
                    dimensions++;
                }
                if (!IdlInfo.Type.IsValueType)
                {
                    dimensions++;
                }
                if (IsOut)
                {
                    dimensions++;
                }

                return Helper.ToPointerName(Name, dimensions);
            }
        }

        public string AnnotatedAbiParameterName
        {
            get
            {
                if (IsVoid)
                {
                    throw new InvalidOperationException("This is a 'void' type reference.");
                }

                if (IsReturnType)
                {
                    throw new InvalidOperationException("This is a return type.");
                }

                StringBuilder builder = new StringBuilder();

                string countParameterName = CountParameterName ?? Name + "Count";
                TypeDefinition type = IdlInfo.Type;

                // Write annotations.
                if (IsArray)
                {
                    // _In_ UINT {countParameterName}, _In_reads_({countParameterName})
                    builder.Append("_In_ UINT ");
                    builder.Append(countParameterName);
                    builder.Append(", _In_reads_(");
                    builder.Append(countParameterName);
                    builder.Append(")");
                }
                else
                {
                    if (IsOut)
                    {
                        if (type.IsValueType)
                        {
                            builder.Append("_Out_");
                        }
                        else
                        {
                            builder.Append("_Outptr_");
                        }
                    }
                    else
                    {
                        builder.Append("_In_");
                    }
                    if (IsOptional)
                    {
                        if (!type.IsValueType || type.IsStringType)
                        {
                            builder.Append("opt_");
                        }
                    }
                }
                builder.Append(' ');

                // Write type name.
                if (IsNullable)
                {
                    builder.Append(Helper.PrefixAbi("Windows.Foundation.IReference<"));
                }
                builder.Append(Helper.GetAbiReferenceFullName(IdlInfo.Type, isGenericArgument: IsNullable));
                if (IsNullable)
                {
                    builder.Append(">*");
                }

                // Write pointers.
                if (!type.IsValueType)
                {
                    builder.Append('*');
                }

                if (IsOut)
                {
                    builder.Append('*');
                }

                // Write parameter name.
                builder.Append(' ');
                builder.Append(AbiParameterName);

                // Write array indexer.
                if (IsArray)
                {
                    builder.Append("[]");
                }

                return builder.ToString();
            }
        }

        public string AnnotatedAbiReturnParameterName
        {
            get
            {
                if (IsVoid)
                {
                    throw new InvalidOperationException("This is a 'void' type reference.");
                }

                StringBuilder builder = new StringBuilder();

                string countParameterName = Helper.ToPointerName(CountParameterName ?? (IsReturnType ? "returnValueCount" : "count"));
                TypeDefinition type = IdlInfo.Type;

                // Write annotations.
                if (IsArray)
                {
                    // _Out_ UINT* {countParameterName}, _Out_writes_to_ptr_(*{countParameterName})
                    builder.Append("_Out_ UINT* ");
                    builder.Append(countParameterName);
                    builder.Append(", _Out_writes_to_ptr_(*");
                    builder.Append(countParameterName);
                    builder.Append(")");
                }
                else
                {
                    if (IsOut)
                    {
                        // Exists for compat only: in Windows 8 ITextRangeProvider.GetBoundingRectangles used an out parameter for its return value by accident.
                        builder.Append("_Out_");
                    }
                    else
                    {
                        if (type.IsValueType)
                        {
                            builder.Append("_Out_");
                        }
                        else
                        {
                            builder.Append("_Outptr_");
                            if (IsOptional)
                            {
                                builder.Append("result_maybenull_");
                            }
                        }
                    }
                }
                builder.Append(' ');

                // Write type name.
                if (IsNullable)
                {
                    builder.Append(Helper.PrefixAbi("Windows.Foundation.IReference<"));
                }
                builder.Append(Helper.GetAbiReferenceFullName(IdlInfo.Type, isGenericArgument: IsNullable));
                if (IsNullable)
                {
                    builder.Append(">*");
                }

                // Write pointers.
                if (!type.IsValueType)
                {
                    builder.Append('*');
                }

                if (IsArray)
                {
                    builder.Append('*');
                }

                builder.Append('*');

                // Write parameter name.
                builder.Append(' ');
                builder.Append(AbiReturnParameterName);

                return builder.ToString();
            }
        }

        public string CoreBufferParameterName
        {
            get
            {
                return AbiParameterName + "Buffer";
            }
        }

        public string CoreFullName
        {
            get
            {
                if (Type is EnumDefinition)
                {
                    return AbiImplementationFullName;
                }
                else if (Type.IsValueType)
                {
                    if (!string.IsNullOrEmpty(Type.PrimitiveCoreName))
                    {
                        return Type.PrimitiveCoreName;
                    }
                    return AbiFullName;
                }
                else
                {
                    return string.Format("{0}*", Type.CoreName);
                }
            }
        }

         public string CoreParameterName
        {
            get
            {
                return AbiParameterName + "Core";
            }
        }

        public string CoreReturnParameterName
        {
            get
            {
                return AbiReturnParameterName + "Core";
            }
        }

        /// <summary>
        /// Describes the name of the count parameter.
        /// </summary>
        public string CountParameterName
        {
            get;
            set;
        }

        public Idl.IdlTypeRefInfo IdlInfo
        {
            get;
            private set;
        }

        public bool IsArray
        {
            get;
            set;
        }

        /// <summary>
        /// Only used for references to implemented interfaces.
        /// </summary>
        public bool IsImplicitInterface
        {
            get;
            set;
        }

        public bool IsIntPtr
        {
            get;
            set;
        }

        public bool IsNullable
        {
            get;
            set;
        }

        public bool IsOptional
        {
            get;
            set;
        }

        public bool IsOut
        {
            get;
            set;
        }

        public bool IsReturnType
        {
            get;
            set;
        }

        public bool IsValueType
        {
            get
            {
                return IdlInfo.Type.IsValueType && !IsNullable;
            }
        }

        public bool IsVoid
        {
            get;
            set;
        }

        /// <summary>
        /// Describes the name of the reference. Generally only used for parameters.
        /// </summary>
        public string Name
        {
            get;
            set;
        }

        public TypeDefinition Type
        {
            get;
            set;
        }

        /// <summary>
        /// Used to describe as of what version a type is implementing an interface.
        /// </summary>
        public int Version
        {
            get;
            set;
        }

        public TypeReference(TypeDefinition type)
        {
            IdlInfo = new Idl.IdlTypeRefInfo(this);
            Version = 1;

            Type = type;
            IdlInfo.Type = type;
        }
    }

    public class TypeReferenceEqualityByTypeComparer : IEqualityComparer<TypeReference>
    {
        public static TypeReferenceEqualityByTypeComparer Instance { get; private set; } = new TypeReferenceEqualityByTypeComparer();

        public bool Equals(TypeReference x, TypeReference y)
        {
            return x.Type == y.Type;
        }

        public int GetHashCode(TypeReference obj)
        {
            return obj.Type.GetHashCode();
        }
    }
}
