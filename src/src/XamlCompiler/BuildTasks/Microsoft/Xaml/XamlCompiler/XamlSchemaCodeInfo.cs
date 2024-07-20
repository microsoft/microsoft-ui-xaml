// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Linq;
    using System.Xaml;
    using System.Xaml.Schema;
    using CodeGen;
    using DirectUI;

    class XamlSchemaCodeInfo
    {
        class ProjectionDefinition
        {
            public ProjectionDefinition(string cppCXName, string cppWinRTName, bool stripSystemPrefix = true)
            {
                CppCXName = cppCXName;
                CppWinRTName = cppWinRTName;
                StripSystemPrefix = stripSystemPrefix;
            }

            public ProjectionDefinition(string all)
                : this(all, all)
            {}

            public string CppCXName;
            public string CppWinRTName;

            // This is for the newly added platform.winmd projection types from bug 14148158.  Since the types are now in our projection map, we normally strip
            // out their System prefix.  This would normally mean that something like "System.ValueType" is now "ValueType" in C#, which should be fine, but we don't
            // want to potentially break anyone at this stage.  We should look at removing this entirely, and always strip the System prefix,
            // once we're confident stripping out the System prefix doesn't break anything in the framework.
            public bool StripSystemPrefix;
        }

        static Dictionary<String, ProjectionDefinition> _winRtPrimitiveTypeList;

        List<InternalTypeEntry> _typeTable;
        List<InternalXamlUserTypeInfo> _userTypeInfo;
        List<InternalXamlUserMemberInfo> _userMemberInfo;

        bool typeInfoReflectionEnabled;

        public XamlSchemaCodeInfo()
        {
            _typeTable = new List<InternalTypeEntry>();
            _userTypeInfo = new List<InternalXamlUserTypeInfo>();
            _userMemberInfo = new List<InternalXamlUserMemberInfo>();
            typeInfoReflectionEnabled = false;
        }

        private IReadOnlyCollection<TypeForCodeGen> _otherMetadataProviders;
        public IReadOnlyCollection<TypeForCodeGen> OtherMetadataProviders
        {
            get
            {
                if (_otherMetadataProviders == null)
                {
                    _otherMetadataProviders = new List<TypeForCodeGen>();
                }
                return _otherMetadataProviders;
            }
            set
            {
                if (value != _otherMetadataProviders)
                {
                    _otherMetadataProviders = value;
                    foreach (var type in _typeTable)
                    {
                        type.IsHandledByOtherProviders = IsTypeHandledByOtherProviders(type.UnderlyingType, _otherMetadataProviders);
                    }
                }
            }
        }

        public bool TypeInfoReflectionEnabled {
            get
            {
                return typeInfoReflectionEnabled;
            }
            set
            {
                typeInfoReflectionEnabled = value;
            }
        }

        public List<InternalTypeEntry> TypeTableFromAllAssemblies
        {
            get { return _typeTable; }
        }

        private bool ShouldIncludeTypeInTypeTable(InternalTypeEntry type)
        {
            // Bug 19918301 - we used to check if type info reflection was enabled here
            // to only add local types instead of all types, since type info reflection on native still needs local types to be generated.
            // However, this means we wouldn't generate an IXamlMetadataProvider implementation
            // on the app object if the app used remote types but no local types,
            // even though we still needed an IXMP implementation to fetch them.
            /*
            if (TypeInfoReflectionEnabled)
            {
                return type.UserTypeInfo != null && type.UserTypeInfo.IsLocalType;
            }
            */
            if (!type.IsHandledByOtherProviders)
            {
                return true;
            }
            if (type.UserTypeInfo != null && type.UserTypeInfo.IsReturnTypeStub)
            {
                return true;
            }
            return false;
        }

        public IReadOnlyList<InternalTypeEntry> TypeTable
        {
            get { return _typeTable.Where(t => ShouldIncludeTypeInTypeTable(t)).ToList(); }
        }

        public List<InternalXamlUserTypeInfo> UserTypeInfo
        {
            get { return _userTypeInfo.Where(t => ShouldIncludeTypeInTypeTable(t.TypeEntry)).ToList(); }
        }

        public List<InternalXamlUserMemberInfo> UserMemberInfo
        {
            get { return _userMemberInfo; }
        }

        public bool TryFindType(string systemName, out InternalTypeEntry type)
        {
            foreach (InternalTypeEntry ixType in _typeTable)
            {
                if (systemName == ixType.SystemName)
                {
                    type = ixType;
                    return true;
                }
            }
            type = null;
            return false;
        }

        public InternalTypeEntry AddTypeAndProperties(XamlType xamlType)
        {
            InternalTypeEntry typeEntry = AddType(xamlType);
            AddAllCodeGenProperties(xamlType);
            return typeEntry;
        }

        public InternalTypeEntry AddReturnTypeStub(XamlType xamlType)
        {
            return AddType(xamlType, true);
        }

        public InternalTypeEntry AddType(XamlType xamlType, bool isReturnTypeStub = false)
        {
            if (xamlType == null)
            {
                throw new ArgumentNullException("xamlType");
            }

            InternalTypeEntry xamlTypeEntry;
            if (TryFindType(xamlType.UnderlyingType.FullName, out xamlTypeEntry))
            {
                if (xamlTypeEntry.UserTypeInfo != null && !isReturnTypeStub)
                {
                    // If the existing entry was a stub, it isn't a stub now.
                    xamlTypeEntry.UserTypeInfo.IsReturnTypeStub = false;
                }
                return xamlTypeEntry;
            }

            DirectUIXamlType directuiType = xamlType as DirectUIXamlType;
            if (directuiType.IsCodeGenType)
            {
                if (xamlType.UnderlyingType.IsEnum)
                {
                    isReturnTypeStub = false;
                }
                return AddNewUserTypeInfo(xamlType, isReturnTypeStub);
            }
            else
            {
                return AddNewTypeEntry(xamlType);
            }
        }

        public InternalTypeEntry AddBindableType(XamlType type)
        {
            InternalTypeEntry bindableType = AddTypeAndProperties(type);

            if (bindableType.IsSystemType)
            {
                return bindableType;    // Error?
            }
            bindableType.UserTypeInfo.IsBindable = true;

            return bindableType;
        }

        private void AddAllCodeGenProperties(XamlType type)
        {
            // The base class "Enum" in MsCorlib has some static methods:
            // GetNames, GetValues, GetUnderlyingType that look like 
            // attached properties to a XAML system.
            // Enum's can't really have any properties we are interested in.
            if (type.UnderlyingType.IsEnum)
            {
                return;
            }

            foreach (var xamlMember in type.GetAllMembers())
            {
                AddCodeGenProperty(xamlMember);
            }

            foreach (var xamlMember in type.GetAllAttachableMembers())
            {
                AddCodeGenProperty(xamlMember);
            }
        }

        public bool TryFindIndexOfMember(String name, InternalTypeEntry declaringType, out int idx)
        {
            for (int i = 0; i < _userMemberInfo.Count; i++)
            {
                InternalXamlUserMemberInfo member = UserMemberInfo[i];
                if (member.Name == name && member.DeclaringType == declaringType)
                {
                    idx = i;
                    return true;
                }
            }
            idx = -1;
            return false;
        }

        private bool TryFindMember(string name, InternalTypeEntry declaringType, out InternalXamlUserMemberInfo member)
        {
            int idx;

            if (TryFindIndexOfMember(name, declaringType, out idx))
            {
                member = UserMemberInfo[idx];
                return true;
            }
            member = null;
            return false;
        }

        private void AddCodeGenProperty(XamlMember xamlMember)
        {
            if (xamlMember.IsEvent)
            {
                return;
            }

            DirectUIXamlType declaringType = xamlMember.DeclaringType as DirectUIXamlType;
            if (declaringType.IsCodeGenType)
            {
                // Skip any "indexer" properties.   (these in C# are declared as "this[arg]")
                // In reflection they appear as a property called "Item".  Although there is an attribute to change the name.
                // It is perfectly possible to have a ordinary property named "Item" if you don't have an indexer,
                // or have renamed the indexer to a name other than Item.
                DirectUIXamlMember duiMember = xamlMember as DirectUIXamlMember;
                if (duiMember != null)
                {
                    if (duiMember.IsIndexer)
                    {
                        return;  // skip the Item Indexer.
                    }
                }

                InternalTypeEntry declaringTypeEntry = AddType(declaringType);
                AddMember(declaringTypeEntry, xamlMember);
            }
        }

        public InternalXamlUserMemberInfo AddMember(InternalTypeEntry usingType, XamlMember xamlMember, bool declaringTypeAsStub = false)
        {
            if (xamlMember.IsDirective)
            {
                return null;
            }

            InternalTypeEntry declaringType = AddType(xamlMember.DeclaringType, declaringTypeAsStub);
            if (declaringType == null)
            {
                throw new InvalidOperationException("declaring type is null on a non-directive property");
            }

            // Should be able to find the property on the base type.
            if (declaringType.IsSystemType)
            {
                return null;
            }

            DirectUIXamlMember duiMember = (DirectUIXamlMember)xamlMember;
            if (duiMember.IsEvent || duiMember.IsHardDeprecated)
            {
                return null;
            }
            DirectUIXamlType duiReturnType = (DirectUIXamlType)xamlMember.Type;
            if (duiReturnType.IsHardDeprecated)
            {
                return null;
            }

            // Add the Type to the master Member table.
            InternalXamlUserMemberInfo userMember;
            if (TryFindMember(xamlMember.Name, declaringType, out userMember))
            {
                return userMember;
            }
            userMember = new InternalXamlUserMemberInfo();
            userMember.Name = xamlMember.Name;
            userMember.DeclaringType = declaringType;

            // Set the Name and DeclaringType and add to the list of members.
            // Do this before the Init() of the other fields of the member
            // because adding Types and Members is mutually recursive.
            // Initializing a member, will add it's declaring type's base type, and 
            // any Generic Type arguments.  Each of those types will have their properties
            // added.
            // Put the member in the table so it can be found as "already present" before the Init.
            UserMemberInfo.Add(userMember);
            declaringType.UserTypeInfo.AddMember(userMember, declaringType, this);

            userMember.Init(xamlMember, this);
            return userMember;
        }

        private InternalTypeEntry AddNewTypeEntry(XamlType xamlType)
        {
            InternalTypeEntry iXamlType = new InternalTypeEntry(xamlType);
            iXamlType.TypeIndex = _typeTable.Count;
            iXamlType.IsHandledByOtherProviders = IsTypeHandledByOtherProviders(xamlType.UnderlyingType, OtherMetadataProviders);
            _typeTable.Add(iXamlType);
            return iXamlType;
        }

        private InternalTypeEntry AddNewUserTypeInfo(XamlType xamlType, bool isReturnTypeStub)
        {
            InternalTypeEntry newEntry = AddNewTypeEntry(xamlType);
            newEntry.UserTypeInfo = new InternalXamlUserTypeInfo(newEntry, this);
            newEntry.UserTypeInfo.IsReturnTypeStub = isReturnTypeStub;
            _userTypeInfo.Add(newEntry.UserTypeInfo);

            newEntry.UserTypeInfo.Init(xamlType);
            return newEntry;
        }

        internal static String GetFullGenericNestedName(Type type, bool setAirity)
        {
            return GetFullGenericNestedName(type, ProgrammingLanguage.IdlWinRT, false, setAirity);
        }

        public static String GetFullGenericNestedName(Type type, string programmingLanguage, bool globalized)
        {
            return GetFullGenericNestedName(type, programmingLanguage, globalized, true);
        }

        // Turn this off for 8.0 code gen compat (we still need this because of 8.0 SDK's)
        public static bool SetAirityOnGenericTypeNames = true;

        public static bool GetGlobalizedFullNameForCppRefType(Type type, out string fullName)
        {
            bool isArray = false;
            fullName = null;

            if(!type.IsByRef)
            {
                return false;
            }

            Debug.Assert(type.FullName.EndsWith("&"), "Winrt ByRef type name doesn't end with '&'");

            string name = type.FullName.Substring(0, type.FullName.Length - 1);
            // We cannot rely on Type.IsArray metadata as it will be false even for "System.Int32[]&"
            if(name.EndsWith("[]"))
            {
                isArray = true;
                // Assuming WinRT arrays are strictly one dimensional, we take the rest of typename as the elementType 
                name = name.Substring(0, name.Length - 2);
            }

            name = ProjectionNameTranslation(name, ProgrammingLanguage.CppCX);

            fullName = isArray ? "::Platform::Array<" + name + ">" :  "::" + name;

            return true;
        }

        public static String GetFullGenericNestedName(Type type, string programmingLanguage, bool globalized, bool setAirity)
        {
            string arraySuffix;
            // if it isn't an array this does nothing.
            type = GetArrayElementType(type, out arraySuffix, programmingLanguage);

            string typeName = FixNestedTypeName(type);
            string globalizerToken = "";
            string simpleTypeName = ProjectionNameTranslation(typeName, programmingLanguage);

            if (globalized)
            {
                switch (programmingLanguage)
                {
                    case ProgrammingLanguage.IdlWinRT:
                        globalizerToken = "";
                        break;
                    case ProgrammingLanguage.CSharp:
                        globalizerToken = "global::";
                        break;

                    case ProgrammingLanguage.VB:
                        globalizerToken = "Global.";
                        break;

                    case ProgrammingLanguage.CppCX:
                        globalizerToken = "::";
                        break;

                    case ProgrammingLanguage.CppWinRT:
                        if (!WinRtPrimitiveTypesForProjection.ContainsKey(typeName))
                        {
                            // For CppWinRT, only prefix with ::winrt if the type is not a fundamental type
                            globalizerToken = "::winrt::";
                        }
                        break;

                    default:
                        throw new ArgumentException("programmingLanguage");
                }
            }
            if (!type.IsGenericType)
            {
                return globalizerToken + simpleTypeName + arraySuffix;
            }

            string OpenTypeParameters = "<";
            string CloseTypeParameters = ">";
            string AiritySpecifier = String.Empty;
            string AfterNonValueTypes = String.Empty;

            switch (programmingLanguage)
            {
                case ProgrammingLanguage.IdlWinRT:
                    AiritySpecifier = "`{0}";
                    break;

                case ProgrammingLanguage.CSharp:
                    break;

                case ProgrammingLanguage.VB:
                    OpenTypeParameters = "(Of ";
                    CloseTypeParameters = ")";
                    break;

                case ProgrammingLanguage.CppCX:
                    AfterNonValueTypes = "^";
                    break;

                case ProgrammingLanguage.CppWinRT:
                    break;

                default:
                    throw new ArgumentException("programmingLanguage");
            }
            if (setAirity == false)
            {
                AiritySpecifier = String.Empty;
            }

            Type[] typeArguments = type.GetGenericArguments();
            Debug.Assert(typeArguments.Length > 0);

            // Nested Generics AAA<X,Y>.BBB<Z>
            // Will Look like "AAA`2.BBB`1" with a list of X, Y, Z type arguments.

            int iTypeArgument = 0;
            string[] nestedGenericParts = simpleTypeName.Split('`');
            string result = nestedGenericParts[0];

            for (int iParts = 1; iParts < nestedGenericParts.Length; iParts++)
            {
                string part = nestedGenericParts[iParts];
                string remainder;
                int argCount = CountTypeArgs(part, out remainder);

                if (!String.IsNullOrEmpty(AiritySpecifier))
                {
                    result += String.Format(CultureInfo.InvariantCulture, AiritySpecifier, argCount);
                }
                result += OpenTypeParameters;

                // Distribuite the next "argCount" type parameters.
                // eg: < X , Y >
                for (int iArg = 0; iArg < argCount; iArg++)
                {
                    Type argType = typeArguments[iTypeArgument + iArg];
                    string genericTypeName = GetFullGenericNestedName(argType, programmingLanguage, true);
                    if (!argType.IsValueType)
                    {
                        genericTypeName += AfterNonValueTypes;
                    }
                    result += genericTypeName;

                    if (iArg + 1 < argCount)
                    {
                        result += ", ";
                    }
                }
                result += CloseTypeParameters;
                result += remainder;
                iTypeArgument += argCount;
            }

            return globalizerToken + result + arraySuffix;
        }

        internal static string FixNestedTypeName(Type type)
        {
            string name = String.Empty;
            if (type.IsGenericType)
            {
                type = type.GetGenericTypeDefinition();
            }

            if (type.IsNested)
            {
                string declaringTypeName = FixNestedTypeName(type.DeclaringType);
                name = declaringTypeName + "." + type.Name;
            }
            else
            {
                name = type.FullName;
            }
            return name;
        }

        internal static Type GetArrayElementType(Type type, out string suffix, string programmingLanguage)
        {
            suffix = string.Empty;
            if (!type.IsArray)
            {
                return type;
            }
            suffix += (programmingLanguage == ProgrammingLanguage.VB) ? "(" : "[";
            for (int i = 1; i < type.GetArrayRank(); i++)
            {
                suffix += ",";
            }
            suffix += (programmingLanguage == ProgrammingLanguage.VB) ? ")" : "]";

            Type elementType = type.GetElementType();

            string more;
            Type retType = GetArrayElementType(elementType, out more, programmingLanguage);

            // this looks backward, but actually the reflection vs. language texts are backward.
            suffix += more;
            return retType;
        }

        public static XamlTypeName GetXamlTypeNameFromFullName(string typeFullName)
        {
            var typeName = new ClassName(typeFullName);
            return new XamlTypeName(KnownStrings.UsingPrefix + typeName.Namespace, typeName.ShortName);
        }

        // namePart is passed a piece of a string that starts with number
        // followed by a '.', "::" or end of string.
        static int CountTypeArgs(string namePart, out string remainder)
        {
            char[] dot = new char[] { '.', ':' };
            string[] split = namePart.Split(dot, 2);
            if (split.Length == 1)
            {
                remainder = String.Empty;
            }
            else if (split[1][0] == ':')
            {
                remainder = ":" + split[1];
            }
            else
            {
                remainder = '.' + split[1];
            }
            return Int32.Parse(split[0]);
        }

        // According to MSDN: https://docs.microsoft.com/en-us/cpp/cppcx/fundamental-types-c-cx
        private static IDictionary<String, ProjectionDefinition> WinRtPrimitiveTypesForProjection
        {
            get
            {
                if (_winRtPrimitiveTypeList == null)
                {
                    _winRtPrimitiveTypeList = new Dictionary<string, ProjectionDefinition>();

                    _winRtPrimitiveTypeList.Add("System.Byte", new ProjectionDefinition("default::uint8", "uint8_t"));
                    _winRtPrimitiveTypeList.Add("System.UInt8", new ProjectionDefinition("default::uint8", "uint8_t"));

                    _winRtPrimitiveTypeList.Add("System.SByte", new ProjectionDefinition("default::int8", "int8_t"));  // signed byte is illegal in WinRT
                    _winRtPrimitiveTypeList.Add("System.Int8", new ProjectionDefinition("default::int8", "int8_t"));  // signed byte is illegal in WinRT

                    _winRtPrimitiveTypeList.Add("System.Char", new ProjectionDefinition("default::char16", "wchar_t"));
                    _winRtPrimitiveTypeList.Add("System.Char16", new ProjectionDefinition("default::char16", "wchar_t"));

                    _winRtPrimitiveTypeList.Add("System.Single", new ProjectionDefinition("default::float32", "float"));
                    _winRtPrimitiveTypeList.Add("System.Double", new ProjectionDefinition("default::float64", "double"));
                    _winRtPrimitiveTypeList.Add("System.Int16", new ProjectionDefinition("default::int16", "int16_t"));
                    _winRtPrimitiveTypeList.Add("System.Int32", new ProjectionDefinition("default::int32", "int32_t"));
                    _winRtPrimitiveTypeList.Add("System.Int64", new ProjectionDefinition("default::int64", "int64_t"));
                    _winRtPrimitiveTypeList.Add("System.UInt16", new ProjectionDefinition("default::uint16", "uint16_t"));
                    _winRtPrimitiveTypeList.Add("System.UInt32", new ProjectionDefinition("default::uint32", "uint32_t"));
                    _winRtPrimitiveTypeList.Add("System.UInt64", new ProjectionDefinition("default::uint64", "uint64_t"));

                    _winRtPrimitiveTypeList.Add("System.Boolean", new ProjectionDefinition("Platform::Boolean", "bool"));
                    _winRtPrimitiveTypeList.Add("System.String", new ProjectionDefinition("Platform::String", "::winrt::hstring"));
                    _winRtPrimitiveTypeList.Add("System.Object", new ProjectionDefinition("Platform::Object", "::winrt::Windows::Foundation::IInspectable"));
                    _winRtPrimitiveTypeList.Add("System.Guid", new ProjectionDefinition("Platform::Guid", "GUID"));
                    _winRtPrimitiveTypeList.Add("Windows.Foundation.IReference`1", new ProjectionDefinition("Platform::IBox`1", "::winrt::Windows::Foundation::IReference`1"));
                    _winRtPrimitiveTypeList.Add("System.TimeSpan", new ProjectionDefinition("Windows::Foundation::TimeSpan", "::winrt::Windows::Foundation::TimeSpan"));

                    // TODO: Bug 15391946 - add CppWinRT values and tests
                    _winRtPrimitiveTypeList.Add("System.Type", new ProjectionDefinition("Platform::Type", null, false));
                    _winRtPrimitiveTypeList.Add("System.Enum", new ProjectionDefinition("Platform::Enum", null, false));
                    _winRtPrimitiveTypeList.Add("System.ValueType", new ProjectionDefinition("Platform::ValueType", null, false));
                    _winRtPrimitiveTypeList.Add("System.UIntPtr", new ProjectionDefinition("Platform::UIntPtr", null, false));

                    _winRtPrimitiveTypeList.Add("Windows.Foundation.Numerics.Vector2", new ProjectionDefinition("Windows::Foundation::Numerics::float2", "::winrt::Windows::Foundation::Numerics::float2"));
                    _winRtPrimitiveTypeList.Add("Windows.Foundation.Numerics.Vector3", new ProjectionDefinition("Windows::Foundation::Numerics::float3", "::winrt::Windows::Foundation::Numerics::float3"));
                    _winRtPrimitiveTypeList.Add("Windows.Foundation.Numerics.Vector4", new ProjectionDefinition("Windows::Foundation::Numerics::float4", "::winrt::Windows::Foundation::Numerics::float4"));
                    _winRtPrimitiveTypeList.Add("Windows.Foundation.Numerics.Plane", new ProjectionDefinition("Windows::Foundation::Numerics::Plane", "::winrt::Windows::Foundation::Numerics::Plane"));
                    _winRtPrimitiveTypeList.Add("Windows.Foundation.Numerics.Quaternion", new ProjectionDefinition("Windows::Foundation::Numerics::quaternion", "::winrt::Windows::Foundation::Numerics::quaternion"));
                    _winRtPrimitiveTypeList.Add("Windows.Foundation.Numerics.Matrix3x2", new ProjectionDefinition("Windows::Foundation::Numerics::float3x2", "::winrt::Windows::Foundation::Numerics::float3x2"));
                    _winRtPrimitiveTypeList.Add("Windows.Foundation.Numerics.Matrix4x4", new ProjectionDefinition("Windows::Foundation::Numerics::float4x4", "::winrt::Windows::Foundation::Numerics::float4x4"));

                }
                return _winRtPrimitiveTypeList;
            }
        }

        // Returns true if the input type is a primitive with a projection provided
        // by the language/project system when used in C++, e.g. 'System.String' or 'System.Int32'
        public static bool IsProjectedPrimitiveCppType(string typeFullName)
        {
            return WinRtPrimitiveTypesForProjection.ContainsKey(typeFullName);
        }

        private static string ProjectionNameTranslation(string simpleTypeName, string programmingLanguage)
        {
            ProjectionDefinition projectionName;
            WinRtPrimitiveTypesForProjection.TryGetValue(simpleTypeName, out projectionName);

            switch (programmingLanguage)
            {
                case ProgrammingLanguage.IdlWinRT:
                    if (projectionName != null && projectionName.StripSystemPrefix && simpleTypeName.StartsWith(KnownNamespaces.SystemPrefix))
                    {
                        return simpleTypeName.Remove(0, KnownNamespaces.SystemPrefix.Length);
                    }
                    return simpleTypeName;

                case ProgrammingLanguage.CppCX:
                    return projectionName != null ? projectionName.CppCXName : KnownStrings.Colonize(simpleTypeName);

                case ProgrammingLanguage.CppWinRT:
                    return projectionName != null ? projectionName.CppWinRTName : KnownStrings.Colonize(simpleTypeName);

                default:
                    return simpleTypeName;
            }
        }

        private static bool IsTypeHandledByOtherProviders(Type type, IEnumerable<TypeForCodeGen> otherProviders)
        {
            if (otherProviders != null)
            {
                foreach (var ixmpType in otherProviders)
                {
                    if (ixmpType.UnderlyingType.Assembly.FullName == type.Assembly.FullName)
                    {
                        return ixmpType.UnderlyingType.HasFullXamlMetadataProviderAttribute();
                    }
                }
            }
            return false;
        }
    }
}
