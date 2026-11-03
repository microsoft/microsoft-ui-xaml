// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Windows.Markup;
using System.Xaml;
using System.Xaml.MS.Impl;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// The closed set of runtime types (as opposed to LMR types, read from metadata) that the XAML compiler can reflect over,
    /// through <see cref="LmrReflection"/>.
    /// </summary>
    /// <remarks>
    /// <para>
    /// Every type the compiler reflects over is an LMR type, except the built-in types of the XAML language
    /// (see <see cref="XamlLanguage"/>): their <see cref="XamlType"/>s are created from the real runtime types
    /// of the compiler's own assembly and of the BCL, and the System.Xaml schema code (<c>TypeReflector</c>,
    /// <c>CollectionReflector</c>, <see cref="XamlType"/>...) reflects over them like over any other type.
    /// </para>
    /// <para>
    /// Each type here is stored in a location annotated with <see cref="DynamicallyAccessedMembersAttribute"/>
    /// (<see cref="DynamicallyAccessedMemberTypes.All"/>), from a <c>typeof</c> literal: the trim analyzers verify
    /// that statically, and ILC keeps the members of exactly these types for reflection. Any other runtime type
    /// is rejected with a <see cref="NotSupportedException"/> naming it (never with a silently wrong result): add
    /// it here if the compiler legitimately needs it, with a comment saying why.
    /// </para>
    /// </remarks>
    internal static class KnownRuntimeTypes
    {
        private const DynamicallyAccessedMemberTypes All = DynamicallyAccessedMemberTypes.All;

        private sealed class Entry
        {
            [DynamicallyAccessedMembers(All)]
            public readonly Type Type;

            public Entry([DynamicallyAccessedMembers(All)] Type type)
            {
                Type = type;
            }
        }

        private static readonly Dictionary<Type, Entry> s_entries = CreateEntries();

        private static Dictionary<Type, Entry> CreateEntries()
        {
            Dictionary<Type, Entry> entries = new Dictionary<Type, Entry>();

            // The underlying types of the built-in XAML language types (see XamlLanguage). System.Xaml reflects over them when
            // it's asked about the schema of these XamlTypes (members, constructors, collection kind...). In the regression
            // projects, only System.Object is actually reflected over (by CollectionReflector.LookupCollectionKind, to find out
            // whether it's a collection); the others are kept so that every XamlType System.Xaml creates from a runtime type
            // works the same as in the other builds of the compiler.
            // Not included: System.Uri (x:Uri), whose nested enum types would make ILC keep Enum.GetValues(Type), which needs
            // dynamic code (IL3050). No regression project uses x:Uri, and reflecting over it throws a NotSupportedException.
            Add(entries, typeof(ArrayExtension));
            Add(entries, typeof(NullExtension));
            Add(entries, typeof(Reference));
            Add(entries, typeof(StaticExtension));
            Add(entries, typeof(TypeExtension));
            Add(entries, typeof(string));
            Add(entries, typeof(double));
            Add(entries, typeof(int));
            Add(entries, typeof(bool));
            Add(entries, typeof(MemberDefinition));
            Add(entries, typeof(PropertyDefinition));
            Add(entries, typeof(XData));
            Add(entries, typeof(object));
            Add(entries, typeof(List<object>));
            Add(entries, typeof(List<MemberDefinition>));
            Add(entries, typeof(List<Attribute>));
            Add(entries, typeof(MarkupExtension));
            Add(entries, typeof(INameScope));
            Add(entries, typeof(System.Xml.Serialization.IXmlSerializable));
            Add(entries, typeof(PositionalParameterDescriptor));
            Add(entries, typeof(char));
            Add(entries, typeof(float));
            Add(entries, typeof(byte));
            Add(entries, typeof(short));
            Add(entries, typeof(long));
            Add(entries, typeof(decimal));
            Add(entries, typeof(TimeSpan));

            // What System.Xaml reflects over when it reflects over the types above: the ICollection<T> implemented by the
            // List<T> types (CollectionReflector gets their Add method), and the base type of the value types (XamlType and
            // LooseTypeExtensions walk base types). The other base types (Object, MarkupExtension, MemberDefinition) are above.
            Add(entries, typeof(ICollection<object>));
            Add(entries, typeof(ICollection<MemberDefinition>));
            Add(entries, typeof(ICollection<Attribute>));
            Add(entries, typeof(ValueType));

            return entries;
        }

        private static void Add(Dictionary<Type, Entry> entries, [DynamicallyAccessedMembers(All)] Type type)
        {
            entries.Add(type, new Entry(type));
        }

        /// <summary>
        /// Gets the same runtime type, statically known to have all its members available for reflection.
        /// </summary>
        /// <param name="type">The runtime type to reflect over.</param>
        /// <param name="member">The reflection member about to be used, for the exception message.</param>
        /// <exception cref="NotSupportedException"><paramref name="type"/> isn't one of the known runtime types.</exception>
        [return: DynamicallyAccessedMembers(All)]
        public static Type Get(Type type, string member)
        {
            if (type == null)
            {
                // What calling the reflection member on null does
                throw new NullReferenceException();
            }

            if (s_entries.TryGetValue(type, out Entry entry))
            {
                return entry.Type;
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture,
                "{0} isn't supported on '{1}' ({2}) by the XAML compiler, which can only reflect over the types it reads from metadata and the runtime types listed in {3}.",
                member, type, type.GetType().FullName, nameof(KnownRuntimeTypes)));
        }

        /// <summary>
        /// Replaces <see cref="Type.GetType(string)"/>, which System.Xaml uses to resolve the type names in some attributes
        /// (string arguments of TypeConverterAttribute and XamlDeferLoadAttribute) as runtime types.
        /// </summary>
        /// <remarks>
        /// Loading types by name isn't supported by Native AOT, and the compiler doesn't need it: it isn't called at all when
        /// compiling the regression projects (the DirectUI XAML types look up their type converters themselves, and the known
        /// runtime types have no such attribute with a string argument). So this always throws, rather than returning null for
        /// a type that the other builds of the compiler would load.
        /// </remarks>
        public static Type GetType(string typeName)
        {
            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture,
                "Type.GetType(\"{0}\") isn't supported by the XAML compiler, which can only reflect over the types it reads from metadata and the runtime types listed in {1}.",
                typeName, nameof(KnownRuntimeTypes)));
        }
    }
}
