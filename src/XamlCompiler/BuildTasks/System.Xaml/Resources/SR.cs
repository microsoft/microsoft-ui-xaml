//-------------------------------------------------------------------------------
// <copyright from='1999' to='2003' company='Microsoft Corporation'>
//    Copyright (c) Microsoft Corporation. All Rights Reserved.
//    Information Contained Herein is Proprietary and Confidential.
// </copyright>
//
// This file is generated from ExceptionStringTable.txt by gensr.pl - do not modify this file directly
//-------------------------------------------------------------------------------


using System;
using System.Resources;
using System.Globalization;

namespace System.Xaml
{

    //a wrapper around string identifiers.
    internal struct SRID
    {
           public const string Default="Default";
           public const string UnexpectedParameterType = "UnexpectedParameterType";
           public const string CannotConvertStringToType = "CannotConvertStringToType";
           public const string CannotConvertType = "CannotConvertType";
           public const string StringEmpty = "StringEmpty";
           public const string ParameterCannotBeNegative = "ParameterCannotBeNegative";
           public const string Enum_Invalid = "Enum_Invalid";
           public const string Collection_BadType = "Collection_BadType";
           public const string Collection_CopyTo_IndexGreaterThanOrEqualToArrayLength = "Collection_CopyTo_IndexGreaterThanOrEqualToArrayLength";
           public const string Collection_CopyTo_NumberOfElementsExceedsArrayLength = "Collection_CopyTo_NumberOfElementsExceedsArrayLength";
           public const string Collection_CopyTo_ArrayCannotBeMultidimensional = "Collection_CopyTo_ArrayCannotBeMultidimensional";
           public const string CollectionNumberOfElementsMustBeLessOrEqualTo = "CollectionNumberOfElementsMustBeLessOrEqualTo";
           public const string Enumerator_VerifyContext = "Enumerator_VerifyContext";
           public const string Animation_ChildMustBeKeyFrame = "Animation_ChildMustBeKeyFrame";
           public const string Animation_NoTextChildren = "Animation_NoTextChildren";
           public const string Animation_InvalidBaseValue = "Animation_InvalidBaseValue";
           public const string Animation_InvalidTimeKeyTime = "Animation_InvalidTimeKeyTime";
           public const string Animation_InvalidResolvedKeyTimes = "Animation_InvalidResolvedKeyTimes";
           public const string Animation_InvalidAnimationUsingKeyFramesDuration = "Animation_InvalidAnimationUsingKeyFramesDuration";
           public const string Animation_Invalid_DefaultValue = "Animation_Invalid_DefaultValue";
           public const string Freezable_CantBeFrozen = "Freezable_CantBeFrozen";
           public const string CannotModifyReadOnlyContainer = "CannotModifyReadOnlyContainer";
           public const string CannotRetrievePartsOfWriteOnlyContainer = "CannotRetrievePartsOfWriteOnlyContainer";
           public const string TokenizerHelperPrematureStringTermination = "TokenizerHelperPrematureStringTermination";
           public const string TokenizerHelperMissingEndQuote = "TokenizerHelperMissingEndQuote";
           public const string TokenizerHelperExtraDataEncountered = "TokenizerHelperExtraDataEncountered";
           public const string TokenizerHelperEmptyToken = "TokenizerHelperEmptyToken";
           public const string InvalidPermissionType = "InvalidPermissionType";
           public const string InvalidPermissionStateValue = "InvalidPermissionStateValue";
           public const string SecurityExceptionForSettingSandboxExternalToTrue = "SecurityExceptionForSettingSandboxExternalToTrue";
           public const string FileFormatException = "FileFormatException";
           public const string FileFormatExceptionWithFileName = "FileFormatExceptionWithFileName";
           public const string TypeMetadataCannotChangeAfterUse = "TypeMetadataCannotChangeAfterUse";
           public const string Visual_ArgumentOutOfRange = "Visual_ArgumentOutOfRange";
           public const string XCRChoiceOnlyInAC = "XCRChoiceOnlyInAC";
           public const string XCRChoiceAfterFallback = "XCRChoiceAfterFallback";
           public const string XCRRequiresAttribNotFound = "XCRRequiresAttribNotFound";
           public const string XCRInvalidRequiresAttribute = "XCRInvalidRequiresAttribute";
           public const string XCRFallbackOnlyInAC = "XCRFallbackOnlyInAC";
           public const string XCRChoiceNotFound = "XCRChoiceNotFound";
           public const string XCRMultipleFallbackFound = "XCRMultipleFallbackFound";
           public const string XCRInvalidAttribInElement = "XCRInvalidAttribInElement";
           public const string XCRUnknownCompatElement = "XCRUnknownCompatElement";
           public const string XCRInvalidACChild = "XCRInvalidACChild";
           public const string XCRInvalidFormat = "XCRInvalidFormat";
           public const string XCRUndefinedPrefix = "XCRUndefinedPrefix";
           public const string XCRUnknownCompatAttrib = "XCRUnknownCompatAttrib";
           public const string XCRNSProcessContentNotIgnorable = "XCRNSProcessContentNotIgnorable";
           public const string XCRDuplicateProcessContent = "XCRDuplicateProcessContent";
           public const string XCRInvalidProcessContent = "XCRInvalidProcessContent";
           public const string XCRDuplicateWildcardProcessContent = "XCRDuplicateWildcardProcessContent";
           public const string XCRMustUnderstandFailed = "XCRMustUnderstandFailed";
           public const string XCRNSPreserveNotIgnorable = "XCRNSPreserveNotIgnorable";
           public const string XCRDuplicatePreserve = "XCRDuplicatePreserve";
           public const string XCRInvalidPreserve = "XCRInvalidPreserve";
           public const string XCRDuplicateWildcardPreserve = "XCRDuplicateWildcardPreserve";
           public const string XCRInvalidXMLName = "XCRInvalidXMLName";
           public const string XCRCompatCycle = "XCRCompatCycle";
           public const string BadXmlnsDefinition = "BadXmlnsDefinition";
           public const string BadXmlnsCompat = "BadXmlnsCompat";
           public const string BadXmlnsPrefix = "BadXmlnsPrefix";
           public const string BadInternalsVisibleTo1 = "BadInternalsVisibleTo1";
           public const string BadInternalsVisibleTo2 = "BadInternalsVisibleTo2";
           public const string DuplicateXmlnsCompat = "DuplicateXmlnsCompat";
           public const string XmlnsCompatCycle = "XmlnsCompatCycle";
           public const string UriNotFound = "UriNotFound";
           public const string DuplicateXmlnsCompatAcrossAssemblies = "DuplicateXmlnsCompatAcrossAssemblies";
           public const string UnresolvedNamespace = "UnresolvedNamespace";
           public const string TypeNotFound = "TypeNotFound";
           public const string TypeNotPublic = "TypeNotPublic";
           public const string TooManyTypeConverterAttributes = "TooManyTypeConverterAttributes";
           public const string CannotFindAssembly = "CannotFindAssembly";
           public const string MissingAssemblyName = "MissingAssemblyName";
           public const string InvalidTypeArgument = "InvalidTypeArgument";
           public const string FileNotFoundExceptionMessage = "FileNotFoundExceptionMessage";
           public const string DirectiveNotFound = "DirectiveNotFound";
           public const string MustNotCallSetter = "MustNotCallSetter";
           public const string MissingLookPropertyBit = "MissingLookPropertyBit";
           public const string TooManyAttributes = "TooManyAttributes";
           public const string GetTargetTypeOnNonAttachableMember = "GetTargetTypeOnNonAttachableMember";
           public const string SetTargetTypeOnNonAttachableMember = "SetTargetTypeOnNonAttachableMember";
           public const string InvalidExpression = "InvalidExpression";
           public const string MissingKey = "MissingKey";
           public const string PropertyDoesNotTakeText = "PropertyDoesNotTakeText";
           public const string EventCannotBeAssigned = "EventCannotBeAssigned";
           public const string TypeConverterFailed = "TypeConverterFailed";
           public const string CantCreateUnknownType = "CantCreateUnknownType";
           public const string CantSetUnknownProperty = "CantSetUnknownProperty";
           public const string MissingImplicitProperty = "MissingImplicitProperty";
           public const string BuilderStackNotEmptyOnClose = "BuilderStackNotEmptyOnClose";
           public const string CannotSetSchemaContext = "CannotSetSchemaContext";
           public const string MissingImplicitPropertyTypeCase = "MissingImplicitPropertyTypeCase";
           public const string ConstructImplicitType = "ConstructImplicitType";
           public const string NonMEWithPositionalParameters = "NonMEWithPositionalParameters";
           public const string PositionalParamsWrongLength = "PositionalParamsWrongLength";
           public const string BadStateObjectWriter = "BadStateObjectWriter";
           public const string DuplicateMemberSet = "DuplicateMemberSet";
           public const string NotAmbientProperty = "NotAmbientProperty";
           public const string NotAmbientType = "NotAmbientType";
           public const string NoSuchConstructor = "NoSuchConstructor";
           public const string UnresolvedForwardReferences = "UnresolvedForwardReferences";
           public const string CantAssignRootInstance = "CantAssignRootInstance";
           public const string ForwardRefDirectives = "ForwardRefDirectives";
           public const string TransitiveForwardRefDirectives = "TransitiveForwardRefDirectives";
           public const string TypeHasNoContentProperty = "TypeHasNoContentProperty";
           public const string GetObjectNull = "GetObjectNull";
           public const string NotAssignableFrom = "NotAssignableFrom";
           public const string NameScopeNameNotEmptyString = "NameScopeNameNotEmptyString";
           public const string NameScopeNameNotFound = "NameScopeNameNotFound";
           public const string NameScopeDuplicateNamesNotAllowed = "NameScopeDuplicateNamesNotAllowed";
           public const string NameScopeInvalidIdentifierName = "NameScopeInvalidIdentifierName";
           public const string NameScopeException = "NameScopeException";
           public const string ObjectWriterTypeNotAllowed = "ObjectWriterTypeNotAllowed";
           public const string DirectiveNotAtRoot = "DirectiveNotAtRoot";
           public const string DirectiveMustBeString = "DirectiveMustBeString";
           public const string XClassMustMatchRootInstance = "XClassMustMatchRootInstance";
           public const string SavedContextSchemaContextMismatch = "SavedContextSchemaContextMismatch";
           public const string SavedContextSchemaContextNull = "SavedContextSchemaContextNull";
           public const string SettingPropertiesIsNotAllowed = "SettingPropertiesIsNotAllowed";
           public const string LateConstructionDirective = "LateConstructionDirective";
           public const string ProvideValueCycle = "ProvideValueCycle";
           public const string AttachedPropOnFwdRefTC = "AttachedPropOnFwdRefTC";
           public const string InitializationSyntaxWithoutTypeConverter = "InitializationSyntaxWithoutTypeConverter";
           public const string NoPropertyInCurrentFrame_SO = "NoPropertyInCurrentFrame_SO";
           public const string NoPropertyInCurrentFrame_NS = "NoPropertyInCurrentFrame_NS";
           public const string NoPropertyInCurrentFrame_GO = "NoPropertyInCurrentFrame_GO";
           public const string NoPropertyInCurrentFrame_GO_noType = "NoPropertyInCurrentFrame_GO_noType";
           public const string NoPropertyInCurrentFrame_V = "NoPropertyInCurrentFrame_V";
           public const string NoPropertyInCurrentFrame_V_noType = "NoPropertyInCurrentFrame_V_noType";
           public const string OpenPropertyInCurrentFrame_EO = "OpenPropertyInCurrentFrame_EO";
           public const string OpenPropertyInCurrentFrame_SM = "OpenPropertyInCurrentFrame_SM";
           public const string NoTypeInCurrentFrame_SM = "NoTypeInCurrentFrame_SM";
           public const string NoTypeInCurrentFrame_EO = "NoTypeInCurrentFrame_EO";
           public const string NoPropertyInCurrentFrame_EM = "NoPropertyInCurrentFrame_EM";
           public const string NoPropertyInCurrentFrame_EM_noType = "NoPropertyInCurrentFrame_EM_noType";
           public const string ValueMustBeFollowedByEndMember = "ValueMustBeFollowedByEndMember";
           public const string DictionaryFirstChanceException = "DictionaryFirstChanceException";
           public const string CannotSetBaseUri = "CannotSetBaseUri";
           public const string DependsOnMissing = "DependsOnMissing";
           public const string CloseInsideTemplate = "CloseInsideTemplate";
           public const string UnexpectedClose = "UnexpectedClose";
           public const string TemplateNotCollected = "TemplateNotCollected";
           public const string DeferredPropertyNotCollected = "DeferredPropertyNotCollected";
           public const string MissingCase = "MissingCase";
           public const string NamespaceNotFound = "NamespaceNotFound";
           public const string NameScopeOnRootInstance = "NameScopeOnRootInstance";
           public const string MissingNameResolver = "MissingNameResolver";
           public const string ObjectNotTcOrMe = "ObjectNotTcOrMe";
           public const string SimpleFixupsMustHaveOneName = "SimpleFixupsMustHaveOneName";
           public const string UnexpectedTokenAfterME = "UnexpectedTokenAfterME";
           public const string WhitespaceAfterME = "WhitespaceAfterME";
           public const string UnexpectedToken = "UnexpectedToken";
           public const string NoConstructorWithNArugments = "NoConstructorWithNArugments";
           public const string MissingComma1 = "MissingComma1";
           public const string MissingComma2 = "MissingComma2";
           public const string TypeNameCannotHavePeriod = "TypeNameCannotHavePeriod";
           public const string UnexpectedNodeType = "UnexpectedNodeType";
           public const string ElementRuleException = "ElementRuleException";
           public const string EmptyElementRuleException = "EmptyElementRuleException";
           public const string EmptyPropertyElementRuleException = "EmptyPropertyElementRuleException";
           public const string StartElementRuleException = "StartElementRuleException";
           public const string ElementBodyRuleException = "ElementBodyRuleException";
           public const string NonemptyPropertyElementRuleException = "NonemptyPropertyElementRuleException";
           public const string PropertyElementRuleException = "PropertyElementRuleException";
           public const string MissingTagInNamespace = "MissingTagInNamespace";
           public const string AssemblyTagMissing = "AssemblyTagMissing";
           public const string UnknownAttributeProperty = "UnknownAttributeProperty";
           public const string NotDeclaringTypeAttributeProperty = "NotDeclaringTypeAttributeProperty";
           public const string UsableDuringInitializationOnME = "UsableDuringInitializationOnME";
           public const string TooManyAttributesOnType = "TooManyAttributesOnType";
           public const string MissingPropertyCaseClrType = "MissingPropertyCaseClrType";
           public const string UnhandledBoolTypeBit = "UnhandledBoolTypeBit";
           public const string AmbiguousCollectionItemType = "AmbiguousCollectionItemType";
           public const string AmbiguousDictionaryItemType = "AmbiguousDictionaryItemType";
           public const string MarkupExtensionWithDuplicateArity = "MarkupExtensionWithDuplicateArity";
           public const string SetOnlyProperty = "SetOnlyProperty";
           public const string XaslTypePropertiesNotImplemented = "XaslTypePropertiesNotImplemented";
           public const string AttachableMemberNotFound = "AttachableMemberNotFound";
           public const string PropertyNotImplemented = "PropertyNotImplemented";
           public const string PrefixNotFound = "PrefixNotFound";
           public const string LineNumberAndPosition = "LineNumberAndPosition";
           public const string LineNumberOnly = "LineNumberOnly";
           public const string QuoteCharactersOutOfPlace = "QuoteCharactersOutOfPlace";
           public const string UnclosedQuote = "UnclosedQuote";
           public const string MalformedPropertyName = "MalformedPropertyName";
           public const string MalformedBracketCharacters = "MalformedBracketCharacters";
           public const string InvalidClosingBracketCharacers = "InvalidClosingBracketCharacers";
           public const string MalformedBracketCharacers = "MalformedBracketCharacers";
           public const string AttributeUnhandledKind = "AttributeUnhandledKind";
           public const string UnknownType = "UnknownType";
           public const string UnknownMember = "UnknownMember";
           public const string UnknownMemberSimple = "UnknownMemberSimple";
           public const string UnknownMemberOnUnknownType = "UnknownMemberOnUnknownType";
           public const string MemberIsInternal = "MemberIsInternal";
           public const string NoAttributeUsage = "NoAttributeUsage";
           public const string NoElementUsage = "NoElementUsage";
           public const string InvalidXamlMemberName = "InvalidXamlMemberName";
           public const string ParentlessPropertyElement = "ParentlessPropertyElement";
           public const string SchemaContextNotInitialized = "SchemaContextNotInitialized";
           public const string ThreadAlreadyStarted = "ThreadAlreadyStarted";
           public const string SchemaContextNull = "SchemaContextNull";
           public const string CloseXamlWriterBeforeReading = "CloseXamlWriterBeforeReading";
           public const string CannotWriteClosedWriter = "CannotWriteClosedWriter";
           public const string WriterIsClosed = "WriterIsClosed";
           public const string DirectiveGetter = "DirectiveGetter";
           public const string BadMethod = "BadMethod";
           public const string CannotResolveTypeForFactoryMethod = "CannotResolveTypeForFactoryMethod";
           public const string CannotCreateBadType = "CannotCreateBadType";
           public const string CannotCreateBadEventDelegate = "CannotCreateBadEventDelegate";
           public const string AttachableEventNotImplemented = "AttachableEventNotImplemented";
           public const string ListNotIList = "ListNotIList";
           public const string ArrayAddNotImplemented = "ArrayAddNotImplemented";
           public const string NoAddMethodFound = "NoAddMethodFound";
           public const string MissingTypeConverter = "MissingTypeConverter";
           public const string CantSetReadonlyProperty = "CantSetReadonlyProperty";
           public const string CantGetWriteonlyProperty = "CantGetWriteonlyProperty";
           public const string XmlDataNull = "XmlDataNull";
           public const string XmlValueNotReader = "XmlValueNotReader";
           public const string NameNotFound = "NameNotFound";
           public const string MustHaveName = "MustHaveName";
           public const string MethodInvocation = "MethodInvocation";
           public const string GetValue = "GetValue";
           public const string SetValue = "SetValue";
           public const string AddCollection = "AddCollection";
           public const string AddDictionary = "AddDictionary";
           public const string SetConnectionId = "SetConnectionId";
           public const string InitializationGuard = "InitializationGuard";
           public const string SetUriBase = "SetUriBase";
           public const string ProvideValue = "ProvideValue";
           public const string SetXmlInstance = "SetXmlInstance";
           public const string GetConverterInstance = "GetConverterInstance";
           public const string DeferredLoad = "DeferredLoad";
           public const string DeferredSave = "DeferredSave";
           public const string FactoryReturnedNull = "FactoryReturnedNull";
           public const string ConstructorInvocation = "ConstructorInvocation";
           public const string NoDefaultConstructor = "NoDefaultConstructor";
           public const string NoConstructor = "NoConstructor";
           public const string DeferringLoaderInstanceNull = "DeferringLoaderInstanceNull";
           public const string TypeConverterFailed2 = "TypeConverterFailed2";
           public const string CanConvertFromFailed = "CanConvertFromFailed";
           public const string CanConvertToFailed = "CanConvertToFailed";
           public const string ShouldSerializeFailed = "ShouldSerializeFailed";
           public const string GetItemsReturnedNull = "GetItemsReturnedNull";
           public const string GetItemsException = "GetItemsException";
           public const string APSException = "APSException";
           public const string CannotReassignSchemaContext = "CannotReassignSchemaContext";
           public const string CannotSetSchemaContextNull = "CannotSetSchemaContextNull";
           public const string MissingCaseXamlNodes = "MissingCaseXamlNodes";
           public const string MarkupExtensionTypeName = "MarkupExtensionTypeName";
           public const string MarkupExtensionTypeNameBad = "MarkupExtensionTypeNameBad";
           public const string MarkupExtensionNoContext = "MarkupExtensionNoContext";
           public const string XamlXmlWriterWriteNotSupportedInCurrentState = "XamlXmlWriterWriteNotSupportedInCurrentState";
           public const string XamlXmlWriterWriteObjectNotSupportedInCurrentState = "XamlXmlWriterWriteObjectNotSupportedInCurrentState";
           public const string XamlXmlWriterPrefixAlreadyDefinedInCurrentScope = "XamlXmlWriterPrefixAlreadyDefinedInCurrentScope";
           public const string XamlXmlWriterNamespaceAlreadyHasPrefixInCurrentScope = "XamlXmlWriterNamespaceAlreadyHasPrefixInCurrentScope";
           public const string XamlXmlWriterDuplicateMember = "XamlXmlWriterDuplicateMember";
           public const string XamlXmlWriterIsObjectFromMemberSetForArraysOrNonCollections = "XamlXmlWriterIsObjectFromMemberSetForArraysOrNonCollections";
           public const string XamlXmlWriterCannotWriteNonstringValue = "XamlXmlWriterCannotWriteNonstringValue";
           public const string ExpandPositionalParametersinTypeWithNoDefaultConstructor = "ExpandPositionalParametersinTypeWithNoDefaultConstructor";
           public const string ConstructorNotFoundForGivenPositionalParameters = "ConstructorNotFoundForGivenPositionalParameters";
           public const string ExpandPositionalParametersWithReadOnlyProperties = "ExpandPositionalParametersWithReadOnlyProperties";
           public const string TypeHasInvalidXamlName = "TypeHasInvalidXamlName";
           public const string MemberHasInvalidXamlName = "MemberHasInvalidXamlName";
           public const string NamespaceDeclarationCannotBeXml = "NamespaceDeclarationCannotBeXml";
           public const string ExpandPositionalParametersWithoutUnderlyingType = "ExpandPositionalParametersWithoutUnderlyingType";
           public const string PrefixNotInFrames = "PrefixNotInFrames";
           public const string WhiteSpaceInCollection = "WhiteSpaceInCollection";
           public const string CannotWriteXmlSpacePreserveOnMember = "CannotWriteXmlSpacePreserveOnMember";
           public const string InvalidTypeString = "InvalidTypeString";
           public const string InvalidTypeListString = "InvalidTypeListString";
           public const string InvalidCharInTypeName = "InvalidCharInTypeName";
           public const string XamlTypeNameNamespaceIsNull = "XamlTypeNameNamespaceIsNull";
           public const string XamlTypeNameNameIsNullOrEmpty = "XamlTypeNameNameIsNullOrEmpty";
           public const string XamlTypeNameCannotGetPrefix = "XamlTypeNameCannotGetPrefix";
           public const string CollectionCannotContainNulls = "CollectionCannotContainNulls";
           public const string NamespaceDeclarationPrefixCannotBeNull = "NamespaceDeclarationPrefixCannotBeNull";
           public const string NamespaceDeclarationNamespaceCannotBeNull = "NamespaceDeclarationNamespaceCannotBeNull";
           public const string IncorrectGetterParamNum = "IncorrectGetterParamNum";
           public const string IncorrectSetterParamNum = "IncorrectSetterParamNum";
           public const string GetterOrSetterRequired = "GetterOrSetterRequired";
           public const string ObjectReaderDictionaryMethod1NotFound = "ObjectReaderDictionaryMethod1NotFound";
           public const string ObjectReaderXamlNamedElementAlreadyRegistered = "ObjectReaderXamlNamedElementAlreadyRegistered";
           public const string ObjectReaderXamlNameScopeResultsInClonedObject = "ObjectReaderXamlNameScopeResultsInClonedObject";
           public const string ObjectReaderXamlNamePropertyMustBeString = "ObjectReaderXamlNamePropertyMustBeString";
           public const string ObjectReaderNoDefaultConstructor = "ObjectReaderNoDefaultConstructor";
           public const string ObjectReaderNoMatchingConstructor = "ObjectReaderNoMatchingConstructor";
           public const string ObjectReaderInstanceDescriptorIncompatibleArgumentTypes = "ObjectReaderInstanceDescriptorIncompatibleArgumentTypes";
           public const string ObjectReaderInstanceDescriptorIncompatibleArguments = "ObjectReaderInstanceDescriptorIncompatibleArguments";
           public const string ObjectReaderInstanceDescriptorInvalidMethod = "ObjectReaderInstanceDescriptorInvalidMethod";
           public const string ObjectReaderTypeCannotRoundtrip = "ObjectReaderTypeCannotRoundtrip";
           public const string ObjectReaderTypeIsNested = "ObjectReaderTypeIsNested";
           public const string ObjectReaderAttachedPropertyNotFound = "ObjectReaderAttachedPropertyNotFound";
           public const string XamlFactoryInvalidXamlNode = "XamlFactoryInvalidXamlNode";
           public const string CannotAddPositionalParameters = "CannotAddPositionalParameters";
           public const string ObjectReaderMultidimensionalArrayNotSupported = "ObjectReaderMultidimensionalArrayNotSupported";
           public const string ObjectReaderTypeNotAllowed = "ObjectReaderTypeNotAllowed";
           public const string ObjectReader_TypeNotVisible = "ObjectReader_TypeNotVisible";
           public const string ExpectedObjectMarkupInfo = "ExpectedObjectMarkupInfo";
           public const string AttachedPropertyOnTypeConvertedOrStringProperty = "AttachedPropertyOnTypeConvertedOrStringProperty";
           public const string AttachedPropertyOnDictionaryKey = "AttachedPropertyOnDictionaryKey";
           public const string MissingNameProvider = "MissingNameProvider";
           public const string XamlMarkupExtensionWriterCannotSetSchemaContext = "XamlMarkupExtensionWriterCannotSetSchemaContext";
           public const string XamlMarkupExtensionWriterDuplicateMember = "XamlMarkupExtensionWriterDuplicateMember";
           public const string XamlMarkupExtensionWriterCannotWriteNonstringValue = "XamlMarkupExtensionWriterCannotWriteNonstringValue";
           public const string XamlMarkupExtensionWriterInputInvalid = "XamlMarkupExtensionWriterInputInvalid";
           public const string DefaultAttachablePropertyStoreCannotAddInstance = "DefaultAttachablePropertyStoreCannotAddInstance";
           public const string UnexpectedConstructorArg = "UnexpectedConstructorArg";
           public const string ShouldOverrideMethod = "ShouldOverrideMethod";
           public const string ExpectedQualifiedTypeName = "ExpectedQualifiedTypeName";
           public const string ExpectedQualifiedAssemblyName = "ExpectedQualifiedAssemblyName";
           public const string ExpectedLoadPermission = "ExpectedLoadPermission";
           public const string SecurityXmlUnexpectedTag = "SecurityXmlUnexpectedTag";
           public const string SecurityXmlUnexpectedValue = "SecurityXmlUnexpectedValue";
           public const string SecurityXmlMissingAttribute = "SecurityXmlMissingAttribute";
           public const string StringIsNullOrEmpty = "StringIsNullOrEmpty";
           public const string NotSupportedOnUnknownType = "NotSupportedOnUnknownType";
           public const string OnlySupportedOnCollections = "OnlySupportedOnCollections";
           public const string OnlySupportedOnDictionaries = "OnlySupportedOnDictionaries";
           public const string OnlySupportedOnCollectionsAndDictionaries = "OnlySupportedOnCollectionsAndDictionaries";
           public const string NotSupportedOnUnknownMember = "NotSupportedOnUnknownMember";
           public const string NotSupportedOnDirective = "NotSupportedOnDirective";
           public const string ArgumentRequired = "ArgumentRequired";
           public const string ConverterMustDeriveFromBase = "ConverterMustDeriveFromBase";
           public const string ReferenceIsNull = "ReferenceIsNull";
           public const string MarkupExtensionArrayType = "MarkupExtensionArrayType";
           public const string MarkupExtensionArrayBadType = "MarkupExtensionArrayBadType";
           public const string MarkupExtensionBadStatic = "MarkupExtensionBadStatic";
           public const string MarkupExtensionStaticMember = "MarkupExtensionStaticMember";
           public const string MustBeOfType = "MustBeOfType";
           public const string ToStringNull = "ToStringNull";
           public const string ConvertToException = "ConvertToException";
           public const string ConvertFromException = "ConvertFromException";
           public const string ServiceTypeAlreadyAdded = "ServiceTypeAlreadyAdded";
           public const string QualifiedNameHasWrongFormat = "QualifiedNameHasWrongFormat";
           public const string ParserAttributeArgsHigh = "ParserAttributeArgsHigh";
           public const string ParserAttributeArgsLow = "ParserAttributeArgsLow";
           public const string ParserAssemblyLoadVersionMismatch = "ParserAssemblyLoadVersionMismatch";
           public const string FrugalList_TargetMapCannotHoldAllData = "FrugalList_TargetMapCannotHoldAllData";
           public const string FrugalList_CannotPromoteBeyondArray = "FrugalList_CannotPromoteBeyondArray";
           public const string ValueInArrayIsNull = "ValueInArrayIsNull";
           public const string InvalidEvent = "InvalidEvent";


    }//endof struct SRID

    internal static class SR
    {
         internal static string Get(string id)
         {
             string message = _resourceManager.GetString(id);
             if (message == null)
             {
                 // Try for Unavailable message
                 message = _resourceManager.GetString("Unavailable");
             }
             return message;
         }

         internal static string Get( string id, params object[] args)
         {
             string message = _resourceManager.GetString(id);
             if (message == null)
             {
                 // Try for Unavailable message
                 message = _resourceManager.GetString("Unavailable");
             }
             else
             {
                 // Apply arguments to formatted string (if applicable)
                 if (args != null && args.Length > 0)
                 {
                     message = String.Format(CultureInfo.CurrentCulture, message, args);
                 }
             }
             return message;
         }

         internal static ResourceManager ResourceManager
         {
            get { return _resourceManager; }
         }

         // Get exception string resources for current locale
         private static ResourceManager _resourceManager = new ResourceManager("ExceptionStringTable", typeof(SR).Assembly);

    }//endof class SR
}//endof namespace
