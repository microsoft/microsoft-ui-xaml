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
           public const string DuplicateXmlnsCompatAcrossAssemblies = "DuplicateXmlnsCompatAcrossAssemblies";
           public const string DuplicateMemberSet = "DuplicateMemberSet";
           public const string UnexpectedTokenAfterME = "UnexpectedTokenAfterME";
           public const string WhitespaceAfterME = "WhitespaceAfterME";
           public const string UnexpectedToken = "UnexpectedToken";
           public const string MissingComma1 = "MissingComma1";
           public const string MissingComma2 = "MissingComma2";
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
           public const string UsableDuringInitializationOnME = "UsableDuringInitializationOnME";
           public const string TooManyAttributesOnType = "TooManyAttributesOnType";
           public const string AmbiguousCollectionItemType = "AmbiguousCollectionItemType";
           public const string AmbiguousDictionaryItemType = "AmbiguousDictionaryItemType";
           public const string MarkupExtensionWithDuplicateArity = "MarkupExtensionWithDuplicateArity";
           public const string PrefixNotFound = "PrefixNotFound";
           public const string LineNumberAndPosition = "LineNumberAndPosition";
           public const string LineNumberOnly = "LineNumberOnly";
           public const string QuoteCharactersOutOfPlace = "QuoteCharactersOutOfPlace";
           public const string UnclosedQuote = "UnclosedQuote";
           public const string MalformedPropertyName = "MalformedPropertyName";
           public const string MalformedBracketCharacters = "MalformedBracketCharacters";
           public const string InvalidClosingBracketCharacers = "InvalidClosingBracketCharacers";
           public const string AttributeUnhandledKind = "AttributeUnhandledKind";
           public const string InvalidXamlMemberName = "InvalidXamlMemberName";
           public const string ParentlessPropertyElement = "ParentlessPropertyElement";
           public const string MissingCaseXamlNodes = "MissingCaseXamlNodes";
           public const string InvalidTypeString = "InvalidTypeString";
           public const string InvalidCharInTypeName = "InvalidCharInTypeName";
           public const string XamlTypeNameNamespaceIsNull = "XamlTypeNameNamespaceIsNull";
           public const string XamlTypeNameNameIsNullOrEmpty = "XamlTypeNameNameIsNullOrEmpty";
           public const string XamlTypeNameCannotGetPrefix = "XamlTypeNameCannotGetPrefix";
           public const string CollectionCannotContainNulls = "CollectionCannotContainNulls";
           public const string IncorrectGetterParamNum = "IncorrectGetterParamNum";
           public const string IncorrectSetterParamNum = "IncorrectSetterParamNum";
           public const string GetterOrSetterRequired = "GetterOrSetterRequired";
           public const string UnexpectedConstructorArg = "UnexpectedConstructorArg";
           public const string ArgumentRequired = "ArgumentRequired";
           public const string ReferenceIsNull = "ReferenceIsNull";


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
