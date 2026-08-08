// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Implements SRM's ISignatureTypeProvider and ICustomAttributeTypeProvider callbacks.
// SRM walks the signature blob internally and calls back into these methods.

using System;
using System.Collections.Immutable;
using System.Linq;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

using System.Reflection;
using Type = System.Type;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// SRM signature type provider that builds LMR Type objects from metadata signature blobs.
    /// One instance per MetadataOnlyModule (needs module for type resolution and caching).
    /// Implements both ISignatureTypeProvider (method/field/property/typespec/local sigs)
    /// and ICustomAttributeTypeProvider (custom attribute argument blobs).
    /// </summary>
    internal sealed class LmrTypeProvider :
        ISignatureTypeProvider<Type, GenericContext>,
        ICustomAttributeTypeProvider<Type>
    {
        private readonly MetadataOnlyModule m_module;

        internal LmrTypeProvider(MetadataOnlyModule module)
        {
            m_module = module ?? throw new ArgumentNullException(nameof(module));
        }

        #region ISimpleTypeProvider<Type> (shared by both interfaces)

        public Type GetPrimitiveType(PrimitiveTypeCode typeCode)
        {
            var corType = MapPrimitiveToCorElement(typeCode);
            return m_module.AssemblyResolver.GetBuiltInType(corType);
        }

        public Type GetTypeFromDefinition(MetadataReader reader, TypeDefinitionHandle handle, byte rawTypeKind)
        {
            Debug.Assert(reader == m_module.RawReader);
            return m_module.ResolveTypeDef(handle);
        }

        public Type GetTypeFromReference(MetadataReader reader, TypeReferenceHandle handle, byte rawTypeKind)
        {
            Debug.Assert(reader == m_module.RawReader);
            // rawTypeKind: 0=unknown, 17=ValueType, 18=Class (from the signature encoding).
            // When non-zero, MetadataOnlySignatureTypeReference uses it directly for IsValueType.
            // When zero, it falls back to resolving the type and checking BaseType.
            return m_module.Factory.CreateSignatureTypeRef(m_module, handle, rawTypeKind);
        }

        #endregion

        #region ISZArrayTypeProvider<Type>

        public Type GetSZArrayType(Type elementType) => elementType.MakeArrayType();

        #endregion

        #region IConstructedTypeProvider<Type>

        public Type GetGenericInstantiation(Type genericType, ImmutableArray<Type> typeArguments)
            => genericType.MakeGenericType(typeArguments.ToArray());

        public Type GetArrayType(Type elementType, ArrayShape shape)
            => elementType.MakeArrayType(shape.Rank);

        public Type GetByReferenceType(Type elementType) => elementType.MakeByRefType();

        public Type GetPointerType(Type elementType) => elementType.MakePointerType();

        #endregion

        #region ISignatureTypeProvider<Type, GenericContext> (own methods)

        public Type GetFunctionPointerType(MethodSignature<Type> signature)
        {
            // LMR treats function pointers as IntPtr (matches the old SignatureUtil behavior)
            return m_module.AssemblyResolver.GetBuiltInType(CorElementType.IntPtr);
        }

        public Type GetGenericMethodParameter(GenericContext genericContext, int index)
        {
            if (genericContext != null && index < genericContext.MethodArgs.Length)
            {
                return genericContext.MethodArgs[index];
            }
            // Fall back to creating a type variable reference if context is insufficient
            return m_module.CreateMethodTypeVariable(index, genericContext);
        }

        public Type GetGenericTypeParameter(GenericContext genericContext, int index)
        {
            if (genericContext != null && index < genericContext.TypeArgs.Length)
            {
                return genericContext.TypeArgs[index];
            }
            // Fall back to creating a type variable reference if context is insufficient
            return m_module.CreateTypeTypeVariable(index, genericContext);
        }

        public Type GetModifiedType(Type modifier, Type unmodifiedType, bool isRequired)
            => new SignatureModifiedType(unmodifiedType, modifier, isRequired);

        public Type GetPinnedType(Type elementType)
            => new SignaturePinnedType(elementType);

        public Type GetTypeFromSpecification(MetadataReader reader, GenericContext genericContext, TypeSpecificationHandle handle, byte rawTypeKind)
        {
            Debug.Assert(reader == m_module.RawReader);
            return m_module.ResolveTypeSpec(handle, genericContext);
        }

        #endregion

        #region ICustomAttributeTypeProvider<Type> (own methods)

        public Type GetSystemType()
        {
            // Return typeof(System.Type) from the universe
            return m_module.AssemblyResolver.GetBuiltInType(CorElementType.Type);
        }

        public Type GetTypeFromSerializedName(string name)
        {
            if (string.IsNullOrEmpty(name))
                return null;

            return TypeNameParser.ParseTypeName(
                m_module.AssemblyResolver,
                m_module,
                name,
                useSystemAssemblyToResolveTypes: true,
                useWindowsRuntimeResolution: MetadataOnlyModule.IsWindowsRuntime(m_module),
                throwOnError: true);
        }

        public PrimitiveTypeCode GetUnderlyingEnumType(Type type)
        {
            // Get the underlying type of the enum and map back to PrimitiveTypeCode
            var underlyingType = m_module.GetEnumUnderlyingType(type);
            return MapCorElementToPrimitive(underlyingType);
        }

        public bool IsSystemType(Type type)
        {
            if (type == null) return false;
            var systemType = GetSystemType();
            return systemType != null && type.Equals(systemType);
        }

        #endregion

        #region Primitive type mapping helpers

        /// <summary>
        /// Maps SRM PrimitiveTypeCode to LMR CorElementType.
        /// </summary>
        internal static CorElementType MapPrimitiveToCorElement(PrimitiveTypeCode code)
        {
            switch (code)
            {
                case PrimitiveTypeCode.Void: return CorElementType.Void;
                case PrimitiveTypeCode.Boolean: return CorElementType.Bool;
                case PrimitiveTypeCode.Char: return CorElementType.Char;
                case PrimitiveTypeCode.SByte: return CorElementType.SByte;
                case PrimitiveTypeCode.Byte: return CorElementType.Byte;
                case PrimitiveTypeCode.Int16: return CorElementType.Short;
                case PrimitiveTypeCode.UInt16: return CorElementType.UShort;
                case PrimitiveTypeCode.Int32: return CorElementType.Int;
                case PrimitiveTypeCode.UInt32: return CorElementType.UInt;
                case PrimitiveTypeCode.Int64: return CorElementType.Long;
                case PrimitiveTypeCode.UInt64: return CorElementType.ULong;
                case PrimitiveTypeCode.Single: return CorElementType.Float;
                case PrimitiveTypeCode.Double: return CorElementType.Double;
                case PrimitiveTypeCode.String: return CorElementType.String;
                case PrimitiveTypeCode.TypedReference: return CorElementType.TypedByRef;
                case PrimitiveTypeCode.IntPtr: return CorElementType.IntPtr;
                case PrimitiveTypeCode.UIntPtr: return CorElementType.UIntPtr;
                case PrimitiveTypeCode.Object: return CorElementType.Object;
                default:
                    throw new ArgumentOutOfRangeException(nameof(code), code, "Unexpected PrimitiveTypeCode");
            }
        }

        /// <summary>
        /// Maps LMR CorElementType back to SRM PrimitiveTypeCode (for custom attribute enum support).
        /// </summary>
        private static PrimitiveTypeCode MapCorElementToPrimitive(CorElementType corType)
        {
            switch (corType)
            {
                case CorElementType.Bool: return PrimitiveTypeCode.Boolean;
                case CorElementType.Char: return PrimitiveTypeCode.Char;
                case CorElementType.SByte: return PrimitiveTypeCode.SByte;
                case CorElementType.Byte: return PrimitiveTypeCode.Byte;
                case CorElementType.Short: return PrimitiveTypeCode.Int16;
                case CorElementType.UShort: return PrimitiveTypeCode.UInt16;
                case CorElementType.Int: return PrimitiveTypeCode.Int32;
                case CorElementType.UInt: return PrimitiveTypeCode.UInt32;
                case CorElementType.Long: return PrimitiveTypeCode.Int64;
                case CorElementType.ULong: return PrimitiveTypeCode.UInt64;
                case CorElementType.Float: return PrimitiveTypeCode.Single;
                case CorElementType.Double: return PrimitiveTypeCode.Double;
                case CorElementType.String: return PrimitiveTypeCode.String;
                case CorElementType.IntPtr: return PrimitiveTypeCode.IntPtr;
                case CorElementType.UIntPtr: return PrimitiveTypeCode.UIntPtr;
                default:
                    throw new ArgumentOutOfRangeException(nameof(corType), corType, "Unexpected CorElementType for enum underlying type");
            }
        }

        #endregion
    }
}
