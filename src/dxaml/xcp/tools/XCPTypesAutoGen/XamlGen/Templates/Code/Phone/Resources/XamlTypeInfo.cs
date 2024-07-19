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
namespace XamlGen.Templates.Code.Phone.Resources
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
    public partial class XamlTypeInfo : PhoneCppCodeGenerator<OMContextView>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write(this.ToStringHelper.ToStringWithCulture(IncludeTemplate<Copyright>()));
            this.Write("\r\n\r\n#include \"XamlTypeInfo.h\"\r\n#include \"XamlTypeInfo.g.h\"\r\n\r\n#define TRUE 1\r\n#de" +
                    "fine FALSE 0\r\n#define NULL 0\r\n#define nullptr 0\r\n#define EnumBoxer_NULL 0\r\n#defi" +
                    "ne AddToVector_NULL 0\r\n#define AddToMap_NULL 0\r\n\r\nXAMLSTRING XAMLSTRINGS\r\n{\r\n");
  foreach(var entry in QueryHelper.GetPhoneXamlStrings(Model)) { 
            this.Write("    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(entry.Key.Length));
            this.Write(",\r\n    L\"");
            this.Write(this.ToStringHelper.ToStringWithCulture(entry.Key));
            this.Write("\",\r\n    0,\r\n");
  } 
            this.Write("}\r\n\r\nXAMLTYPENAME XAMLTYPENAMES\r\n{\r\n");
  foreach (var t in QueryHelper.GetPhoneXamlTypes(Model))
    { 
            this.Write("    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsResourceIndex(t.MetadataName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsResourceKey(t.MetadataFullName)));
            this.Write(", ");
            this.Write(this.ToStringHelper.ToStringWithCulture(t.IsPhoneSystemType ? "SC_SYSTEMTYPE" : t.PhoneTypeTableIndex.ToString()));
            this.Write(",\r\n");
  } 
            this.Write("}\r\n\r\nUSERTYPEINFO USERTYPES\r\n{\r\n");
  foreach (var t in QueryHelper.GetPhoneXamlUserTypes(Model))
    {
        ClassDefinition typeAsClass = t as ClassDefinition;
        EnumDefinition typeAsEnum = t as EnumDefinition;
        
            this.Write("\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsResourceIndex(t.MetadataName)));
            this.Write(",\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsResourceKey(t.MetadataName)));
            this.Write(",\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(typeAsClass != null ? typeAsClass.ContentPropertyInTypeTable.PhoneMemberTableIndex : -1));
            this.Write(", // Content Property\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(typeAsClass != null && typeAsClass.BaseClass != null  &&  !typeAsClass.BaseClass.IsObjectType ? AsResourceIndex(typeAsClass.BaseClass.Name) : "-1"));
            this.Write(", // Base Type\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(typeAsEnum != null ? typeAsEnum.PhoneEnumIndex : -1));
            this.Write(", // iEnumIndex\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(0 * 0x01));
            this.Write("  + // isArray\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(typeAsClass?.IsVector == true ? 2 : 0));
            this.Write("  + // isCollection\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(!t.IsValueType && typeAsClass?.IsCreateableFromXAML == true ? 4 : 0));
            this.Write("  + // isConstructable\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(typeAsClass?.IsMap == true ? 8 : 0));
            this.Write(" + // isDictionary\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(0 * 0x10));
            this.Write(", // isMarkupExtension\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(t.PhoneMemberTableIndex));
            this.Write("L,  // Index into the MemberIndices[] table\r\n    ");
 if(!t.IsValueType && typeAsClass?.IsCreateableFromXAML == true) { 
            this.Write("activate_");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsCppDefine(t.MetadataName)));
 } else { 
            this.Write("nullptr");
 }
            this.Write(", // Activator\r\n    EnumBoxer_");
            this.Write(this.ToStringHelper.ToStringWithCulture(typeAsEnum != null ? AsCppDefine(typeAsEnum.MetadataFullName) : "NULL"));
            this.Write(",  // enumBoxer\r\n    AddToVector_");
            this.Write(this.ToStringHelper.ToStringWithCulture(typeAsClass?.IsVector == true ? AsCppDefine(t.MetadataFullName) : "NULL"));
            this.Write(",  // addToVector\r\n    AddToMap_");
            this.Write(this.ToStringHelper.ToStringWithCulture(typeAsClass?.IsMap == true ? AsCppDefine(t.MetadataFullName) : "NULL"));
            this.Write(",  // addToMap\r\n");
  } 
            this.Write(@"}

//--------------------------------------------------------------------------
//
//  Member index table
//
//  This table is used by XamlTypes to quickly look up IXamlMembers. 
//  It contains indices that map to entries in the UserMembers table. 
//  These indices are passed to FindXamlMember_FromInt32 which will get 
//  or create a XamlMember object using the information in this table to 
//  provide the metadata needed to set members in XAML to the parser. 
//  The indices are separated by a negative one deliminater into the groups 
//  of members that belong to each type such that they can be iterated over 
//  to match string names of members of each XamlType.
//
//--------------------------------------------------------------------------

MEMBERINDICE MEMBERINDICES
{
");

    int currentMemberIndex = 0;
    GenerationEnvironment.Append("    ");
    foreach (var obj in QueryHelper.GetPhoneXamlUserPropertiesWithSeparator(Model))
    {
        var objAsMember = obj as MemberDefinition;
        currentMemberIndex++;
        
            this.Write(this.ToStringHelper.ToStringWithCulture(objAsMember != null? objAsMember.PhoneMemberTableIndex : -1));
            this.Write("L, ");

        if (currentMemberIndex % 13 == 0)
        {
            GenerationEnvironment.Append("\r\n    ");
        }
    } 
            this.Write("-1L\r\n}\r\n\r\n//---------------------------------------------------------------------" +
                    "-----\r\n//\r\n//  User type member table\r\n//\r\n//-----------------------------------" +
                    "---------------------------------------\r\nUSERMEMBERINFO USERMEMBERS\r\n{\r\n");
  foreach (var m in QueryHelper.GetPhoneXamlUserProperties(Model)) { 
            this.Write("\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsResourcePropertyIndex(m)));
            this.Write(",\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsResourceKey(m.Name)));
            this.Write(",\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsResourceIndex(m.DeclaringType.MetadataName)));
            this.Write(", // Target type\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(AsResourceIndex(m.PropertyType.Type.MetadataName)));
            this.Write(", // Type\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(m is AttachedPropertyDefinition ? 1 : 0));
            this.Write(" + // IsAttachable\r\n    2 + // IsDP\r\n    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.IsReadOnly || m.SetterModifier == Modifier.Private ? 4 : 0));
            this.Write(", // IsReadOnly\r\n");
      if (m is AttachedPropertyDefinition) { 
            this.Write("    put_");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.DeclaringType.Name));
            this.Write("_Attached");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.Name));
            this.Write(",\r\n    get_");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.DeclaringType.Name));
            this.Write("_Attached");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.Name));
            this.Write("\r\n");
 } else { 
      if (m.SetterModifier == Modifier.Public) { 
            this.Write("    put_");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.DeclaringType.Name));
            this.Write("_");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.Name));
            this.Write(",\r\n");
      } else { 
            this.Write("    NULL,\r\n");
      } 
            this.Write("    get_");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.DeclaringType.Name));
            this.Write("_");
            this.Write(this.ToStringHelper.ToStringWithCulture(m.Name));
            this.Write("\r\n");
      } 
  } 
            this.Write("}\r\n\r\n//--------------------------------------------------------------------------" +
                    "\r\n//\r\n//  User type enum table\r\n//\r\n//------------------------------------------" +
                    "--------------------------------\r\n");
 foreach (var e in Model.GetAllEnums()) { 
            this.Write("User");
            this.Write(this.ToStringHelper.ToStringWithCulture(e.Name));
            this.Write("EnumValues USERENUMVALUEINFO\r\n{\r\n");
     foreach (var v in e.Values)
       { 
            this.Write("    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(String.Format("{0,-41} {1, 5},", AsResourceKey(v.Name) + ",", v.Value)));
            this.Write("\r\n");
     } 
            this.Write("}\r\n\r\n");
 } 
            this.Write("\r\nUSERENUMINFO USERENUMS\r\n{\r\n");
 foreach (var e in Model.GetAllEnums()) { 
            this.Write("    ");
            this.Write(this.ToStringHelper.ToStringWithCulture(String.Format("{0,-41} {1, 5},User{2}EnumValues ,", AsResourceIndex(e.Name) + ",", e.Values.Count(), e.Name)));
            this.Write("\r\n");
 } 
            this.Write("}");
            return this.GenerationEnvironment.ToString();
        }
    }
}
