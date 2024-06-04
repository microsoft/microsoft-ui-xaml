// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Xaml;
using System.Xml;

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using DirectUI;
    using Properties;

    internal class DomHelper
    {
        static public IEnumerable<XamlDomNode> DomNodeAncestry(XamlDomNode node)
        {
            XamlDomMember currentMember = node as XamlDomMember;
            XamlDomObject currentObject = node as XamlDomObject;

            while (currentMember != null || currentObject != null)
            {
                if (currentMember != null)
                {
                    currentObject = currentMember.Parent;
                    currentMember = null;
                    yield return currentObject;
                }
                else // if (currentObject != null)
                {
                    currentMember = currentObject.Parent;
                    currentObject = null;
                    if (currentMember == null)
                        break;
                    yield return currentMember;
                }
            }
        }

        public static string GetStringValueOfProperty(XamlDomObject domObject, string memberName)
        {
            return GetStringValueOfProperty(domObject.GetMemberNode(memberName));
        }

        static public String GetStringValueOfProperty(XamlDomObject domObject, XamlMember member)
        {
            Debug.Assert(member.Type == XamlLanguage.String);
            return GetStringValueOfProperty(domObject.GetMemberNode(member));
        }

        static public String GetStringValueOfProperty(XamlDomMember domMember)
        {
            XamlDomValue domValue = domMember?.Item as XamlDomValue;
            return domValue?.Value as String;
        }

        static public XamlDomObject GetDomRoot(XamlDomNode node)
        {
            if (node == null)
            {
                return null;
            }

            XamlDomMember currentMember = node as XamlDomMember;
            XamlDomObject currentObject = node as XamlDomObject;

            while (currentMember != null || currentObject != null)
            {
                if (currentMember != null)
                {
                    // Members will never be the root node, just get the member's owner and keep going
                    currentObject = currentMember.Parent;
                    currentMember = null;
                }
                else
                {
                    // A XamlDomObject is the root if its parent/owning member is null,
                    // so return the current object if so.  Otherwise keep
                    // traversing via its owning member.
                    XamlDomMember parent = currentObject.Parent;
                    if (parent == null)
                    {
                        return currentObject;
                    }
                    currentMember = parent;
                    currentObject = null;
                }
            }

            Debug.Assert(false, "Could not resolve DomRoot for non-null DomNode");
            return null;
        }

        static public bool IsLocalType(XamlType xamlType)
        {
            if (xamlType == null)
            {
                return false;
            }

            if (xamlType.UnderlyingType == null)
            {
                return true;
            }

            DirectUIAssembly duiAsm = xamlType.UnderlyingType.Assembly as DirectUIAssembly;
            if (duiAsm == null)
            {
                duiAsm = DirectUIAssembly.Wrap(xamlType.UnderlyingType.Assembly);
            }

            var duiSchema = xamlType.SchemaContext as DirectUISchemaContext;
            if (duiSchema == null)
            {
                return false;
            }
            return duiSchema.IsLocalAssembly(duiAsm);
        }

        static public XamlDomMember GetAliasedMemberNode(XamlDomObject domObject, XamlDirective directive, bool forcePass1Eval = false)
        {
            if (!directive.IsDirective)
            {
                throw new ArgumentException(XamlCompilerResources.DuiSchema_ArgumentNotXamlDirective, "directive");
            }

            // First look for the directive.
            XamlDomMember domMember = domObject.GetMemberNode(directive, allowPropertyAliasing: !forcePass1Eval);
            if (domMember != null)
            {
                return domMember;
            }

            if (forcePass1Eval && DomHelper.IsLocalType(domObject.Type))
            {
                return null;
            }

            // Then look for an aliased member.
            XamlMember aliasMember = domObject.Type.GetAliasedProperty(directive);
            if (aliasMember == null)
            {
                return null;
            }
            return domObject.GetMemberNode(aliasMember);
        }

        static public string SaveToString(XamlDomObject rootObjectNode)
        {
            XamlSchemaContext schemaContext = rootObjectNode.Type.SchemaContext;
            XamlDomReader domReader = new XamlDomReader(rootObjectNode, schemaContext);

            StringWriter stringWriter = new StringWriter();
            using (XmlTextWriter xmlWriter = new XmlTextWriter(stringWriter))
            {
                xmlWriter.Formatting = Formatting.Indented;
                xmlWriter.Indentation = 2;

                XamlXmlWriter xamlXmlWriter = new XamlXmlWriter(xmlWriter, schemaContext);
                XamlServices.Transform(domReader, xamlXmlWriter);
            }
            return stringWriter.ToString();
        }

        // This assumes we're in Pass 1 if the caller doesn't know,
        // so that we don't try to resolve types when they may not be resolvable.
        // Ideally all callers would be able to set isPass1 to its true value.
        static internal bool UnderANamescope(XamlDomObject namedObject, bool isPass1 = true)
        {
            foreach (XamlDomNode node in DomHelper.DomNodeAncestry(namedObject))
            {
                XamlDomMember currentMember = node as XamlDomMember;
                XamlDomObject currentObject = node as XamlDomObject;

                XamlType type = null;
                if (currentMember == null)
                {
                    type = currentObject.Type;
                    if (type == null)
                    {
                        continue;  // Get Object
                    }
                }
                else
                {
                    if (currentMember.Member.IsDirective)
                    {
                        continue;
                    }
                    if (currentMember.Member.IsUnknown)
                    {
                        continue;
                    }
                    XamlMember member = currentMember.Member;
                    type = member.Type;
                }

                if (type.IsNameScope)
                {
                    return true;
                }
                if (type.DeferringLoader != null)
                {
                    return true;
                }
                if ( !(IsLocalType(type) && isPass1) && 
                     type.IsDerivedFromFrameworkTemplate())
                {
                    return true;
                }
            }
            return false;
        }

        static internal bool IsInsideControlTemplate(XamlDomObject namedObject, bool isPass1 = false)
        {
            foreach (XamlDomNode node in DomHelper.DomNodeAncestry(namedObject))
            {
                XamlDomMember currentMember = node as XamlDomMember;
                XamlDomObject currentObject = node as XamlDomObject;

                XamlType type = null;
                if (currentMember == null)
                {
                    type = currentObject.Type;
                    if (type == null)
                    {
                        continue;  // Get Object
                    }
                }
                else
                {
                    if (currentMember.Member.IsDirective)
                    {
                        continue;
                    }
                    if (currentMember.Member.IsUnknown)
                    {
                        continue;
                    }
                    XamlMember member = currentMember.Member;
                    type = member.Type;
                }
                if (type.IsDerivedFromDataTemplate())
                {
                    // If we first come across a DataTemplate, then we shouldn't consider
                    // ourselves inside a control template.
                    return false;
                }
                if (type.IsDerivedFromControlTemplate())
                {
                    return true;
                }
            }
            return false;
        }

        public static bool IsDerivedFromDataTemplate(XamlDomObject domObject)
        {
            return !domObject.Type.IsUnknown && domObject.Type.IsDerivedFromDataTemplate();
        }

        public static bool IsDerivedFromControlTemplate(XamlDomObject domObject)
        {
            return !domObject.Type.IsUnknown && domObject.Type.IsDerivedFromControlTemplate();
        }

        public static bool IsDerivedFromResourceDictionary(XamlDomObject domObject)
        {
            return !domObject.Type.IsUnknown && domObject.Type.IsDerivedFromResourceDictionary();
        }

        public static bool IsDerivedFromUIElement(XamlDomObject domObject)
        {
            return !domObject.Type.IsUnknown && domObject.Type.IsDerivedFromUIElement();
        }

        public static bool IsDerivedFromFlyoutBase(XamlDomObject domObject)
        {
            return !domObject.Type.IsUnknown && domObject.Type.IsDerivedFromFlyoutBase();
        }

        public static bool IsNamedDirective(XamlDomMember domMember, string directiveName)
        {
            XamlMember xamlMember = domMember.Member;
            if (xamlMember.IsDirective)
            {
                XamlDirective directive = xamlMember as XamlDirective;
                if (directive.Name.Equals(directiveName, StringComparison.InvariantCulture))
                {
                    return true;
                }
            }
            return false;
        }

        public static bool IsDeferLoadStrategyMember(XamlDomMember domMember)
        {
            return IsNamedDirective(domMember, "DeferLoadStrategy");
        }

        public static bool IsLoadMember(XamlDomMember domMember)
        {
            return IsNamedDirective(domMember, "Load");
        }

        public static bool IsPropertiesMember(XamlDomMember domMember)
        {
            return IsNamedDirective(domMember, "Properties");
        }

        public static bool IsDefaultBindModeMember(XamlDomMember domMember)
        {
            return IsNamedDirective(domMember, "DefaultBindMode");
        }
        
        public static bool IsDataTypeMember(XamlMember xamlMember)
        {
            if (xamlMember.IsDirective)
            {
                XamlDirective directive = xamlMember as XamlDirective;
                if (directive.Name.Equals("DataType", StringComparison.InvariantCulture))
                {
                    return true;
                }
            }
            else if (xamlMember.Name.Equals("TargetType", StringComparison.InvariantCulture))
            {
                return true;
            }
            return false;
        }

        public static bool IsDataTypeMember(XamlDomMember domMember)
        {
            XamlMember member = domMember.Member;
            return IsDataTypeMember(member);
        }

        public static XamlDomMember GetDataTypeMember(XamlDomObject domObject)
        {
            foreach (XamlDomMember domMember in domObject.MemberNodes)
            {
                if (IsDataTypeMember(domMember))
                {
                    return domMember;
                }
            }

            return null;
        }
        public static string GetStaticResource_ResourceKey(XamlDomObject domStaticResourceObject)
        {
            string resourceKey = null;
            DirectUISchemaContext schemaContext = domStaticResourceObject.SchemaContext as DirectUISchemaContext;
            if (schemaContext != null && domStaticResourceObject.Type.CanAssignTo(schemaContext.DirectUIXamlLanguage.StaticResourceExtension))
            {
                // Look in the ResourceKey property or the first of the Position Parameters.
                XamlDomMember domResourceKeyMember = domStaticResourceObject.GetMemberNode("ResourceKey");
                if (domResourceKeyMember != null)
                {
                    resourceKey = DomHelper.GetStringValueOfProperty(domResourceKeyMember);
                }
                else
                {
                    XamlDomMember pparams = domStaticResourceObject.GetMemberNode(XamlLanguage.PositionalParameters);
                    if (pparams != null && pparams.Items.Count == 1 && pparams.Items[0] != null)
                    {
                        XamlDomValue keyValue = pparams.Items[0] as XamlDomValue;
                        resourceKey = (keyValue != null) ? keyValue.Value as string : null;
                    }
                }
            }
            return resourceKey;
        }

        public static bool IsPhaseMember(XamlMember xamlMember)
        {
            if (xamlMember.IsDirective)
            {
                XamlDirective directive = xamlMember as XamlDirective;
                if (directive.Name.Equals("Phase", StringComparison.InvariantCulture))
                {
                    return true;
                }
            }
            return false;
        }

        public static bool IsPhaseMember(XamlDomMember domMember)
        {
            XamlMember member = domMember.Member;
            return IsPhaseMember(member);
        }

        public static bool IsBindExtension(XamlDomMember domMember)
        {
            XamlDomObject memItemDomObject = domMember.Item as XamlDomObject;
            return memItemDomObject != null && IsBindExtension(memItemDomObject);
        }

        public static bool IsBindExtension(XamlDomObject domObject)
        {
            DirectUISchemaContext schema = domObject.SchemaContext as DirectUISchemaContext;
            return schema != null && domObject.Type == schema.DirectUIXamlLanguage.BindExtension;
        }

        public static bool IsDependencyProperty(XamlDomMember domMember)
        {
            DirectUIXamlMember member = domMember.Member as DirectUIXamlMember;
            if (member != null)
            {
                return member.IsDependencyProperty || member.IsAttachable;
            }
            else
            {
                return false;
            }
        }

        public static XamlDomObject GetBindExtensionOrNull(XamlDomMember domMember)
        {
            XamlDomObject item = domMember.Item as XamlDomObject;
            DirectUISchemaContext schema = domMember.Parent.SchemaContext as DirectUISchemaContext;

            if (item != null && schema != null && item.Type == schema.DirectUIXamlLanguage.BindExtension)
            {
                return item;
            }
            return null;
        }

        public static bool HasTwoWayBinding(XamlDomMember domMember)
        {
            XamlDomObject item = GetBindExtensionOrNull(domMember);
            if (item != null)
            { 
                string mode = DomHelper.GetStringValueOfProperty(item.GetMemberNode(KnownStrings.Mode));
                return mode != null && mode == KnownStrings.TwoWay;
            }
            return false;
        }

        public static bool HasTargetNullValue(XamlDomMember domMember)
        {
            XamlDomObject item = GetBindExtensionOrNull(domMember);
            if (item != null)
            {
                return item.GetMemberNode(KnownStrings.TargetNullValue) != null;
            }
            return false;
        }

        public static bool HasUpdateSourceTrigger(XamlDomMember domMember)
        {
            XamlDomObject item = GetBindExtensionOrNull(domMember);
            if (item != null)
            {
                return item.GetMemberNode(KnownStrings.UpdateSourceTrigger) != null;
            }
            return false;
        }

        public static bool DoesAnyMemberUseBindExpression(XamlDomObject domObject)
        {
            foreach (XamlDomMember domMember in domObject.MemberNodes)
            {
                if (IsBindExtension(domMember))
                {
                    return true;
                }
            }
            return false;
        }

        public static bool CanBeInstantiatedLater(XamlDomObject namedObject)
        {
            if (HasLoadOrDeferLoadStrategyMember(namedObject))
            {
                return true;
            }

            foreach (XamlDomNode node in DomHelper.DomNodeAncestry(namedObject))
            {
                XamlDomObject domObject = node as XamlDomObject;
                if (domObject != null && HasLoadOrDeferLoadStrategyMember(domObject))
                {
                    return true;
                }
            }
            return false;
        }

        public static bool HasLoadOrDeferLoadStrategyMember(XamlDomObject namedObject)
        {
            return namedObject.MemberNodes.Where(x => DomHelper.IsDeferLoadStrategyMember(x) || DomHelper.IsLoadMember(x)).Any();
        }

        public static bool HasDefaultBindModeMember(XamlDomObject namedObject)
        {
            return namedObject.MemberNodes.Where(x => DomHelper.IsDefaultBindModeMember(x)).Any();
        }

        // Returns whether a type with the given platform conditional should be kept in markup.
        public static bool ConditionalValidForPlatform(Platform platCond, Platform targPlat)
        {
            switch (targPlat)
            {
                // For strict Xaml Standard apps, platform conditionals can never be used.  This is actually an error case that gets checked in the validator
                case Platform.Any:
                    return (platCond == Platform.Any);

                // For single-platform cases, only unconditional statements and platform conditionals for that platform should be kept
                case Platform.Android:
                    return (platCond == Platform.Any || platCond == Platform.Android);
                case Platform.iOS:
                    return (platCond == Platform.Any || platCond == Platform.iOS);
                case Platform.UWP:
                    return (platCond == Platform.Any || platCond == Platform.UWP);
                default:
                    throw new Exception("Unknown target platform!");
            }
        }

        private static bool IsTypeInvalidForPlatform(XamlType type, Platform targPlat)
        {
            Platform platCond = Platform.Any;
            DirectUIXamlType objType = type as DirectUIXamlType;

            if (objType != null)
            {
                platCond = objType.TargetPlatform;
            }
            else
            {
                // We only store platform-conditional info in DirectUIXamlType.  If the XamlType isn't a DUIXamlType, it is most likely an unresolved type from another platform.
                // We'll parse its namespace to determine its platform.
                string xmlns = type.PreferredXamlNamespace;
                if (xmlns.HasUsingPrefix())
                {
                    string xmlnsNoUsing = xmlns.StripUsingPrefix();
                    if (xmlnsNoUsing.IsConditionalNamespace())
                    {
                        try
                        {
                            platCond = ConditionalNamespace.Parse(xmlnsNoUsing).PlatConditional;
                        }
                        catch (ParseException)
                        {
                            Debug.Assert(false, "Validator should have checked this scenario");
                        }
                    }
                }
            }

            return !ConditionalValidForPlatform(platCond, targPlat);
        }

        public static bool IsObjectInvalidForPlatform(XamlDomObject obj, Platform targPlat)
        {
            return IsTypeInvalidForPlatform(obj.Type, targPlat);
        }

        public static bool IsMemberInvalidForPlatform(XamlDomMember member, Platform targPlat)
        {
            return IsTypeInvalidForPlatform(member.Member.Type, targPlat);
        }

        // Gets the proper DefaultBindMode for this object - checks for DefaultBindMode on the object itself,
        // as well as if any of its ancestors had it.
        // Returns null if no default bind mode is present for this object.
        public static string GetDefaultBindMode(XamlDomObject namedObject)
        {
            string defaultBindMode = GetDefaultBindModeForSingleton(namedObject);
            if (defaultBindMode == null)
            {
                // The local object didn't have the member, we need to check its ancestors in case we should inherit a default bind mode.
                foreach (XamlDomNode node in DomHelper.DomNodeAncestry(namedObject))
                {
                    XamlDomObject domObject = node as XamlDomObject;
                    if (domObject != null)
                    {
                        defaultBindMode = GetDefaultBindModeForSingleton(domObject);
                        if (defaultBindMode != null)
                        {
                            break;
                        }
                    }
                }
            }

            return defaultBindMode;
        }

        //Returns the DefaultBindMode member or null if the object doesn't have one, does not check ancestors.
        public static XamlDomMember GetDefaultBindModeMember(XamlDomObject namedObject)
        {
            return namedObject.MemberNodes.Where(x => DomHelper.IsDefaultBindModeMember(x)).FirstOrDefault();
        }

        // Checks for the value of DefaultBindMode on the given object only, does not check its ancestors.
        private static string GetDefaultBindModeForSingleton(XamlDomObject namedObject)
        {
            XamlDomMember domDefaultBindModeMember = GetDefaultBindModeMember(namedObject);
            string defaultBindMode = null;
            if (domDefaultBindModeMember != null)
            {
                defaultBindMode = DomHelper.GetStringValueOfProperty(domDefaultBindModeMember);
            }
            return defaultBindMode;
        }

        // Returns true if the object has an x:Name, and needs to be collected
        // as a code behind element.
        public static bool IsNamedCollectableObject(XamlDomObject domObject, bool isPass1)
        {
            return DomHelper.GetAliasedMemberNode(domObject, XamlLanguage.Name, forcePass1Eval: true) != null;
        }
    }
}
