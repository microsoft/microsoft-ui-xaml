// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Xaml;
using System.Xaml.Schema;
using System.Xml.Linq;
using System.Reflection;
using System.IO;
using System.Reflection.Adds;
using SuccinctCollectionSyntax;


namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using DirectUI;
    using Utilities;
    using XamlDom;

    internal class XamlDomValidator
    {
        private Lazy<List<XamlCompileError>> _errors = new Lazy<List<XamlCompileError>>(() => new List<XamlCompileError>());
        private Lazy<List<XamlCompileWarning>> _warnings = new Lazy<List<XamlCompileWarning>>(() => new List<XamlCompileWarning>());
        private NamedElementsStore _namedElementsHash = new NamedElementsStore();
        private DirectUISchemaContext _schemaContext;
        private bool _domRootHasCodeBehind = false;
        private bool _skipMinSdkValidation = true;

        //Cache of already checked types - if the typename is here, we've already thrown an error
        //or verified the type is OK so there's no need to re-evaluate it
        private HashSet<string> _minVersionTypeCache = new HashSet<string>();
        //Cache of already checked members
        private HashSet<Tuple<string, string>> _minVersionMemberCache = new HashSet<Tuple<string, string>>();
        //Cache of contracts from Platform.xml
        private Dictionary<string, Version> _contractCache = new Dictionary<string, Version>();

        public bool IsPass1 { get; set; }

        public bool HasUnknownChildren { get; set; }

        public Version TargetPlatformMinVersion { get; set; }

        public Platform XamlPlatform { get; set; }

        public bool Validate(XamlDomObject domRoot)
        {
            this._schemaContext = (DirectUISchemaContext)domRoot.Type.SchemaContext;
            this._domRootHasCodeBehind = HasStringAttribute(domRoot, XamlLanguage.Class);

            CheckXaml(domRoot);
            return HasErrors;
        }

        public bool HasErrors { get { return _errors.IsValueCreated && _errors.Value.Count > 0; } }
        public List<XamlCompileError> Errors { get { return _errors.Value; } }

        public bool HasWarnings { get { return _warnings.IsValueCreated; } }
        public List<XamlCompileWarning> Warnings { get { return _warnings.Value; } }

        public XamlDomValidator()
        {
        }

        private void CheckXaml(XamlDomObject domRoot)
        {
            // Check the x:Class attribute on the root element (if any)
            string className;
            XamlDomMember domMember;
            XamlDomIterator iterator;

            if (TryGetStringAttribute(domRoot, XamlLanguage.Class, out className, out domMember))
            {
                ProcessClassName(domRoot, domMember, className);
            }

            iterator = new XamlDomIterator(domRoot);
            try
            {
                iterator.EnterNewScopeCallback += _namedElementsHash.EnterNewScope;
                iterator.ExitScopeCallback += _namedElementsHash.ExitCurrentScope;

                foreach (XamlDomObject domObject in iterator.DescendantsAndSelf())
                {
                    // For objects behind conditional markup that won't appear in the app, skip all validation.
                    // If the target platform is strictly Xaml Standard, platform conditionals are disallowed
                    // so error if we find one.
                    if (DomHelper.IsObjectInvalidForPlatform(domObject, XamlPlatform))
                    {
                        if (XamlPlatform == Platform.Any)
                        {
                            Errors.Add(new XamlValidationPlatformConditionalStrict(domObject));
                        }
                        continue;
                    }

                    if (DomHelper.IsObjectInvalidForPlatform(domObject, Platform.Any))
                    {
                        // This will evaluate to true if we're using Platform Conditionals.
                        Warnings.Add(new XamlValidationWarningExperimental(ErrorCode.WMC1504, domObject, "Platform Conditionals"));
                        continue;
                    }

                    string forwardedTypeMsg;
                    if (CheckIsUnresolvedForwardedType(domObject, out forwardedTypeMsg))
                    {
                        Errors.Add(new XamlValidationErrorUnresolvedForwardedTypeAssembly(domObject, forwardedTypeMsg));
                        continue;
                    }

                    if (domObject.IsGetObject)
                    {
                        ProcessGetObjectNode(domObject);
                    }
                    else
                    {
                        if (domObject.Type.IsUnknown)
                        {
                            ProcessUnknownObjectNode(domObject);

                            // If it is still unknown (didn't get fixed up)
                            if (domObject.Type.IsUnknown)
                            {
                                HasUnknownChildren = true;
                            }
                        }
                        else
                        {
                            ProcessObjectNode(domObject, domObject == domRoot);
                        }
                    }
                }
            }
            finally
            {
                iterator.ExitScopeCallback -= _namedElementsHash.ExitCurrentScope;
                iterator.EnterNewScopeCallback -= _namedElementsHash.EnterNewScope;
            }
        }

        private bool CheckIsUnresolvedForwardedType(XamlDomObject domObject, out string errorMessage)
        {
            bool unresolved = false;
            errorMessage = null;

            try
            {
                ForceTypeResolution(domObject?.Type);
            }
            catch (UnresolvedAssemblyException e)
            {
                unresolved = true;
                errorMessage = e.Message;
            }

            return unresolved;
        }

        private bool CheckIsUnresolvedForwardedType(XamlDomMember domMember, out string errorMessage)
        {
            bool unresolved = false;
            errorMessage = null;

            try
            {
                ForceTypeResolution(domMember?.Member?.Type);
            }
            catch (UnresolvedAssemblyException e)
            {
                unresolved = true;
                errorMessage = e.Message;
            }

            return unresolved;
        }

        // Force type resolution (including base types) by checking for assignability against an arbitrary type (in this case, UIElement).
        // This throws UnresolvedAssemblyException if a type couldn't be resolved, and does nothing otherwise.
        private void ForceTypeResolution(XamlType type)
        {
            if (type != null)
            {
                type.CanAssignTo(_schemaContext.DirectUIXamlLanguage.UIElement);
            }
        }

        private bool ProcessUnknownObjectNode(XamlDomObject domObject)
        {
            // This might skip the error if local and Pass1.
            ErrorOnUnknownType(domObject);

            // Check the directive usage on Unknown Objects.
            // Might be a Local type or it might be just misspelled.
            // directive usages is mostly independent of tag.
            foreach (XamlDomMember domMember in domObject.MemberNodes)
            {
                XamlMember member = domMember.Member;
                if (member.IsDirective)
                {
                    XamlDirective directive = (XamlDirective)member;
                    CheckIdDirectiveUsage(domObject, domMember, directive);
                }
            }

            return true;
        }

        private void ErrorOnUnknownType(XamlDomObject domObject)
        {
            XamlType unknownType = domObject.Type;
            String xmlnsUri = unknownType.PreferredXamlNamespace;

            // Requery the schema for the typename.
            // The Parser marks Non-Public types as unknown.
            // The core schema will return them as non-public.
            var typeName = new XamlTypeName(xmlnsUri, unknownType.Name);
            XamlType requery = unknownType.SchemaContext.GetXamlType(typeName);

            // If type exists, but is not public
            if (requery != null && !requery.IsPublic)
            {
                Errors.Add(new XamlValidationErrorNonPublicType(domObject));
            }
            else  // type does not exist;
            {
                bool okLocalType = IsPass1 && IsPossiblyALocalType(unknownType);
                if (!okLocalType)
                {
                    if (KS.Eq(xmlnsUri, KnownStrings.UsingPrefix))
                    {
                        Errors.Add(new XamlCompilerTypeMustHaveANamespace(domObject));
                    }
                    else
                    {
                        // TODO: figure out a way of recognizing this type failed due to an assembly resolution failure (the type in domObject doesn't have its
                        // real resolved namespace or its actual name like "BindTestBed.Blah", instead it's "{using:BindTestBed}Blah" - can't map to full type/assembly name we have
                        // when resolution fails)
                        Errors.Add(new XamlValidationErrorUnknownObject(domObject));
                    }
                }
            }
        }

        private void ProcessObjectNode(XamlDomObject domObject, bool isRoot)
        {
            bool hasDeferLoadStrategyMember = false;
            bool hasLoadMember = false;

            List<string> previouslySetMembers = new List<string>();

            // Skip checking the Root if it has an x:Class Attribute.
            if (!isRoot || !HasStringAttribute(domObject, XamlLanguage.Class))
            {
                CheckDomObjectIsConstructibleOrTypeConvertable(domObject);
            }

            // Check if the object is "Style"
            if (!IsPass1)
            {
                if (_schemaContext.DirectUISystem.Style.IsAssignableFrom(domObject.Type.UnderlyingType))
                {
                    // Check that TargetType is set.  Error if it is not set.
                    XamlType xamlTargetType = GetAndResolveStyleTargetTypeProperty(domObject, true);
                    if (xamlTargetType == null)
                    {
                        return;
                    }

                    // Check if there is a BasedOn, and that it is correct.
                    XamlDomMember domBasedOnMember = domObject.GetMemberNode("BasedOn");
                    if (domBasedOnMember != null)
                    {
                        CheckBasedOnIsCorrect(domObject, xamlTargetType, domBasedOnMember);
                    }
                }
            }

            // Check the Content Property of the Element type is not an Unknown Property
            //  (the value is cached so redundant checks are quick)
            XamlMember contentProperty = domObject.Type.ContentProperty;
            if (contentProperty != null && contentProperty.IsUnknown)
            {
                Errors.Add(new XamlValidationErrorBadCPA(domObject, contentProperty.Name));
            }

            if (!IsPass1)
            {
                // Check for Deprecated Objects.
                // Only check in pass2 because Warnings don't interrupt the build and thus will
                // otherwise be reported twice (in pass1 and in pass2).
                DirectUIXamlType duiXamlType = (DirectUIXamlType)domObject.Type;
                if (duiXamlType.IsDeprecated)
                {
                    if (duiXamlType.IsHardDeprecated)
                    {
                        var error = new XamlValidationErrorDeprecated(domObject, duiXamlType.Name, duiXamlType.DeprecatedMessage);
                        Errors.Add(error);
                    }
                    else
                    {
                        var warn = new XamlValidationWarningDeprecated(domObject, duiXamlType.Name, duiXamlType.DeprecatedMessage);
                        Warnings.Add(warn);
                    }
                }

                if (duiXamlType.IsExperimental)
                {
                    var warn = new XamlValidationWarningExperimental(ErrorCode.WMC1501, domObject, duiXamlType.Name);
                    Warnings.Add(warn);
                }
            }

            //Validate that the object's type exists in the min version.
            if (!IsPass1)
            {
                ValidateTypePresentInMinVersion(domObject.Type.UnderlyingType, domObject, null);
            }

            // Validate namespaces declared on the objects are valid
            ValidateNamespaces(domObject.Namespaces);

            // Check the Properties
            foreach (XamlDomMember domMember in domObject.MemberNodes)
            {
                XamlMember member = domMember.Member;

                if (member.IsUnknown)
                {
                    ProcessUnknownMemberNode(domObject, domMember);

                    if (DomHelper.IsDeferLoadStrategyMember(domMember))
                    {
                        hasDeferLoadStrategyMember = true;
                    }
                    else if (DomHelper.IsLoadMember(domMember))
                    {
                        hasLoadMember = true;
                    }
                    // Cannot have both
                    if (hasDeferLoadStrategyMember && hasLoadMember)
                    {
                        Errors.Add(new XamlValidationError_LoadConflict(domMember));
                    }
                    continue;
                }

                // Check for duplicate assignment.
                if (previouslySetMembers.Contains(member.NameWithApiInformation()))
                {
                    Errors.Add(new XamlValidationErrorDuplicateAssigment(domMember));
                    continue;
                }

                previouslySetMembers.Add(member.NameWithApiInformation());

                if (member == ((DirectUIXamlType)domObject.Type).GetAliasedProperty(XamlLanguage.Name))
                {
                    EnsureUniqueElementName(domObject, DomHelper.GetStringValueOfProperty(domMember));
                }

                // Check x:Name x:Uid x:Key x:FieldModifier
                if (member.IsDirective)
                {
                    XamlDirective directive = (XamlDirective)member;
                    CheckIdDirectiveUsage(domObject, domMember, directive);
                    CheckModifierUsage(domObject, domMember, directive);

                    if (directive == XamlLanguage.Name)
                    {
                        // Value Types cannot be named.
                        var duiType = (DirectUIXamlType)domObject.Type;
                        if (duiType.IsValueType)
                        {
                            Errors.Add(new XamlValidationErrorCannotNameValueTypes(domObject));
                        }

                        // Check if Name property isn't set beside x:Name
                        var nameDomMember = domObject.GetMemberNode("Name");
                        if (nameDomMember != null && duiType.GetAliasedProperty(XamlLanguage.Name) == nameDomMember.Member)
                        {
                            Errors.Add(new XamlValidationErrorCannotNameElementTwice(domObject));
                        }

                        EnsureUniqueElementName(domObject, DomHelper.GetStringValueOfProperty(domMember));
                    }
                    else if (directive == XamlLanguage.Items)
                    {
                        ProcessItemsNode(domObject, domMember);
                    }

                    continue;
                }

                ProcessNormalPropertyNode(domMember);
            }
        }

        //Validates the given type is present in the targeted min version.  Also takes a domObject or domMember which is needed
        //when throwing an error, although only one of these should be valid (the other should be null).
        private void ValidateTypePresentInMinVersion(Type type, XamlDomObject domObject, XamlDomMember domMember)
        {
            // Skip validation if we're not prepared to do it (i.e. running in razzle)
            // or if this object or member is using conditional markup - assume the developer knows what they're doing.
            if (_skipMinSdkValidation || domObject?.ApiInformation != null || domMember?.ApiInformation != null)
            {
                return;
            }

            //If we've already evaluated this type, skip it - we would've already passed or thrown an error
            //for it
            if (_minVersionTypeCache.Contains(type.FullName))
            {
                return;
            }

            _minVersionTypeCache.Add(type.FullName);

            //Search through the type's attributes for contracts.
            foreach (CustomAttributeData attr in type.CustomAttributes.Where((a, ind) => a.AttributeType.IsContractVersionAttribute()))
            {
                //The arguments can be one of 3 constructurs:
                //1. Just the version with no contract name (we can't validate anything, so we just skip this case)
                //2. The contract (as a System.Type) and version
                //3. The contract (as a String) and version
                IList<CustomAttributeTypedArgument> typedArgs = attr.ConstructorArguments;

                //The contract name we'll pull out of the first constructor argument, e.g. Windows.Foundation.UniversalApiContract
                string contractName = null;

                //Case 1, we only have the version and can't validate anything, so keep going
                if (typedArgs.Count < 2)
                {
                    continue;
                }
                else
                {
                    //Check for case 2 (this is what most Windows types use) - if we have a type instead of the string, get the full name of the type which is the contract's name
                    Type type2 = typedArgs[0].Value as Type;
                    if (type2 != null)
                    {
                        contractName = type2.FullName;
                    }
                    else
                    {
                        //Case 3: just use the provided string as the contract name
                        contractName = typedArgs[0].Value as String;
                    }
                }
                Debug.Assert(contractName != null, "Could not extract contract name for type " + type.FullName + "!");

                //Get the version from the contract.
                Version typeVersion = ContractVersion.ToVersion((uint)(typedArgs[1].Value));

                Version supportedVersion = null;
                if (_contractCache.TryGetValue(contractName, out supportedVersion))
                {
                    //The contract version we got at runtime was greater than the
                    //min version's contract, so the type we loaded on the build machine
                    //won't necessarily exist on another machine the app runs on.
                    if (supportedVersion < typeVersion)
                    {
                        if (domObject != null)
                        {
                            Warnings.Add(new XamlValidationErrorWrongContract(domObject, type.FullName, contractName, typeVersion.ToString(), supportedVersion.ToString()));
                        }
                        else if (domMember != null)
                        {
                            Warnings.Add(new XamlValidationErrorWrongContract(domMember, type.FullName, contractName, typeVersion.ToString(), supportedVersion.ToString()));
                        }
                        else
                        {
                            Debug.Assert(false, "Invalid dom object and member when validating type exists in min version!");
                        }
                    }
                }
            }
        }

        private void ValidateMemberPresentInMinVersion(DirectUIXamlMember duiMember, XamlDomMember domMember)
        {
            // Skip validation if we're not prepared to do it (i.e. running in razzle)
            // or if this member is using conditional markup - assume the developer knows what they're doing.
            if (_skipMinSdkValidation || domMember.ApiInformation != null)
            {
                return;
            }

            //Check that the member will also exist in the targeted min version.  The UnderlyingMember can be null
            //if it's in an x:Bind etc.
            //Also cache the result so we don't do repeat this work
            if (duiMember?.UnderlyingMember?.DeclaringType != null && !_minVersionMemberCache.Contains(Tuple.Create<string, string>(duiMember.UnderlyingMember.DeclaringType.FullName, duiMember.UnderlyingMember.Name)))
            {
                _minVersionMemberCache.Add(Tuple.Create<string, string>(duiMember.UnderlyingMember.DeclaringType.FullName, duiMember.UnderlyingMember.Name));

                //Get the type that declared the member, then check its interfaces to figure out which one really declared it.
                //Note there are potentially two distinct members, one on the DeclaringType, and another on an interface which corresponds
                //to the member on the DeclaringType.  We can't just use the contract info for DeclaringType since we may have
                //an old type with new members from a newer contract.
                MemberInfo underlyingMemberInfo = duiMember.UnderlyingMember;
                Type declType = duiMember.UnderlyingMember.DeclaringType;

                //Where the member was really declared - probably an interface, but potentially the DeclaringType
                //if it was introduced in the same contract as the DeclaringType
                Type trulyDeclaredType = null;

                //Loop through the interfaces on the underlying DeclaringType and see if any declare a matching member
                foreach (Type curInter in declType.GetInterfaces())
                {
                    //Search for a Member with the matching name
                    if (curInter.GetMember(underlyingMemberInfo.Name).Length > 0)
                    {
                        trulyDeclaredType = curInter;
                        break;
                    }
                }

                //If we couldn't locate the member in an interface, default to the declaring type
                if (trulyDeclaredType == null)
                {
                    trulyDeclaredType = declType;
                }

                ValidateTypePresentInMinVersion(trulyDeclaredType, null, domMember);
            }
        }

        private void EnsureUniqueElementName(XamlDomObject domObject, string name)
        {
            if (_namedElementsHash.IsNameAlreadyUsed(name))
            {
                Errors.Add(new XamlValidationErrorElementNameAlreadyUsed(domObject, name));
            }
            _namedElementsHash.AddNamedElement(name);
        }

        private void CheckDomObjectIsConstructibleOrTypeConvertable(XamlDomObject domObject)
        {
            if (IsPass1)
            {
                return;
            }

            XamlType xamlType = domObject.Type;

            XamlDomMember initDirective = domObject.GetMemberNode(XamlLanguage.Initialization);

            if (initDirective != null)
            {
                return;
            }

            // Jupiter does not support constructors with arguments (using the x:Arguments directive)
            // so check that also in addition to IsConstructible
            if (!xamlType.IsConstructible || xamlType.ConstructionRequiresArguments)
            {
                Errors.Add(new XamlValidationErrorNotConstructibleObject(domObject, xamlType));
            }
        }

        private void ProcessGetObjectNode(XamlDomObject domObject)
        {
            if (IsPass1)
            {
                return;
            }

            // There should only be one member and it should be "Items"
            // but we attempt to be robust against the unexpected.
            if (domObject.MemberNodes != null)
            {
                foreach (XamlDomMember domMem in domObject.MemberNodes)
                {
                    if (domMem.Member == XamlLanguage.Items)
                    {
                        ProcessItemsNode(domObject, domMem);
                    }
                }
            }
        }

        private void ProcessClassName(XamlDomObject domRoot, XamlDomMember domMember, string className)
        {
            string[] pathParts = className.Split('.');
            if (pathParts.Length == 1)
            {
                Errors.Add(new XamlValidationErrorClassMustHaveANamespace(domMember, className));
            }
            foreach (string part in pathParts)
            {
                int idx;
                if (String.IsNullOrWhiteSpace(part))
                {
                    Errors.Add(new XamlValidationErrorClassNameEmptyPathPart(domMember, className));
                }
                else if (part.Contains(' ') || part.Contains('\t') || part.Contains('\n'))
                {
                    Errors.Add(new XamlValidationErrorClassNameNoWhiteSpace(domMember, className));
                }
                else if (!XamlDomValidator.IsValidIdentifierName(part, out idx))
                {
                    Errors.Add(new XamlValidationErrorBadName(domMember, className, part[idx]));
                }
            }

            // In Pass 2, validate the base class of the x:Class type isn't itself an x:Class/code-behind class for another .xaml file.
            // The reason is the base class will first call its InitializeComponent, loading all of its content, only to be later replaced
            // when the derived class we're currently evaluating call its InitializeComponent and loads in its content.
            // Since loading the base class' content is pointless, this can lead to an unexpected perf hit.
            // Unfortunately since we didn't have an error for this initially, we'll only raise a warning instead of a hard error, but developers shouldn't use this pattern.
            // The way we'll actually check if the base class is itself an x:Class/code-behind class for another .xaml file is by checking if the base class implements IComponentConnector,
            // signalling the Xaml compiler generated code for it.
            if (!IsPass1)
            {
                if (_schemaContext.DirectUISystem.IComponentConnector.IsAssignableFrom(domRoot.Type.UnderlyingType))
                {
                    Warnings.Add(new XamlXClassDerivedFromXClassWarning(domRoot, className, domRoot.Type.UnderlyingType.FullName));
                }
            }
        }

        private void ProcessNormalPropertyNode(XamlDomMember domMember)
        {
            if (domMember.Items.Count == 0)
            {
                return;
            }

            XamlMember member = domMember.Member;

            string forwardedTypeMsg;
            if (CheckIsUnresolvedForwardedType(domMember, out forwardedTypeMsg))
            {
                Errors.Add(new XamlValidationErrorUnresolvedForwardedTypeAssembly(domMember, forwardedTypeMsg));
                return;
            }

            var domValue = domMember.Item as XamlDomValue;
            var domObject = domMember.Item as XamlDomObject;
            Debug.Assert(domValue != null || domObject != null);

            if (!IsPass1)
            {
                // Check for Deprecated properties.
                // Only check in pass2 because Warnings don't interrupt the build and thus will
                // otherwise be reported twice (in pass1 and in pass2).
                DirectUIXamlMember duiMember = (DirectUIXamlMember)domMember.Member;
                if (duiMember.IsDeprecated)
                {
                    if (duiMember.IsHardDeprecated)
                    {
                        Errors.Add(new XamlValidationErrorDeprecated(domMember, duiMember.Name, duiMember.DeprecatedMessage));
                    }
                    else
                    {
                        Warnings.Add(new XamlValidationWarningDeprecated(domMember, duiMember.Name, duiMember.DeprecatedMessage));
                    }
                }

                if (duiMember.IsExperimental)
                {
                    Warnings.Add(new XamlValidationWarningExperimental(ErrorCode.WMC1501, domMember, duiMember.Name));
                }

                //Check that the member will also exist in the targeted min version.
                ValidateMemberPresentInMinVersion(duiMember, domMember);
            }

            var duiType = (DirectUIXamlType)domMember.Member.DeclaringType;
            if (duiType.IsCodeGenType)
            {
                CheckPropertyTypeForIllegalValueType(domMember);
            }

            if (domValue != null)
            {
                String text = (String)domValue.Value;
                CheckCanAssignTextToProperty(domMember, domMember.Member, text);

                if (!IsPass1 && domMember.Member.IsEvent)
                {
                    CheckIsAmbiguousEvent(domMember.Parent, domMember);
                }
            }
            else
            {
                // Pass1: Skip unknown child objects because we allow some unknowns in Pass1.
                // Pass2: Skip them because the object will be reported as unknown elsewhere
                // and all we are gonna say here is "cannont assign Foo to property of type <Type>."
                // which is mostly useless, and when the property is of type Object, confusing.
                if (!domObject.Type.IsUnknown)
                {
                    ProcessNormalPropertyWithObjectChild(domMember, domObject);
                }
            }

            if (DomHelper.IsBindExtension(domMember))
            {
                ProcessXBindPropertyNode(domMember);
            }
        }

        private void ProcessXBindPropertyNode(XamlDomMember domMember)
        {
            // Check for nested x:Bind
            if (DomHelper.IsBindExtension(domMember.Parent))
            {
                Errors.Add(new XamlXBindInsideXBindError(domMember));
            }

            // TwoWay x:Bind on a property that is not a DP
            if (!DomHelper.IsDependencyProperty(domMember) && DomHelper.HasTwoWayBinding(domMember))
            {
                Errors.Add(new XamlXBindTwoWayBindingToANonDependencyPropertyError(domMember));
            };

            // Check if x:Bind is used in a Xaml file that does not have code-behind
            if (!this._domRootHasCodeBehind)
            {
                Errors.Add(new XamlXBindWithoutCodeBehindError(domMember));
            }

            // TargetNullValue on non nullable type
            if (DomHelper.HasTargetNullValue(domMember) &&
                !domMember.Member.IsUnknown && !domMember.Member.Type.IsNullable)
            {
                Errors.Add(new XamlXBindTargetNullValueOnNonNullableTypeError(domMember));
            };

            if (DomHelper.IsDerivedFromControlTemplate(domMember.Parent))
            {
                Errors.Add(new XamlXBindOnControlTemplateError(domMember));
            }

            // In Pass 2, validate that if x:Binds are used outside of a template (so they aren't under a namescope)
            // that the root element of the .xaml file derives from FrameworkElement or Window.
            // x:Bind can only be used with a root element that derives from FrameworkElement or Window so
            // x:Bind can subcribe to its Loading event, which determines when x:Bind logic should first run.
            // This check doesn't apply for x:Binds in a template - their logic to run is determined by the template owner
            // calling IDataTemplateComponent.ProcessBindings on our generated Bindings object.
            if (!IsPass1 && !DomHelper.UnderANamescope(domMember.Parent))
            {
                XamlDomObject domRoot = DomHelper.GetDomRoot(domMember);
                if (!(_schemaContext.DirectUISystem.FrameworkElement.IsAssignableFrom(domRoot.Type.UnderlyingType) || _schemaContext.DirectUISystem.Window.IsAssignableFrom(domRoot.Type.UnderlyingType)))
                {
                    Errors.Add(new XamlXBindRootNoLoadingEvent(domMember, domRoot.Type.UnderlyingType.FullName));
                }
            }
        }

        private bool CheckIsAmbiguousEvent(XamlDomObject domObject, XamlDomMember domMember)
        {
            // For events used on a type defined in the local assembly,
            // we didn't know the member was an event in pass 1, and may not
            // have collected the object with the event on it if the object
            // didn't meet other collection critera (e.g. having an x:Bind or an x:Name).
            // For simplicity, we'll just require that to use an event on a type defined locally,
            // it must also have an x:Name (to guarantee it's collected).

            /*
            // TODO: Bug 19571582: re-enable this error after fixing the condition it's raised in
            if (DomHelper.IsLocalType(domObject.Type) && DomHelper.IsNamedCollectableObject(domObject, IsPass1))
            {
                Errors.Add(new XamlValidationErrorAmbiguousEvent(domMember));
                return false;
            }
            */

            return true;
        }

        private bool CheckCanAssignTextToProperty(XamlDomNode locationForErrors, XamlMember property, String text)
        {
            XamlType propertyType = property.Type;


            if (propertyType.IsCollection)
            {
                XamlType collectionItemType = propertyType.ItemType;
                if(propertyType.TypeConverter == null && !propertyType.HasCreateFromStringMethod() && (collectionItemType.TypeConverter != null || collectionItemType.HasCreateFromStringMethod()))
                {
                    return (SuccinctCollectionSyntaxVerifier.TryParse(text, locationForErrors, Errors, property));
                }

            }

            if (property.IsReadOnly)
            {
                Errors.Add(new XamlValidationErrorCannotAssignToReadOnlyProperty(locationForErrors, property, text));
                return false;
            }
            if (property.IsEvent)
            {
                if (string.IsNullOrWhiteSpace(text))
                {
                    Errors.Add(new XamlValidationErrorEventValuesMustBeText(locationForErrors, property.Name));
                    return false;
                }

                return true;  // Event Handler method names are text.
            }

            if (propertyType == _schemaContext.DirectUIXamlLanguage.String || propertyType == _schemaContext.DirectUIXamlLanguage.Object)
            {
                return true;
            }

            if (propertyType.TypeConverter == null && !propertyType.HasCreateFromStringMethod())
            {
                Errors.Add(new XamlValidationErrorCannotAssignTextToProperty(locationForErrors, property, text));
                return false;
            }
            else if (propertyType.HasCreateFromStringMethod())
            {
                // Validate that if we're using the CreateFromString method, we could resolve it.
                // If we couldn't resolve it, log the error for the member trying to use it
                XamlCompileError err = _schemaContext.EnsureCreateFromStringResolved(propertyType.Name, propertyType.GetCreateFromStringMethod(), locationForErrors);
                if (err != null)
                {
                    Errors.Add(err);
                    return false;
                }
            }

            if (propertyType.IsEnum())
            {
                String[] valueParts = text.Split(',');
                foreach (string value in valueParts)
                {
                    string trimValue = value.Trim();
                    if (!propertyType.GetEnumNames().Contains(trimValue, StringComparer.OrdinalIgnoreCase))
                    {
                        int result;
                        if (!Int32.TryParse(trimValue, out result))  // enum also except integer values (decimal)
                        {
                            Errors.Add(new XamlValidationErrorCannotAssignTextToProperty(locationForErrors, property, text));
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        private void ProcessNormalPropertyWithObjectChild(XamlDomMember domMember, XamlDomObject domChildObject)
        {
            XamlMember member = domMember.Member;

            if (domChildObject.IsGetObject)
            {
                return;  // Gotten objects alway match.
            }

            if (member.IsReadOnly)
            {
                Errors.Add(new XamlValidationErrorCannotAssignToReadOnlyProperty(domMember));
                return;
            }

            XamlType memberType = member.Type;
            if (!domChildObject.Type.CanAssignTo(memberType))
            {
                if (!domChildObject.Type.IsMarkupExtension)
                {
                    Errors.Add(new XamlValidationErrorAssignment(domChildObject, member, memberType));
                }
            }
        }

        private void ProcessItemsNode(XamlDomObject domCollectionObject, XamlDomMember domItemsMember)
        {
            Debug.Assert(domItemsMember.Member == XamlLanguage.Items);

            if (IsPass1)
            {
                return;
            }

            XamlType collectionType = domCollectionObject.Type;
            Debug.Assert(collectionType.IsCollection || collectionType.IsDictionary);

            if (domCollectionObject.Type.IsCollection)
            {
                ProcessCollectionItemsNode(domCollectionObject, domItemsMember);
                ProcessSpecialCollections(domCollectionObject, domItemsMember);
            }
            else if (domCollectionObject.Type.IsDictionary)
            {
                ProcessDictionaryItemsNode(domCollectionObject, domItemsMember);
            }
        }

        private void ProcessCollectionItemsNode(XamlDomObject domCollectionObject, XamlDomMember domItemsMember)
        {
            XamlDomMember domParentMember = domCollectionObject.Parent;
            XamlType collectionPropertyType = domParentMember.Member.Type;
            XamlType itemType = domCollectionObject.Type.ItemType;

            foreach (XamlDomItem domItem in domItemsMember.Items)
            {
                var domValue = domItem as XamlDomValue;
                var domObject = domItem as XamlDomObject;
                Debug.Assert(domValue != null || domObject != null);

                XamlDomItem errorItem = null;

                if (domValue != null)   // Value child
                {
                    errorItem = domValue;
                    foreach (XamlType allowedType in collectionPropertyType.AllowedContentTypes)
                    {
                        if (allowedType == _schemaContext.DirectUIXamlLanguage.String || allowedType == _schemaContext.DirectUIXamlLanguage.Object)
                        {
                            errorItem = null;
                            break;
                        }
                    }
                }
                else   // Object child
                {
                    if (!domObject.Type.CanAssignTo(itemType))
                    {
                        if (!domObject.Type.IsMarkupExtension)
                        {
                            errorItem = domObject;
                        }
                    }
                }

                if (errorItem != null)
                {
                    // If the error is an object (vs. text Value) and is Unknown type,
                    // Don't error 'type mismatch'.
                    // Type Unknown is reported elsewhere and is a sufficent error.
                    if (!(domObject == errorItem && domObject.Type.IsUnknown))
                    {
                        Errors.Add(new XamlValidationErrorCollectionAdd(errorItem, itemType, domCollectionObject, domParentMember));
                    }
                }
            }
        }

        private void ProcessSpecialCollections(XamlDomObject domCollectionObject, XamlDomMember domItemsMember)
        {
            XamlDomMember domParentMember = domCollectionObject.Parent;
            if (domParentMember == null)
            {
                return;
            }

            XamlDomObject domParentObject = domParentMember.Parent;
            XamlType domParentType = domParentObject.Type;
            if (_schemaContext.DirectUISystem.Style.IsAssignableFrom(domParentType.UnderlyingType))
            {
                ProcessStyleSetterCollection(domParentObject, domItemsMember);
            }
            return;
        }

        private void ProcessStyleSetterCollection(XamlDomObject domStyleObject, XamlDomMember domItemsMember)
        {
            XamlType xamlTargetType = GetAndResolveStyleTargetTypeProperty(domStyleObject);
            if (xamlTargetType == null)
            {
                return;
            }

            foreach (XamlDomItem domItem in domItemsMember.Items)
            {
                var domSetterObject = domItem as XamlDomObject;
                if (domSetterObject == null || domSetterObject.Type.IsUnknown)
                {
                    // Only checking Setters in Styles
                    // Unknown object is reported elsewhere.
                    continue;
                }
                if (!domSetterObject.Type.UnderlyingType.IsAssignableFrom(_schemaContext.DirectUISystem.Setter))
                {
                    // Only checking Setters in Styles
                    // Bad elements in collections are reported elsewhere.
                    continue;
                }
                ProcessSingleSetter(xamlTargetType, domSetterObject);
            }
        }

        private void ProcessSingleSetter(XamlType xamlTargetType, XamlDomObject domSetterObject)
        {
            // Find the Property Property.
            XamlDomMember domPropertyMember = domSetterObject.GetMemberNode("Property");
            if (domPropertyMember == null)
            {
                // Target and Property are interchangable.
                domPropertyMember = domSetterObject.GetMemberNode("Target");
            }
            if (domPropertyMember == null)
            {
                Errors.Add(new XamlValidationErrorSetterMissingField(domSetterObject, true));
                return;
            }
            String propertyName = DomHelper.GetStringValueOfProperty(domPropertyMember);
            if (propertyName == null)
            {
                Errors.Add(new XamlValidationErrorSetterMissingField(domPropertyMember, true));
                return;
            }
            XamlMember member = domSetterObject.ResolveMemberName(xamlTargetType, propertyName);
            if (member == null)
            {
                Errors.Add(new XamlValidationErrorSetterUnknownMember(domPropertyMember, xamlTargetType, propertyName));
                return;
            }
            DirectUIXamlMember directMember = member as DirectUIXamlMember;
            if (directMember != null && !directMember.IsAttachable)
            {
                if (!directMember.IsDependencyProperty)
                {
                    Errors.Add(new XamlValidationErrorSetterSetterPropertyMustBeDP(domPropertyMember, propertyName));
                }
            }

            // Find the Value Property
            XamlDomMember domValueMember = domSetterObject.GetMemberNode("Value");
            if (domValueMember == null)
            {
                Errors.Add(new XamlValidationErrorSetterMissingField(domSetterObject, false));
                return;
            }
            if (domValueMember.Item == null)
            {
                Errors.Add(new XamlValidationErrorSetterMissingField(domValueMember, false));
                return;
            }
            XamlDomValue domChildValue = domValueMember.Item as XamlDomValue;
            XamlDomObject domChildObject = domValueMember.Item as XamlDomObject;
            Debug.Assert((domChildValue != null && domChildObject == null) || (domChildValue == null && domChildObject != null));

            if (domChildValue != null)
            {
                String text = (String)domChildValue.Value;
                CheckCanAssignTextToProperty(domChildValue, member, text);
            }
            else
            {
                XamlType childXamlType = domChildObject.Type;
                if (!childXamlType.CanAssignTo(member.Type))
                {
                    if (!childXamlType.IsMarkupExtension)
                    {
                        Errors.Add(new XamlValidationErrorAssignment(domChildObject, member, member.Type));
                    }
                }
            }
        }

        private XamlType GetAndResolveStyleTargetTypeProperty(XamlDomObject domStyleObject, bool reportErrors = false)
        {
            // Get the TargetType property of the Style.
            XamlDomMember domTargetTypeMember = domStyleObject.GetMemberNode(KnownMembers.TargetType);
            if (domTargetTypeMember == null)
            {
                if (reportErrors)
                {
                    Errors.Add(new XamlValidationErrorStyleMustHaveTargetType(domStyleObject));
                }
                return null;
            }

            // Get the Value of the TargetType property
            string xTypeName = DomHelper.GetStringValueOfProperty(domTargetTypeMember);
            if (xTypeName == null)
            {
                if (reportErrors)
                {
                    Errors.Add(new XamlValidationErrorStyleMustHaveTargetType(domTargetTypeMember));
                }
                return null;
            }

            // Convert the value of the TargetType property into to a XamlType
            XamlType xamlTargetType = domStyleObject.ResolveXmlName(xTypeName);
            if (xamlTargetType == null)
            {
                if (reportErrors)
                {
                    Errors.Add(new XamlValidationErrorUnknownStyleTargetType(domTargetTypeMember, xTypeName));
                }
                return null;
            }
            return xamlTargetType;
        }

        // Look up a BasedOn={StaticResource ...
        // Confirm that the TargetTypes are compatible, and report errors if they are not.
        // If we can't find the StaticResource return silently.
        private void CheckBasedOnIsCorrect(XamlDomObject domStyleObject, XamlType xamlTargetType, XamlDomMember domBasedOnMember)
        {
            // Get the Value of the BasedOn property
            XamlDomObject domBaseStyleObject = domBasedOnMember.Item as XamlDomObject;
            XamlDomObject domStaticResource = null;
            string keyString = null;
            string otherFile = null;

            // BasedOn="someString"   error!
            if (domBaseStyleObject == null)
            {
                XamlDomValue value = domBasedOnMember.Item as XamlDomValue;
                string text = (value == null) ? String.Empty : value.Value as String;
                Errors.Add(new XamlValidationErrorStyleBasedOnMustBeStyle(domStyleObject, text));
                return;
            }

            // BasedOn="{x:Null}"  this is legal, no further checking needed.
            if (domBaseStyleObject.Type.CanAssignTo(_schemaContext.DirectUIXamlLanguage.NullExtension))
            {
                return;
            }

            // No way to validate BasedOn="{CustomResourceExtension ...}", just let these through.
            if (domBaseStyleObject.Type.CanAssignTo(_schemaContext.DirectUIXamlLanguage.CustomResourceExtension))
            {
                return;
            }

            // Check that the value was a Style
            if (!_schemaContext.DirectUISystem.Style.IsAssignableFrom(domBaseStyleObject.Type.UnderlyingType))
            {
                // If it wasn't a Style then perhaps it is a StaticResource?
                domStaticResource = domBaseStyleObject;
                if (!domStaticResource.Type.CanAssignTo(_schemaContext.DirectUIXamlLanguage.StaticResourceExtension))
                {
                    // Not a Style or StaticResource?  Error
                    Errors.Add(new XamlValidationErrorStyleBasedOnMustBeStyle(domStyleObject, domStaticResource));
                    return;
                }

                // If it was a StaticResource then lookup the Resource
                // Read the Key From the StaticResource ME
                keyString = DomHelper.GetStaticResource_ResourceKey(domStaticResource);
                if (keyString == null)
                {
                    return;  // No ResourceKey... Not error checkin StaticResource usage here.
                }

                domBaseStyleObject = ResolveStaticResource(domStyleObject, keyString, out otherFile);
                if (domBaseStyleObject == null)
                {
                    return; // didn't find it?  Return w/o error.
                }
            }

            // Is the BasedOn object a Style Object?
            if (!_schemaContext.DirectUISystem.Style.IsAssignableFrom(domBaseStyleObject.Type.UnderlyingType))
            {
                // did we get here through the Style, or StaticResource Path?
                if (keyString == null)
                {
                    Errors.Add(new XamlValidationErrorStyleBasedOnMustBeStyle(domStyleObject, domBaseStyleObject));
                }
                else
                {
                    Errors.Add(new XamlValidationErrorStyleBasedOnMustBeStyle(domStyleObject, keyString, domBaseStyleObject, otherFile));
                }
                return;
            }

            XamlType basedOnXamlTargetType = GetAndResolveStyleTargetTypeProperty(domBaseStyleObject);
            if (basedOnXamlTargetType == null)
            {
                return;
            }

            if (!xamlTargetType.CanAssignTo(basedOnXamlTargetType))
            {
                Errors.Add(new XamlValidationErrorStyleBasedOnBadStyleTargetType(domStyleObject, basedOnXamlTargetType, xamlTargetType));
            }
        }

        private void ProcessDictionaryItemsNode(XamlDomObject domDictionaryObject, XamlDomMember domItemsMember)
        {
            XamlDomMember domParentMember = domDictionaryObject.Parent;
            List<String> usedKeys = new List<string>();
            List<String> usedTypes = new List<string>();

            foreach (XamlDomItem domItem in domItemsMember.Items)
            {
                var domValue = domItem as XamlDomValue;
                var domObject = domItem as XamlDomObject;
                Debug.Assert(domValue != null || domObject != null);

                XamlType itemType = domDictionaryObject.Type.ItemType;

                if (domValue != null)
                {
                    Errors.Add(new XamlValidationErrorDictionaryAdd(domValue));
                    continue;
                }

                // We process this dictionary object twice - once in this dictionary items node method, and once in the main DescendantsAndSelf
                // loop, which will throw the unresolved forwarde type error for us.  So don't throw an error here,
                // but stop processing further errors since the assignability checks below can throw.
                string forwardedTypeMsg;
                if (CheckIsUnresolvedForwardedType(domObject, out forwardedTypeMsg))
                {
                    continue;
                }

                XamlType item = domObject.Type;

                // Can't check for Assignablity if the type is unknown.
                // Unknown checking is later, we are seeing it now because we are looking
                // deeper into tree to find the children and the main check for unknowns
                // has not gotten here yet.
                // Just skip this part and let the normaly unknown type check catch it later.
                if (!item.IsUnknown)
                {
                    if (!item.CanAssignTo(itemType))
                    {
                        if (!item.IsMarkupExtension)
                        {
                            Errors.Add(new XamlValidationErrorDictionaryAdd(domObject, itemType, domDictionaryObject, domParentMember));
                        }
                    }
                }

                // Find the "Key" on the item.
                // All this "Key" stuff is valid even if the object is of an unknown type.
                // GetMemberNode will find key aliases (like TargetType), but not x:Name.
                XamlDomMember keyMemberNode = domObject.GetMemberNode(XamlLanguage.Key);

                if (keyMemberNode == null || keyMemberNode.Member != XamlLanguage.Key)   // Null or an alias like TargetType.
                {
                    // x:Name is a x:Key aliases, with high priority than TargetType.
                    XamlDomMember alternateKeyMemberNode = domObject.GetMemberNode(XamlLanguage.Name);
                    if (alternateKeyMemberNode != null)
                    {
                        keyMemberNode = alternateKeyMemberNode;
                    }
                }

                if (keyMemberNode == null)
                {
                    Errors.Add(new XamlValidationDictionaryKeyError(domObject));
                    continue;
                }

                string keyText = DomHelper.GetStringValueOfProperty(keyMemberNode);

                // Missing TargetType, unknown types, etc are all reported elsewhere
                // This is simply checking for duplicate key usage.
                if (!String.IsNullOrWhiteSpace(keyText) && domObject.ApiInformation == null)
                {
                    // We only check for duplicated keys if the object is not conditional.
                    // If it has ApiInformation, it is conditional, so we skip it.

                    // Seperate Scopes for names/keys vs. TargetType.
                    List<String> usedList;
                    if (keyMemberNode.Member.Name == KnownMembers.TargetType)
                    {
                        usedList = usedTypes;
                        XamlType xamlType = domObject.ResolveXmlName(keyText);
                        if (xamlType != null)
                        {
                            keyText = xamlType.UnderlyingType.FullName;
                        }
                    }
                    else
                    {
                        usedList = usedKeys;
                    }
                    if (usedList.Contains(keyText))
                    {
                        Errors.Add(new XamlValidationDictionaryKeyError(domObject, keyText));
                        break;
                    }
                    usedList.Add(keyText);
                }
            }
        }

        private void ProcessUnknownMemberNode(XamlDomObject domObject, XamlDomMember domMember)
        {
            XamlMember member = domMember.Member;
            if (member.IsDirective)
            {
                XamlDirective directive = member as XamlDirective;
                if (directive == XamlLanguage.UnknownContent)
                {
                    Errors.Add(new XamlValidationErrorMissingCPA(domObject, domMember.Items[0]));
                }
                else if (DomHelper.IsPropertiesMember(domMember))
                {
                    return;
                }
                else if (DomHelper.IsDefaultBindModeMember(domMember))
                {
                    // Validate x:DefaultBindModeMember

                    // Validate that the value for DefaultBindModeMember is supported
                    string defaultBindMode = DomHelper.GetStringValueOfProperty(domMember);
                    DefaultBindMode value;
                    if (!Enum.TryParse<DefaultBindMode>(defaultBindMode, true /*case insensitive*/, out value))
                    {
                        Errors.Add(new XamlValidationError_DefaultBindModeInvalidValue(domMember));
                    }
                    return;
                }
                else if (DomHelper.IsDeferLoadStrategyMember(domMember))
                {
                    // Validate x:DeferLoadStrategy property

                    // Validate that the value provided for DeferLoadStrategy is indeed a supported strategy
                    string strategy = DomHelper.GetStringValueOfProperty(domMember);
                    DeferLoadStrategy value;
                    if (!Enum.TryParse<DeferLoadStrategy>(strategy, false /*case insensitive*/, out value))
                    {
                        Errors.Add(new XamlValidationError_DeferLoadStrategyInvalidValue(domMember));
                    }

                    // DeferLoad elements must have x:Name property
                    if (DomHelper.GetAliasedMemberNode(domObject, XamlLanguage.Name, forcePass1Eval: true) == null)
                    {
                        Errors.Add(new XamlValidationError_DeferLoadStrategyMissingXName(domObject));
                    }

                    if (!CanHaveDeferLoadStrategyOrLoad(domObject))
                    {
                        Errors.Add(new XamlValidationError_CannotHaveDeferLoadStrategy(domMember));
                    }
                    return;
                }
                else if (DomHelper.IsLoadMember(domMember))
                {
                    // Validate x:Load property

                    // Validate that the value provided for x:Load is valid (a bool)
                    string loadValue = DomHelper.GetStringValueOfProperty(domMember);
                    bool value = false;
                    if (!bool.TryParse(loadValue, out value))
                    {
                        if (DomHelper.IsBindExtension(domMember))
                        {
                            ProcessXBindPropertyNode(domMember);
                        }
                        else
                        {
                            Errors.Add(new XamlValidationError_LoadInvalidValue(domMember));
                        }
                    }

                    // x:Load elements must have x:Name property
                    if (DomHelper.GetAliasedMemberNode(domObject, XamlLanguage.Name, forcePass1Eval: true) == null)
                    {
                        Errors.Add(new XamlValidationError_LoadMissingName(domObject));
                    }

                    if (!CanHaveDeferLoadStrategyOrLoad(domObject))
                    {
                        Errors.Add(new XamlValidationError_LoadNotSupported(domMember));
                    }
                    return;
                }
                else if (DomHelper.IsDataTypeMember(domMember))
                {
                    // Validate x:DataType property.

                    // DataType is only allowed on DataTemplate
                    if (!DomHelper.IsDerivedFromDataTemplate(domObject))
                    {
                        Errors.Add(new XamlValidationError_DataTypeOnlyAllowedOnDataTemplate(domObject));
                    }

                    // We must be able to resolve the type specified in pass2
                    if (!IsPass1)
                    {
                        string dataTypeName = DomHelper.GetStringValueOfProperty(domMember);
                        if (!String.IsNullOrEmpty(dataTypeName))
                        {
                            if (domObject.ResolveXmlName(dataTypeName) == null)
                            {
                                // Need a class for the rrror;Can't resolve type
                                Errors.Add(new XamlValidationError_CantResolveDataType(domObject, dataTypeName));
                            }
                        }
                    }
                    return;
                }
                else if (DomHelper.IsPhaseMember(domMember))
                {
                    // Validate x:Phase

                    // Must be positive integer
                    string phase = DomHelper.GetStringValueOfProperty(domMember);
                    if (!String.IsNullOrEmpty(phase))
                    {
                        int value = 0;
                        if (!Int32.TryParse(phase, out value) || (value < 0) || (value > 24))
                        {
                            Errors.Add(new XamlValidationError_InvalidValueForPhase(domObject));
                        }
                    }

                    // Must be used along with x:bind
                    if (!DomHelper.DoesAnyMemberUseBindExpression(domObject))
                    {
                        Errors.Add(new XamlValidationError_PhaseCanBeUsedOnlyWithBind(domObject));
                    }

                    // x:Phase must be used inside a datatemplate
                    if (!DomHelper.UnderANamescope(domObject))
                    {
                        Errors.Add(new XamlValidationError_PhaseOnlyAllowedInDataTemplate(domObject));
                    }

                    return;
                }
            }

            if (IsPass1)
            {
                XamlType declaringType = member.DeclaringType;
                if (declaringType != null && declaringType.IsUnknown
                    && IsPossiblyALocalType(declaringType))
                {
                    return;  // local attached property provider
                }
            }

            // Unknown "dotted" properties will show as Unknown Attachable properties.
            if (member.IsAttachable)
            {
                Errors.Add(new XamlValidationErrorUnknownMember(domObject, domMember));
                return;
            }

            // Readonly properties appear to the parser as Unknown properties.
            // Requery the Schema for the property and check if it is readonly
            // to give a better error message.
            XamlMember schemaMember;
            if (TryFindPropertyInSchema(member, out schemaMember))
            {
                if (!schemaMember.IsUnknown && schemaMember.IsReadOnly)
                {
                    if (domMember.Items.Count != 0)
                    {
                        Errors.Add(new XamlValidationErrorCannotAssignToReadOnlyProperty(domMember));
                        return;
                    }
                }
            }

            Errors.Add(new XamlValidationErrorUnknownMember(domObject, domMember));
        }

        //  ----- support methods ------

        // In Pass1 local types will be "unknown".
        // So, in Pass1 only, don't error on "unknown local types".
        // But we can't actually be sure an unknown types is local.
        // True means it might be, so let it slide by (Pass1 only)
        // False means it is not a local type.
        private bool IsPossiblyALocalType(XamlType xamlType)
        {
            string usingTypePath;
            bool isLocalType = XamlHarvester.IsPossiblyALocalType(xamlType, out usingTypePath);
            return isLocalType;
        }

        private bool TryFindPropertyInSchema(XamlMember unknownMember, out XamlMember member)
        {
            var declaringType = unknownMember.DeclaringType as DirectUIXamlType;
            member = null;
            if (declaringType != null)
            {
                member = declaringType.LookupMember_SkipReadOnlyCheck(unknownMember.Name);
            }
            return (member != null);
        }

        private void CheckPropertyTypeForIllegalValueType(XamlDomMember domMember)
        {
            if (domMember.Member.IsUnknown || domMember.Member.IsEvent)
            {
                return;
            }

            DirectUIXamlType propertyType = (DirectUIXamlType)domMember.Member.Type;
            if (propertyType.IsInvalidType)
            {
                if (propertyType.IsSignedChar)
                {
                    Errors.Add(new XamlCompileErrorInvalidPropertyType_SignedChar(domMember));
                }
                else
                {
                    Errors.Add(new XamlCompileErrorInvalidPropertyType(domMember));
                }
            }
        }

        private bool HasStringAttribute(XamlDomObject domObject, XamlMember member)
        {
            string value;
            XamlDomMember domMember;
            return TryGetStringAttribute(domObject, member, out value, out domMember);
        }

        private bool TryGetStringAttribute(XamlDomObject domObject, XamlMember member, out string value, out XamlDomMember domMember)
        {
            value = null;
            domMember = domObject.GetMemberNode(member);
            if (domMember == null)
            {
                return false;
            }
            value = DomHelper.GetStringValueOfProperty(domMember);
            return !String.IsNullOrWhiteSpace(value);
        }

        private void CheckIdDirectiveUsage(XamlDomObject domObject, XamlDomMember domMember, XamlDirective xamlDirective)
        {
            if (xamlDirective == XamlLanguage.Uid || xamlDirective == XamlLanguage.Name || xamlDirective == XamlLanguage.Key)
            {
                string stringValue = DomHelper.GetStringValueOfProperty(domMember);
                if (String.IsNullOrWhiteSpace(stringValue))
                {
                    Errors.Add(new XamlValidationIdPropertiesMustBeText(domMember));
                }
                else if (xamlDirective == XamlLanguage.Name)
                {
                    int idx;
                    if (!XamlDomValidator.IsValidIdentifierName(stringValue, out idx))
                    {
                        Errors.Add(new XamlValidationErrorBadName(domMember, stringValue, stringValue[idx]));
                    }
                }
                else if (xamlDirective == XamlLanguage.Key)
                {
                    if (!XamlDomValidator.IsValidKeyIdentifierName(stringValue))
                    {
                        Errors.Add(new XamlValidationErrorBadName(domMember, stringValue));
                    }
                }
            }
        }

        private void CheckModifierUsage(XamlDomObject domObject, XamlDomMember domMember, XamlDirective xamlDirective)
        {
            if (xamlDirective == XamlLanguage.FieldModifier)
            {
                string _userProvidedModifier;
                XamlDomMember fieldModifierMember = domObject.GetMemberNode(XamlLanguage.FieldModifier);

                _userProvidedModifier = DomHelper.GetStringValueOfProperty(fieldModifierMember).ToLower();

                if (!IsValidModifier(_userProvidedModifier))
                {
                    Errors.Add(new XamlValidationErrorInvalidFieldModifier(domObject, _userProvidedModifier));
                }
            }
        }

        private XamlDomObject ResolveStaticResource(XamlDomObject domObject, string keyString, out string otherFile)
        {
            otherFile = null;
            XamlDomObject resolvedObject = null;
            XamlDomObject current = domObject;
            Type fe = _schemaContext.DirectUISystem.FrameworkElement;

            // Climb through all the parents looking for resources.
            // Jupiter XAML doesn't implement the Ambient flag, so all Resources are ambient.
            // If we are an element in a dictionary then only search the elements that
            // come before us in the dictionary.
            XamlDomObject previousCurrentDictionaryEntry = null;
            while (current != null)
            {
                Type currentType = current.Type.UnderlyingType;
                if (fe.IsAssignableFrom(currentType))
                {
                    XamlDomMember resources = current.GetMemberNode("Resources");
                    if (resources != null)
                    {
                        resolvedObject = FindKeyInResources(keyString, resources, previousCurrentDictionaryEntry);
                        if (resolvedObject != null)
                        {
                            break;
                        }
                    }
                }
                XamlDomMember parentMember = current.Parent;
                if (parentMember == null)
                {
                    break;
                }
                if (IsAnItemInAResourceDictionary(current))
                {
                    previousCurrentDictionaryEntry = current;
                }
                current = parentMember.Parent;
            }

            return resolvedObject;
        }

        private bool IsAnItemInAResourceDictionary(XamlDomObject item)
        {
            XamlDomMember parentMember = item.Parent;
            if (parentMember != null && parentMember.Member == XamlLanguage.Items)
            {
                XamlDomObject dict = parentMember.Parent;
                Type resourceDictionary = _schemaContext.DirectUISystem.ResourceDictionary;
                if (dict != null && resourceDictionary.IsAssignableFrom(dict.Type.UnderlyingType))
                {
                    return true;
                }
            }
            return false;
        }

        private XamlDomObject FindKeyInResources(string keyString, XamlDomMember resources, XamlDomObject searchLimit)
        {
            XamlDomObject resourceDictionary = resources.Item as XamlDomObject;
            if (resourceDictionary != null)
            {
                XamlDomMember itemsMember = resourceDictionary.GetMemberNode(XamlLanguage.Items);
                if (itemsMember != null && itemsMember.Items.Count > 0)
                {
                    foreach (XamlDomItem item in itemsMember.Items)
                    {
                        XamlDomObject domObj = item as XamlDomObject;
                        if (domObj != null)
                        {
                            // Only search items before us in the dictionary
                            // If we are also in the dictionary.
                            if (domObj == searchLimit)
                            {
                                return null;
                            }

                            // Try to find the Key node
                            XamlDomMember domKey = domObj.GetMemberNode(XamlLanguage.Key, allowPropertyAliasing: false);
                            if (domKey == null)
                            {
                                // Key nod not found, try the Name node
                                domKey = domObj.GetMemberNode(XamlLanguage.Name, allowPropertyAliasing: false);
                            }

                            if (domKey != null)
                            {
                                string key = DomHelper.GetStringValueOfProperty(domKey);
                                if (String.Equals(key, keyString, StringComparison.Ordinal))
                                {
                                    return domObj;
                                }
                            }
                        }
                    }
                }
            }
            return null;
        }

        // ------ public statics

        public static bool IsValidIdentifierName(string name)
        {
            int idx;
            return IsValidIdentifierName(name, out idx);
        }

        public static bool IsValidIdentifierName(string name, out int idx)
        {
            // Grammar:
            // <identifier> ::= <identifier_start> ( <identifier_start> | <identifier_extend> )*
            // <identifier_start> ::= [{Lu}{Ll}{Lt}{Lo}{Nl}('_')]
            // <identifier_extend> ::= [{Mn}{Mc}{Lm}{Nd}]

            //Check if name is a valid string
            idx = 0;
            if (name == null)
            {
                return false;
            }

            UnicodeCategory uc;
            for (int i = 0; i < name.Length; i++)
            {
                uc = Char.GetUnicodeCategory(name[i]);
                bool idStart = (uc == UnicodeCategory.UppercaseLetter || // (Lu)
                             uc == UnicodeCategory.LowercaseLetter || // (Ll)
                             uc == UnicodeCategory.TitlecaseLetter || // (Lt)
                             uc == UnicodeCategory.OtherLetter || // (Lo)
                             uc == UnicodeCategory.LetterNumber || // (Nl)
                             name[i] == '_');
                bool idExtend = (uc == UnicodeCategory.NonSpacingMark || // (Mn)
                              uc == UnicodeCategory.SpacingCombiningMark || // (Mc)
                              uc == UnicodeCategory.ModifierLetter || // (Lm)
                              uc == UnicodeCategory.DecimalDigitNumber); // (Nd)
                if (i == 0)
                {
                    if (!idStart)
                    {
                        idx = i;
                        return false;
                    }
                }
                else if (!(idStart || idExtend))
                {
                    idx = i;
                    return false;
                }
            }
            return true;
        }

        public static bool IsValidKeyIdentifierName(string name)
        {
            // Grammar: accepts any value ..

            //Check if name is a valid string
            if (name == null)
            {
                return false;
            }
            return true;
        }

        public static bool IsValidModifier(string modifierValue)
        {
            bool _validModifier = false;

            if (string.Compare(modifierValue, "private", StringComparison.OrdinalIgnoreCase) == 0)
            {
                _validModifier = true;
            }
            else if (string.Compare(modifierValue, "public", StringComparison.OrdinalIgnoreCase) == 0)
            {
                _validModifier = true;
            }
            else if (string.Compare(modifierValue, "protected", StringComparison.OrdinalIgnoreCase) == 0)
            {
                _validModifier = true;
            }
            else if (string.Compare(modifierValue, "internal", StringComparison.OrdinalIgnoreCase) == 0)
            {
                _validModifier = true;
            }
            else if (string.Compare(modifierValue, "friend", StringComparison.OrdinalIgnoreCase) == 0)
            {
                _validModifier = true;
            }

            return _validModifier;
        }

        public bool CanHaveDeferLoadStrategyOrLoad(XamlDomObject domObject)
        {
            // Direct parent cannot be a resource dictionary
            if (domObject.Parent != null && DomHelper.IsDerivedFromResourceDictionary(domObject.Parent.Parent))
            {
                return false;
            }

            // Cannot be a Scope Root (Page/UserControl)
            if (domObject.Parent?.Parent == null)
            {
                return false;
            }

            // Cannot be a Scope Root (DataTemplate)
            if (domObject.Parent != null && DomHelper.IsDerivedFromDataTemplate(domObject.Parent.Parent))
            {
                return false;
            }

            // Element can only be a UIElement or FlyoutBase
            if (!DomHelper.IsDerivedFromUIElement(domObject) &&
                !DomHelper.IsDerivedFromFlyoutBase(domObject))
            {
                return false;
            }

            // All other can have x:DeferLoadStrategy or x:Load
            return true;
        }

        private void ValidateNamespaces(IEnumerable<XamlDomNamespace> namespacesToValidate)
        {
            foreach (var namespaceToValidate in namespacesToValidate)
            {
                var nsUri = namespaceToValidate.NamespaceDeclaration.Namespace;
                if (nsUri.IsConditionalNamespace())
                {
                    try
                    {
                        ConditionalNamespace.Parse(nsUri);
                    }
                    catch (ParseException e)
                    {
                        Errors.Add(new XamlValidationConditionalNamespaceError(nsUri, e.Message, namespaceToValidate));
                    }
                }
            }
        }
    }
}
