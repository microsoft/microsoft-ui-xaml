// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 15.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace XamlGen.Templates.Metadata
{
    using System.Linq;
    using System.Text;
    using System.Collections.Generic;
    using OM;
    using System;
    
    /// <summary>
    /// Class to produce the template output
    /// </summary>
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "15.0.0.0")]
    public partial class StableXbfIndexMetadata : XbfMetadataCodeGenerator<OMContextView>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Copyright>()));
            this.Write("\r\n\r\nusing System;\r\nusing System.Collections.Generic;\r\nusing System.Collections.Ob" +
                    "jectModel;\r\n\r\nnamespace Microsoft.Xaml.WidgetSpinner.Metadata\r\n{\r\n    // Define " +
                    "the stable XBF type indices\r\n    public enum ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write("\r\n    {\r\n");
 foreach (var type in StableIndexes.GetStableTypeIndexes()) { 
            this.Write("        ");
            this.Write(this.ToStringHelper.ToStringWithCulture(type.stableIndexName + " = " + type.stableIndex));
            this.Write(",\r\n");
 } 
            this.Write("    }\r\n\r\n    // Define the stable XBF property indices\r\n    public enum ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfPropertyIndexEnumName));
            this.Write("\r\n    {\r\n");
 foreach (var property in StableIndexes.GetStablePropertyIndexes()) { 
            this.Write("        ");
            this.Write(this.ToStringHelper.ToStringWithCulture(property.stableIndexName + " = " + property.stableIndex));
            this.Write(",\r\n");
 } 
            this.Write("    }\r\n\r\n    // Define the stable XBF event indices\r\n    public enum ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableEventIndexEnumName));
            this.Write("\r\n    {\r\n");
 foreach (var evt in StableIndexes.GetStableEventIndexes()) { 
            this.Write("        ");
            this.Write(this.ToStringHelper.ToStringWithCulture(evt.stableIndexName + " = " + evt.stableIndex));
            this.Write(",\r\n");
 } 
            this.Write(@"    }

    [Flags]
    public enum XamlPropertyFlags
    {
        None = 0x0,
        IsVisualTreeProperty = 0x1,
        IsAttached = 0x2,
    }

    public struct XamlPropertyInfo
    {
        public readonly string FullName;
        public readonly ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(" PropertyTypeIndex;\r\n        public readonly ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(" DeclaringTypeIndex;\r\n        public readonly XamlPropertyFlags Flags;\r\n\r\n       " +
                    " public XamlPropertyInfo(string fullName, ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(" propertyTypeIndex, ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(@" declaringTypeIndex, XamlPropertyFlags flags)
        {
            FullName = fullName;
            PropertyTypeIndex = propertyTypeIndex;
            DeclaringTypeIndex = declaringTypeIndex;
            Flags = flags;
        }
    }

    [Flags]
    public enum XamlTypeFlags
    {
        None = 0x0,
        IsCollection = 0x1,
        IsDictionary = 0x2,
        IsMarkupExtension = 0x4,
    }

    public struct XamlTypeInfo
    {
        public readonly string FullName;
        public readonly ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(" BaseTypeIndex;\r\n        public readonly XamlTypeFlags Flags;\r\n\r\n        public X" +
                    "amlTypeInfo(string fullName, ");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(@" baseTypeIndex, XamlTypeFlags flags)
        {
            FullName = fullName;
            BaseTypeIndex = baseTypeIndex;
            Flags = flags;
        }
    }

    public class StableXbfIndexMetadata
    {
        // Map stable XBF type indices to known type full names
        public static readonly IReadOnlyDictionary<");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(", XamlTypeInfo> StableXbfTypeIndexToTypeInfo = new ReadOnlyDictionary<");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(", XamlTypeInfo>(new Dictionary<");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfTypeIndexEnumName));
            this.Write(", XamlTypeInfo>()\r\n        {\r\n");
 Func<ClassDefinition, bool> classSelector = c => !c.IsInterface && !c.IsAEventArgs; 
 foreach (var type in Model.GetAllTypeTableTypes().Where(t => !(t is ClassDefinition) || classSelector(t as ClassDefinition))) { 
     var stableIndexName = StableIndexes.GetStableTypeIndexNameFromKnownTypeIndexName(type.IndexNameWithoutPrefix); 
     if (stableIndexName != StableIndexes.UnknownTypeName) { 
         var baseTypeIndexName = StableIndexes.UnknownTypeName; 
         var flags = "XamlTypeFlags.None"; 
         var typeAsClass = type as ClassDefinition; 
         if (typeAsClass != null) 
         { 
             baseTypeIndexName = StableIndexes.GetStableTypeIndexNameFromKnownTypeIndexName(typeAsClass.BaseClassInTypeTable.IndexNameWithoutPrefix); 
             var flagsList = new List<string>(); 
             if (typeAsClass.IsCollectionImplementationClass || typeAsClass.XamlClassFlags.IsICollection) { flagsList.Add("XamlTypeFlags.IsCollection"); } 
             if (typeAsClass.XamlClassFlags.IsIDictionary) { flagsList.Add("XamlTypeFlags.IsDictionary"); } 
             if (typeAsClass.XamlClassFlags.IsMarkupExtension) { flagsList.Add("XamlTypeFlags.IsMarkupExtension"); } 
             if (flagsList.Count > 0) { flags = flagsList.Aggregate((left, right) => left + " | " + right); } 
         } 
            this.Write("            ");
            this.Write(this.ToStringHelper.ToStringWithCulture("{ " + StableXbfIndexGenerator.StableXbfTypeIndexEnumName + "." + stableIndexName + ", new XamlTypeInfo(\"" + type.GenericClrFullName + "\", " + StableXbfIndexGenerator.StableXbfTypeIndexEnumName + "." + baseTypeIndexName + ", " + flags + ") },"));
            this.Write("\r\n");
     } 
 } 
            this.Write("        });\r\n\r\n        // Map stable XBF property indices to known properties\r\n  " +
                    "      public static readonly IReadOnlyDictionary<");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfPropertyIndexEnumName));
            this.Write(", XamlPropertyInfo> StableXbfPropertyIndexToPropertyInfo = new ReadOnlyDictionary" +
                    "<");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfPropertyIndexEnumName));
            this.Write(", XamlPropertyInfo>(new Dictionary<");
            this.Write(this.ToStringHelper.ToStringWithCulture(StableXbfIndexGenerator.StableXbfPropertyIndexEnumName));
            this.Write(", XamlPropertyInfo>()\r\n        {\r\n");
 foreach (var property in Model.GetAllTypeTableProperties().Where(p => classSelector(p.DeclaringClass))) { 
     var stableIndexName = StableIndexes.GetStablePropertyIndexNameFromKnownPropertyIndexName(property.IndexNameWithoutPrefix); 
     if (stableIndexName != StableIndexes.UnknownPropertyName) { 
         var propertyTypeIndexName = StableXbfIndexGenerator.StableXbfTypeIndexEnumName + "." + StableIndexes.GetStableTypeIndexNameFromKnownTypeIndexName(property.PropertyType.Type.IndexNameWithoutPrefix); 
         var declaringTypeIndexName = StableXbfIndexGenerator.StableXbfTypeIndexEnumName + "." + StableIndexes.GetStableTypeIndexNameFromKnownTypeIndexName(property.DeclaringType.IndexNameWithoutPrefix); 
         var flags = "XamlPropertyFlags.None"; 
         var flagsList = new List<string>(); 
         if (property.IsVisualTreeProperty) { flagsList.Add("XamlPropertyFlags.IsVisualTreeProperty"); } 
         if (property is AttachedPropertyDefinition) { flagsList.Add("XamlPropertyFlags.IsAttached"); } 
         if (flagsList.Count > 0) { flags = flagsList.Aggregate((left, right) => left + " | " + right); } 
            this.Write("            ");
            this.Write(this.ToStringHelper.ToStringWithCulture("{ " + StableXbfIndexGenerator.StableXbfPropertyIndexEnumName + "." + stableIndexName + ", new XamlPropertyInfo(\"" + property.DescriptiveName + "\", " + propertyTypeIndexName + ", " + declaringTypeIndexName + ", " + flags + ") },"));
            this.Write("\r\n");
     } 
 } 
            this.Write("        });\r\n    }\r\n}\r\n");
            return this.GenerationEnvironment.ToString();
        }
    }
}
