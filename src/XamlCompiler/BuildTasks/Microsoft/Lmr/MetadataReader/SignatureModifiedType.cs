// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Internal Type subclass used during signature decoding to carry modreq/modopt information.
// Peeled off by SignatureUnwrap.Unwrap() after DecodeSignature returns.
// Never escapes into public reflection results.

using System;
using System.Globalization;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Wraps a Type with an associated modifier (modreq or modopt).
    /// SRM's ISignatureTypeProvider.GetModifiedType returns TType, so we encode
    /// the modifier info into the Type itself and strip it later via SignatureUnwrap.
    /// </summary>
    internal sealed class SignatureModifiedType : LmrTypeBase
    {
        internal Type Underlying { get; }
        internal Type Modifier { get; }
        internal bool IsRequired { get; }

        internal SignatureModifiedType(Type underlying, Type modifier, bool isRequired)
        {
            Underlying = underlying ?? throw new ArgumentNullException(nameof(underlying));
            Modifier = modifier ?? throw new ArgumentNullException(nameof(modifier));
            IsRequired = isRequired;
        }

        // Forward all Type virtuals to Underlying so that callers who accidentally
        // use this Type before unwrapping get reasonable behavior.

        public override Assembly Assembly => Underlying.Assembly;
        public override string AssemblyQualifiedName => Underlying.AssemblyQualifiedName;
        public override Type BaseType => Underlying.BaseType;
        public override string FullName => Underlying.FullName;
        public override Guid GUID => Underlying.GUID;
        public override Module Module => Underlying.Module;
        public override string Namespace => Underlying.Namespace;
        public override Type UnderlyingSystemType => Underlying.UnderlyingSystemType;
        public override string Name => Underlying.Name;

        internal override ConstructorInfo[] GetConstructorsCore(BindingFlags bindingAttr) => Underlying.GetConstructorsLmr(bindingAttr);
        public override object[] GetCustomAttributes(bool inherit) => Underlying.GetCustomAttributes(inherit);
        public override object[] GetCustomAttributes(Type attributeType, bool inherit) => Underlying.GetCustomAttributes(attributeType, inherit);
        public override Type GetElementType() => Underlying.GetElementType();
        internal override EventInfo GetEventCore(string name, BindingFlags bindingAttr) => Underlying.GetEventLmr(name, bindingAttr);
        internal override EventInfo[] GetEventsCore(BindingFlags bindingAttr) => Underlying.GetEventsLmr(bindingAttr);
        internal override FieldInfo GetFieldCore(string name, BindingFlags bindingAttr) => Underlying.GetFieldLmr(name, bindingAttr);
        internal override FieldInfo[] GetFieldsCore(BindingFlags bindingAttr) => Underlying.GetFieldsLmr(bindingAttr);
        internal override Type GetInterfaceCore(string name, bool ignoreCase) => Underlying.GetInterfaceLmr(name, ignoreCase);
        internal override Type[] GetInterfacesCore() => Underlying.GetInterfacesLmr();
        internal override MemberInfo[] GetMembersCore(BindingFlags bindingAttr) => Underlying.GetMembersLmr(bindingAttr);
        internal override MethodInfo[] GetMethodsCore(BindingFlags bindingAttr) => Underlying.GetMethodsLmr(bindingAttr);
        internal override Type GetNestedTypeCore(string name, BindingFlags bindingAttr) => Underlying.GetNestedTypeLmr(name, bindingAttr);
        internal override Type[] GetNestedTypesCore(BindingFlags bindingAttr) => Underlying.GetNestedTypesLmr(bindingAttr);
        internal override PropertyInfo[] GetPropertiesCore(BindingFlags bindingAttr) => Underlying.GetPropertiesLmr(bindingAttr);
        internal override object InvokeMemberCore(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters) => Underlying.InvokeMemberLmr(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        public override bool IsDefined(Type attributeType, bool inherit) => Underlying.IsDefined(attributeType, inherit);
        protected override TypeAttributes GetAttributeFlagsImpl() => Underlying.Attributes;
        internal override ConstructorInfo GetConstructorImplCore(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers) => Underlying.GetConstructorLmr(bindingAttr, binder, callConvention, types, modifiers);
        internal override MethodInfo GetMethodImplCore(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers) => Underlying.GetMethodLmr(name, bindingAttr, binder, callConvention, types, modifiers);
        internal override PropertyInfo GetPropertyImplCore(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers) => Underlying.GetPropertyLmr(name, bindingAttr, binder, returnType, types, modifiers);
        protected override bool HasElementTypeImpl() => Underlying.HasElementType;
        protected override bool IsArrayImpl() => Underlying.IsArray;
        protected override bool IsByRefImpl() => Underlying.IsByRef;
        protected override bool IsCOMObjectImpl() => false;
        protected override bool IsPointerImpl() => Underlying.IsPointer;
        protected override bool IsPrimitiveImpl() => Underlying.IsPrimitive;

        public override bool Equals(object obj)
        {
            if (obj is SignatureModifiedType other)
                return Underlying.Equals(other.Underlying);
            return Underlying.Equals(obj);
        }

        public override int GetHashCode() => Underlying.GetHashCode();
    }
}
