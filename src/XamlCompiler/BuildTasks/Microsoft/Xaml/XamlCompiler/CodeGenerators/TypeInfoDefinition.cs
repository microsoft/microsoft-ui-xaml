// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class TypeInfoDefinition : Definition
    {
        private List<MemberGenInfo> _memberInfos = new List<MemberGenInfo>();
        private Dictionary<String, int> _typeInfoIndexes;
        private UInt32[] _typeInfoLookup;
        private List<EnumGenInfo> _enumValues = new List<EnumGenInfo>();
        private HashSet<string> _neededCppWinRTProjectionHeaderFiles;

        public TypeInfoDefinition(XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo)
            : base(projectInfo, schemaInfo)
        {
        }

        internal ClassName AppXamlInfo { get; set; }

        public bool GenerateTypeInfo
        {
            get
            {
                return SchemaInfo.UserTypeInfo.Count != 0 || ProjectInfo.EnableTypeInfoReflection;
            }
        }

        public IEnumerable<String> AllLocalXamlHeaderFiles
        {
            get
            {
                foreach (var additionalHeader in ProjectInfo.AdditionalXamlTypeInfoIncludes)
                {
                    yield return additionalHeader.ItemSpec;
                }
                foreach (String headerFile in ProjectInfo.ClassToHeaderFileMap.Values)
                {
                    yield return headerFile;
                }
            }
        }

        public IEnumerable<String> AllLocalHppGeneratedFiles
        {
            get
            {
                foreach (var headerFile in ProjectInfo.ClassToHeaderFileMap.Values)
                {
                    // Ex: MainPage.xaml.h (CX) or MainPage.h (C++\WinRT)
                    var fileName = headerFile;

                    // Remove the extension, which is usually .h (but may be anything, really)
                    var extension = System.IO.Path.GetExtension(fileName);
                    if (!string.IsNullOrEmpty(extension))
                    {
                        fileName = fileName.Remove(fileName.Length - extension.Length);
                    }

                    // Now, remove a potential .xaml suffix (most times present before the extension)
                    if (fileName.EndsWith(KnownStrings.XamlExtension))
                    {
                        fileName = fileName.Remove(fileName.Length - KnownStrings.XamlExtension.Length);
                    }

                    // Append a new "hpp" extension to the bare file name we now have.
                    yield return fileName + KnownStrings.GeneratedHppExtension;
                }
            }
        }

        /// <summary>
        /// Gets the app's namespace - this is valid in Pass 2, so it's most useful for VB/C# typeinfo structure which occurs all in Pass 2.
        /// This is invalid for non-app codegen.
        /// Used for generating the app's implementation of IXamlMetadataProvider in VB/C#.  In VB/C# this is done in Pass 2 TypeInfo
        /// so we need this information, for C++/CX it's done in the Pass 1 App codegen so we don't use it there.
        /// This should never change, as we have a contract with Visual Studio.
        /// If a developer does not have an App.xaml as an MSBuild ApplicationDefinition item and is handwriting their App and IXamlMetadataProvider implementation,
        /// this can be null.
        /// </summary>
        public string AppMetadataProviderNamespace
        {
            get
            {
                Debug.Assert(!ProjectInfo.IsLibrary);
                return AppXamlInfo?.Namespace;
            }
        }

        //
        // Properties for building the TypeInfos table
        //

        public Dictionary<String, int> TypeInfoIndexes
        {
            get
            {
                if (_typeInfoIndexes == null)
                {
                    int index = 0;
                    _typeInfoIndexes = new Dictionary<string, int>();
                    foreach (var entry in TypeInfos)
                    {
                        _typeInfoIndexes.Add(entry.StandardName, index++);
                    }
                }
                return _typeInfoIndexes;
            }
        }

        public IEnumerable<UInt32> TypeInfoLookup
        {
            get
            {
                if (this._typeInfoLookup == null)
                {
                    int min = this.TypeInfos.Min(x => x.StandardName.Length);
                    int max = this.TypeInfos.Max(x => x.StandardName.Length) + 1;
                    _typeInfoLookup = new UInt32[max + 1];
                    UInt32 typeIndex = 0;
                    foreach (var entry in this.TypeInfos)
                    {
                        int lookupIndex = entry.StandardName.Length;
                        if (_typeInfoLookup[lookupIndex] == 0 && lookupIndex != min)
                        {
                            // Only update entries that are not initialized
                            // And don't update the lookup index for the type with the smallest length, it will always be zero.
                            _typeInfoLookup[lookupIndex] = typeIndex;
                        }
                        typeIndex++;
                    }
                    _typeInfoLookup[max] = typeIndex;

                    // Back populate entries that did not get a type length entry
                    for (int i = max - 1; i > min; i--)
                    {
                        if (_typeInfoLookup[i] == 0)
                        {
                            _typeInfoLookup[i] = typeIndex;
                        }
                        else
                        {
                            typeIndex = _typeInfoLookup[i];
                        }
                    }
                }
                return _typeInfoLookup;
            }
        }

        //
        // Properties for building the MemberInfos table
        //

        public IEnumerable<MemberGenInfo> MemberInfos
        {
            get { return _memberInfos; }
        }

        public void TrackTypeMembers(TypeGenInfo entry, out int startIndex)
        {
            startIndex = _memberInfos.Count;
            foreach (var memberInfo in entry.Members)
            {
                _memberInfos.Add(memberInfo);
            }
        }

        //
        // Properties for building the EnumValues table
        //

        public IEnumerable<EnumGenInfo> EnumValues
        {
            get { return _enumValues; }
        }

        public void TrackTypeEnumValues(TypeGenInfo entry, out int startIndex)
        {
            // startIndex contains the index where we're going to add the first enum value
            startIndex = _enumValues.Count;
            foreach (String eValue in entry.EnumValues)
            {
                _enumValues.Add(new EnumGenInfo(entry, eValue));
            }
        }

        //
        // Properties for building the type/member delegate functions
        //

        public IEnumerable<String> AttachableMemberGetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicGetter && x.IsAttachable).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> ValueTypeMemberGetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicGetter && !x.IsAttachable && x.IsValueType).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> StringGetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicGetter && !x.IsAttachable && !x.IsValueType && x.IsString).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> ReferenceTypeMemberGetterUniqueNamesNoStrings
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicGetter && !x.IsAttachable && !x.IsValueType && !x.IsString).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> ReferenceTypeMemberGetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicGetter && !x.IsAttachable && !x.IsValueType).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> AttachableMemberSetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicSetter && x.IsAttachable).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> EnumTypeMemberSetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicSetter && !x.IsAttachable && x.IsValueType && x.IsEnum).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> ValueTypeMemberSetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicSetter && !x.IsAttachable && x.IsValueType && !x.IsEnum).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> StringSetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicSetter && !x.IsAttachable && !x.IsValueType && x.IsString).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> ReferenceTypeMemberSetterUniqueNames
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicSetter && !x.IsAttachable && !x.IsValueType).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<String> ReferenceTypeMemberSetterUniqueNamesNoStrings
        {
            get
            {
                return SchemaInfo.UserMemberInfo.Where(x => x.HasPublicSetter && !x.IsAttachable && !x.IsValueType && !x.IsString).Select(x => x.Name).Distinct();
            }
        }

        public IEnumerable<string> NeededCppWinRTProjectionHeaderFiles
        {
            get
            {
                if (_neededCppWinRTProjectionHeaderFiles == null)
                {
                    _neededCppWinRTProjectionHeaderFiles = LookupNeededCppWinRTProjectionHeaderFiles();
                }
                return _neededCppWinRTProjectionHeaderFiles;
            }
        }

        private HashSet<string> LookupNeededCppWinRTProjectionHeaderFiles()
        {
            var headers = new HashSet<string>();

            void addCppWinRTHeaderForTypeIfNecessary(Type type)
            {
                if (type != null && !XamlSchemaCodeInfo.IsProjectedPrimitiveCppType(type.FullName))
                {
                    headers.Add($"winrt/{type.Namespace}.h");
                }
            }

            foreach (var typeInfo in this.TypeInfos)
            {
                bool hasGeneratedCodeReference = typeInfo.HasActivator || typeInfo.IsCollection || typeInfo.IsDictionary || typeInfo.HasEnumValues;

                if (hasGeneratedCodeReference)
                {
                    addCppWinRTHeaderForTypeIfNecessary(typeInfo.TypeEntry?.UnderlyingType);
                }
            }

            return headers;
        }

        public String GetterName(int i)
        {
            InternalXamlUserMemberInfo entry = SchemaInfo.UserMemberInfo[i];
            String name = "get_" + i.ToString() + "_" + entry.DeclaringType.Name + "_" + entry.Name;
            return name;
        }

        public String SetterName(int i)
        {
            InternalXamlUserMemberInfo entry = SchemaInfo.UserMemberInfo[i];
            String name = "set_" + i.ToString() + "_" + entry.DeclaringType.Name + "_" + entry.Name;
            return name;
        }

        public String ActivatorName(InternalXamlUserTypeInfo entry)
        {
            return "Activate_" + entry.TypeIndex.ToString() + "_" + entry.Name;
        }

        public String VectorAddName(InternalXamlUserTypeInfo entry)
        {
            if (entry.IsCollection)
            {
                String name = "VectorAdd_" + entry.TypeIndex.ToString() + "_" + entry.Name;
                return name;
            }
            return String.Empty;
        }

        public String MapAddName(InternalXamlUserTypeInfo entry)
        {
            if (entry.IsDictionary)
            {
                String name = "MapAdd_" + entry.TypeIndex.ToString() + "_" + entry.Name;
                return name;
            }
            return String.Empty;
        }
    }
}
