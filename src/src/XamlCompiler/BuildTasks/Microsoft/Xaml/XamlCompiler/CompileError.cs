// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using Properties;
    using Utilities;
    using XamlDom;

    // Never delete error codes from this enum.
    // Rather, mark them as "reserved" when we no longer need them.
    internal enum ErrorCode
    {
        // Validation and other XAML analysis errors 001 - 499
        WMC0001 = 0001,
        WMC0003 = 0003,
        WMC0005 = 0005,
        WMC0010 = 0010,
        WMC0011 = 0011,
        WMC0015 = 0015,
        WMC0020 = 0020,
        WMC0021 = 0021,
        WMC0025 = 0025,
        WMC0026 = 0026,
        WMC0027 = 0027,
        WMC0030 = 0030,
        WMC0035 = 0035,
        WMC0040 = 0040,
        WMC0045 = 0045,
        WMC0046 = 0046,
        WMC0047 = 0047,
        WMC0050 = 0050,
        WMC0055 = 0055,
        WMC0056 = 0056,
        WMC0060 = 0060,
        WMC0065 = 0065,
        WMC0070 = 0070,
        WMC0075 = 0075,
        WMC0080 = 0080,
        WMC0085 = 0085,
        WMC0086 = 0086,
        WMC0090 = 0090,
        WMC0091 = 0091,
        WMC0095 = 0095,
        WMC0100 = 0100,
        WMC0105 = 0105,
        WMC0110 = 0110,
        WMC0115 = 0115,
        WMC0120 = 0120,
        WMC0121 = 0121,
        WMC0125 = 0125,
        WMC0130 = 0130,
        WMC0131 = 0131,
        WMC0132 = 0132,
        WMC0140 = 0140,
        WMC0141 = 0141,
        WMC0142 = 0142,
        WMC0145 = 0145,
        WMC0150 = 0150,
        WMC0153 = 0153,
        WMC0154 = 0154,
        WMC0155 = 0155,

        // XAML rewriter Errors 500 - 599
        WMC0500 = 0500,
        WMC0501 = 0501,
        WMC0502 = 0502,
        WMC0503 = 0503,
        WMC0504 = 0504,
        WMC0505 = 0505,

        // XBF Error messages 600 - 699
        WMC0600 = 0600,
        WMC0601 = 0601,
        WMC0605 = 0605,
        WMC0610 = 0610,
        WMC0612 = 0612,
        WMC0615 = 0615,
        WMC0620 = 0620,
        WMC0621 = 0621,

        // Schema Errors 800 - 899
        WMC0800 = 0800,
        WMC0805 = 0805,
        WMC0806 = 0806,
        WMC0810 = 0810,
        WMC0815 = 0815,
        WMC0820 = 0820,
        WMC0821 = 0821,
        WMC0822 = 0822,

        // Validation Errors 900-999
        WMC0901 = 0901,
        WMC0902 = 0902,
        WMC0903 = 0903,
        WMC0905 = 0905,
        WMC0906 = 0906,
        WMC0907 = 0907,
        WMC0908 = 0908,
        WMC0909 = 0909,
        WMC0910 = 0910,
        WMC0911 = 0911,
        WMC0912 = 0912,
        WMC0913 = 0913,
        WMC0914 = 0914,
        WMC0915 = 0915,
        WMC0916 = 0916,
        WMC0917 = 0917,
        WMC0918 = 0918,
        WMC0919 = 0919,

        // CompileXaml top level errors 1000 - 1100
        WMC1002 = 1002,
        WMC1003 = 1003,
        WMC1005 = 1005,
        WMC1006 = 1006,
        WMC1007 = 1007,
        WMC1008 = 1008,
        WMC1009 = 1009,
        WMC1010 = 1010,
        WMC1011 = 1011,
        WMC1012 = 1012,
        WMC1013 = 1013,

        // x:Bind 1100 - 1200
        WMC1110 = 1110,
        WMC1111 = 1111,
        WMC1112 = 1112,
        WMC1113 = 1113,
        WMC1114 = 1114,
        WMC1115 = 1115,
        WMC1116 = 1116,
        WMC1117 = 1117,
        WMC1118 = 1118,
        WMC1119 = 1119,
        WMC1120 = 1120,
        WMC1121 = 1121,
        WMC1122 = 1122,
        WMC1123 = 1123,
        WMC1124 = 1124,
        WMC1125 = 1125,

        // Warnings placed incorrectly among errors
        WMC0151 = 0151,
        WMC0152 = 0152,
        WMC1001 = 1001,
        WMC1004 = 1004,
        WMC1014 = 1014,

        // Warnings 1500+
        WMC1500 = 1500,
        WMC1501 = 1501,
        WMC1502 = 1502,
        WMC1503 = 1503,
        WMC1504 = 1504,
        WMC1505 = 1505,
        WMC1506 = 1506,
        WMC1507 = 1507,
        WMC1508 = 1508,
        WMC1509 = 1509,

        // Xaml Compiler Internal error and other missplaced errors
        WMC9997 = 9997,
        WMC9998 = 9998,
        WMC9999 = 9999,
    };

    internal static class ErrorCodeExtension
    {
        public static string AsErrorCode(this ErrorCode code)
        {
            return AsErrorCode((int) code);
        }

        public static string AsErrorCode(this int code)
        {
            return $"WMC{code.ToString("D4")}";
        }
    }

    internal class XamlCompileError : XamlCompileErrorBase
    {
        protected XamlCompileError(ErrorCode code)
        : base(code, null, 0, 0)
        { }

        public XamlCompileError(ErrorCode code, IXamlDomNode domNode)
        : base(code, domNode?.SourceFilePath ?? null, domNode?.StartLineNumber ?? 0, domNode?.StartLinePosition ?? 0)
        { }

        public XamlCompileError(ErrorCode code, int lineNumber, int lineOffset)
        : base(code, null, lineNumber, lineOffset)
        { }

        protected XamlCompileError(ErrorCode code, string fileName, int lineNumber, int lineOffset)
        : base(code, fileName, lineNumber, lineOffset)
        {}
    }

    internal class XamlCompileWarning : XamlCompileErrorBase
    {
        public XamlCompileWarning(ErrorCode code, IXamlDomNode domNode)
            : base(code, domNode.SourceFilePath, domNode.StartLineNumber, domNode.StartLinePosition)
        { }

        protected XamlCompileWarning(ErrorCode code)
            : base(code, null, 0, 0)
        { }
    }

    internal class XamlCompileErrorBase
    {
        public XamlCompileErrorBase(ErrorCode code, string fileName, int lineNumber, int lineOffset)
        {
            Code = code;
            FileName = fileName;
            LineNumber = lineNumber;
            LineOffset = lineOffset;
        }

        public ErrorCode Code
        {
            get;
            protected set;
        }

        public String Message
        {
            get;
            protected set;
        }

        public string FileName
        {
            get;
            protected set;
        }

        public int LineNumber
        {
            get;
            private set;
        }

        public int LineOffset
        {
            get;
            private set;
        }
    }

    internal class XamlValidationErrorUnknownObject : XamlCompileError
    {
        public XamlValidationErrorUnknownObject(XamlDomObject domObject)
            : base(ErrorCode.WMC0001, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnknownObject, domObject.Type.Name, domObject.Type.PreferredXamlNamespace);
        }
    }

    internal class XamlValidationErrorUnresolvedForwardedTypeAssembly : XamlCompileError
    {
        public XamlValidationErrorUnresolvedForwardedTypeAssembly(XamlDomMember domMember, string errorMessage)
            : base(ErrorCode.WMC0003, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnresolvedForwardedTypeAssembly, errorMessage);
        }

        public XamlValidationErrorUnresolvedForwardedTypeAssembly(XamlDomObject domObject, string errorMessage)
    : base(ErrorCode.WMC0003, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnresolvedForwardedTypeAssembly, errorMessage);
        }
    }

    internal class XamlValidationErrorNonPublicType : XamlCompileError
    {
        public XamlValidationErrorNonPublicType(XamlDomObject domObject)
            : base(ErrorCode.WMC0005, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAccessNonPublicType, domObject.Type.Name, domObject.Type.PreferredXamlNamespace);
        }
    }

    internal class XamlValidationErrorUnknownMember : XamlCompileError
    {
        public XamlValidationErrorUnknownMember(XamlDomObject domObject, XamlDomMember domMember)
            : base(ErrorCode.WMC0010, domMember)
        {
            XamlMember member = domMember.Member;
            XamlType ownerType = domMember.Parent.Type;
            XamlType declaringType = member.DeclaringType;

            if (declaringType != null && member.IsAttachable)
            {
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnknownAttachableMember,
                                declaringType.Name, member.Name, ownerType.Name);
            }
            else
            {
                Code = ErrorCode.WMC0011;
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnknownMember,
                                member.Name, ownerType.Name);
            }
        }
    }

    internal class XamlValidationErrorAssignment : XamlCompileError
    {
        public XamlValidationErrorAssignment(XamlDomObject domChildObject, XamlMember property, XamlType propertyItemType)
            : base(ErrorCode.WMC0015, domChildObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAssign,
                                domChildObject.Type.Name, property.Name, propertyItemType.Name);
        }
    }

    internal class XamlValidationErrorCollectionAdd : XamlCompileError
    {
        public XamlValidationErrorCollectionAdd(XamlDomItem domChildItem, XamlType itemType, XamlDomObject collectionObject, XamlDomMember collectionMember)
            : base(ErrorCode.WMC0020, domChildItem)
        {
            XamlDomValue domChildValue = domChildItem as XamlDomValue;
            XamlDomObject domChildObject = domChildItem as XamlDomObject;

            string childName = (domChildValue != null) ? (domChildValue.Value as String) : domChildObject.Type.Name;

            if (collectionObject.IsGetObject)
            {
                Code = ErrorCode.WMC0020;
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAddToCollectionProperty,
                        childName, collectionMember.Member.Name, itemType.Name);
            }
            else
            {
                Code = ErrorCode.WMC0021;
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAddToCollectionObject,
                        childName, collectionObject.Type.Name, itemType.Name);
            }
        }
    }

    internal class XamlValidationErrorDictionaryAdd : XamlCompileError
    {
        // Explicit Dictionary
        public XamlValidationErrorDictionaryAdd(XamlDomValue domChildValue)
            : base(ErrorCode.WMC0025, domChildValue)
        {
            string text = domChildValue.Value as String;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_DictionaryItemsCannotBeText, text);
        }

        public XamlValidationErrorDictionaryAdd(XamlDomObject domChildObject, XamlType itemType, XamlDomObject domDictionaryObject, XamlDomMember domDictionaryProperty)
            : base(ErrorCode.WMC0026, domChildObject)
        {
            if (domDictionaryObject.IsGetObject)
            {
                Code = ErrorCode.WMC0026;
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAddToDictionaryProperty,
                        domChildObject.Type.Name, domDictionaryProperty.Member.Name, itemType.Name);
            }
            else
            {
                Code = ErrorCode.WMC0027;
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAddToDictionaryObject,
                    domChildObject.Type.Name, domDictionaryObject.Type.Name, itemType.Name);
            }
        }
    }

    internal class XamlValidationIdPropertiesMustBeText : XamlCompileError
    {
        public XamlValidationIdPropertiesMustBeText(XamlDomMember domMember)
            : base(ErrorCode.WMC0030, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_IdPropertiesMustBeText, domMember.Member.Name);
        }
    }

    internal class XamlValidationErrorDuplicateAssigment : XamlCompileError
    {
        public XamlValidationErrorDuplicateAssigment(XamlDomMember domMember)
            : base(ErrorCode.WMC0035, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_DuplicationAssignment, domMember.Member.Name, domMember.Parent.Type.Name);
        }
    }

    internal class XamlValidationErrorBadName : XamlCompileError
    {
        // used for Name and Class
        public XamlValidationErrorBadName(XamlDomMember domMember, String name)
            : base(ErrorCode.WMC0040, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_BadName, name, domMember.Member.Name, domMember.Parent.Type.Name);
        }

        public XamlValidationErrorBadName(XamlDomMember domMember, String name, char badChar)
            : base(ErrorCode.WMC0040, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_BadNameChar, name, domMember.Member.Name, domMember.Parent.Type.Name, badChar);
        }
    }

    internal class XamlValidationErrorCannotNameValueTypes : XamlCompileError
    {
        public XamlValidationErrorCannotNameValueTypes(XamlDomObject domObject)
            : base(ErrorCode.WMC0045, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantNameValueTypes, domObject.Type.Name);
        }
    }

    internal class XamlValidationErrorCannotNameElementTwice : XamlCompileError
    {
        public XamlValidationErrorCannotNameElementTwice(XamlDomObject domObject)
                    : base(ErrorCode.WMC0046, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CannotNameElementTwice);
        }
    }

    internal class XamlValidationErrorElementNameAlreadyUsed : XamlCompileError
    {
        public XamlValidationErrorElementNameAlreadyUsed(XamlDomObject domObject, string duplicateName)
                    : base(ErrorCode.WMC0047, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_ElementNameAlreadyUsed, duplicateName);
        }
    }

    internal class XamlValidationErrorCannotAssignToReadOnlyProperty : XamlCompileError
    {
        public XamlValidationErrorCannotAssignToReadOnlyProperty(XamlDomMember domMember)
            : base(ErrorCode.WMC0050, domMember)
        {
            var domValue = domMember.Items[0] as XamlDomValue;
            var domObject = domMember.Items[0] as XamlDomObject;
            string value = (domValue != null) ? domValue.Value as String : domObject.Type.Name;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAssignToReadOnlyProperty, value, domMember.Member.Name);
        }
        public XamlValidationErrorCannotAssignToReadOnlyProperty(XamlDomNode location, XamlMember member, String value)
            : base(ErrorCode.WMC0050, location)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAssignToReadOnlyProperty, value, member.Name);
        }
    }

    internal class XamlValidationErrorCannotAssignTextToProperty : XamlCompileError
    {
        public XamlValidationErrorCannotAssignTextToProperty(XamlDomNode location, XamlMember member, String value)
            : base(ErrorCode.WMC0055, location)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantAssignTextToProperty, value, member.Name, member.Type.Name);
        }
    }

    internal class XamlValidationErrorCannotAssignNullableProperty : XamlCompileError
    {
        public XamlValidationErrorCannotAssignNullableProperty(XamlDomNode location, XamlMember member)
            : base(ErrorCode.WMC0056, location)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_NullablePropertyType, member.Name);
        }
    }

    internal class XamlValidationDictionaryKeyError : XamlCompileError
    {
        public XamlValidationDictionaryKeyError(XamlDomObject domObject)
            : base(ErrorCode.WMC0060, domObject)
        {
            string objectName = domObject.Type.Name;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_DictionaryItemsMustHaveKeys, objectName);
        }

        public XamlValidationDictionaryKeyError(XamlDomObject domObject, string keyText)
            : base(ErrorCode.WMC0065, domObject)
        {
            string objectName = domObject.Type.Name;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_DictionaryItemsHasDuplicateKey, objectName, keyText);
        }
    }

    internal class XamlValidationErrorBadCPA : XamlCompileError
    {
        public XamlValidationErrorBadCPA(XamlDomObject domObject, string cpaName)
            : base(ErrorCode.WMC0070, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_InvalidCPA, domObject.Type.Name, cpaName);
        }
    }

    internal class XamlValidationErrorMissingCPA : XamlCompileError
    {
        public XamlValidationErrorMissingCPA(XamlDomObject domParentObject, XamlDomItem firstChild)
            : base(ErrorCode.WMC0075, firstChild)
        {
            var domValue = firstChild as XamlDomValue;
            var domObject = firstChild as XamlDomObject;
            string value = (domValue != null) ? domValue.Value as String : domObject.Type.Name;

            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_MissingCPA, domParentObject.Type.Name, value);
        }
    }

    internal class XamlValidationErrorStyleMustHaveTargetType : XamlCompileError
    {
        public XamlValidationErrorStyleMustHaveTargetType(XamlDomNode styleOrTargetType)
            : base(ErrorCode.WMC0080, styleOrTargetType)
        {
            Message = XamlCompilerResources.XamlCompiler_StyleMustHaveTargetType;
        }
    }

    internal class XamlValidationErrorSetterMissingField : XamlCompileError
    {
        public XamlValidationErrorSetterMissingField(XamlDomNode setterOrProperty, bool isProperty)
            : base(ErrorCode.WMC0085, setterOrProperty)
        {
            if (isProperty)
            {
                Code = ErrorCode.WMC0085;
                Message = XamlCompilerResources.XamlCompiler_SettersMustHaveProperty;
            }
            else
            {
                Code = ErrorCode.WMC0086;
                Message = XamlCompilerResources.XamlCompiler_SetterMustHaveValue;
            }
        }
    }

    internal class XamlValidationErrorSetterUnknownMember : XamlCompileError
    {
        public XamlValidationErrorSetterUnknownMember(XamlDomMember domPropertyMember, XamlType xamlTargetType, string propertyName)
            : base(ErrorCode.WMC0090, domPropertyMember)
        {
            if (propertyName.IndexOf('.') == -1)
            {
                Code = ErrorCode.WMC0090;
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnknownMember,
                                                propertyName, xamlTargetType.Name);
            }
            else
            {
                Code = ErrorCode.WMC0091;
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnknownSetterAttachableMember,
                                                propertyName, xamlTargetType.Name);
            }
        }
    }

    internal class XamlValidationErrorSetterSetterPropertyMustBeDP : XamlCompileError
    {
        public XamlValidationErrorSetterSetterPropertyMustBeDP(XamlDomMember domPropertyMember, String propertyName)
            : base(ErrorCode.WMC0095, domPropertyMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_SetterPropertyMustBeDP, propertyName);
        }
    }

    internal class XamlValidationErrorNotConstructibleObject : XamlCompileError
    {
        public XamlValidationErrorNotConstructibleObject(XamlDomObject domObject, XamlType xamlType)
            : base(ErrorCode.WMC0100, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_NotConstructibleObj, xamlType.Name);
        }
    }

    internal class XamlCompilerTypeMustHaveANamespace : XamlCompileError
    {
        public XamlCompilerTypeMustHaveANamespace(XamlDomObject domObject)
            : base(ErrorCode.WMC0105, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_TypeMustHaveANamespace, domObject.Type.Name);
        }
    }

    internal class XamlValidationErrorUnknownStyleTargetType : XamlCompileError
    {
        public XamlValidationErrorUnknownStyleTargetType(XamlDomMember targetTypeMember, String typeName)
            : base(ErrorCode.WMC0110, targetTypeMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnknownStyleTargetType, typeName);
        }
    }

    internal class XamlCompilerErrorProcessingStyle : XamlCompileError
    {
        public XamlCompilerErrorProcessingStyle(XamlDomObject domStyle)
            : base(ErrorCode.WMC0115, domStyle)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_InternalErrorProcessingStyle);
        }
    }

    internal class XamlCompileErrorInvalidPropertyType : XamlCompileError
    {
        public XamlCompileErrorInvalidPropertyType(XamlDomMember domMember)
            : base(ErrorCode.WMC0120, domMember)
        {
            String typeName = domMember.Member.Type.Name;
            String propertyName = domMember.Member.Name;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_InvalidPropertyType, typeName, propertyName);
        }
    }

    internal class XamlCompileErrorInvalidPropertyType_SignedChar : XamlCompileError
    {
        public XamlCompileErrorInvalidPropertyType_SignedChar(XamlDomMember domMember)
            : base(ErrorCode.WMC0121, domMember)
        {
            String typeName = domMember.Member.Type.Name;
            String propertyName = domMember.Member.Name;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_InvalidSignedChar, typeName, propertyName);
        }
    }

    internal class XamlValidationErrorEventValuesMustBeText : XamlCompileError
    {
        public XamlValidationErrorEventValuesMustBeText(XamlDomNode domNode, string eventName)
        : base(ErrorCode.WMC0125, domNode)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_EventValuesMustBeText, eventName);
        }
    }

    internal class XamlValidationErrorClassMustHaveANamespace : XamlCompileError
    {
        public XamlValidationErrorClassMustHaveANamespace(XamlDomMember domMember, string classname)
            : base(ErrorCode.WMC0130, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_EventValuesMustBeText, classname);
        }
    }

    internal class XamlValidationErrorClassNameEmptyPathPart : XamlCompileError
    {
        public XamlValidationErrorClassNameEmptyPathPart(XamlDomMember domMember, string classname)
            : base(ErrorCode.WMC0131, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.Harvester_ClassNameEmptyPathPart, classname);
        }
    }

    internal class XamlValidationErrorClassNameNoWhiteSpace : XamlCompileError
    {
        public XamlValidationErrorClassNameNoWhiteSpace(XamlDomMember domMember, string classname)
            : base(ErrorCode.WMC0132, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.Harvester_ClassNameNoWhiteSpace, classname);
        }
    }

    internal class XamlValidationErrorStyleBasedOnMustBeStyle : XamlCompileError
    {
        public XamlValidationErrorStyleBasedOnMustBeStyle(XamlDomObject styleObject, string keyString, XamlDomObject domBaseStyleObject, string otherFile)
            : base(ErrorCode.WMC0140, styleObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_StyleBasedOnMustBeStyle_SR, keyString, domBaseStyleObject.Type.Name);
        }
        public XamlValidationErrorStyleBasedOnMustBeStyle(XamlDomObject styleObject, XamlDomObject domNotStyleObject)
            : base(ErrorCode.WMC0141, styleObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_StyleBasedOnMustBeStyle_BadObj, domNotStyleObject.Type.Name);
        }
        public XamlValidationErrorStyleBasedOnMustBeStyle(XamlDomObject styleObject, string text)
            : base(ErrorCode.WMC0142, styleObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_StyleBasedOnMustBeStyle_Text, text);
        }
    }

    internal class XamlValidationErrorStyleBasedOnBadStyleTargetType : XamlCompileError
    {
        public XamlValidationErrorStyleBasedOnBadStyleTargetType(XamlDomNode styleObject, XamlType targetType, XamlType basedOnTargetType)
            : base(ErrorCode.WMC0145, styleObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_StyleBasedOnBadStyleTargetType, targetType.Name, basedOnTargetType.Name);
        }
    }

    internal class XamlValidationErrorDeprecated : XamlCompileError
    {
        public XamlValidationErrorDeprecated(XamlDomObject domObject, string name, string message)
            : base(ErrorCode.WMC0150, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_Deprecated, name, message);
        }

        public XamlValidationErrorDeprecated(XamlDomMember domMember, string name, string message)
            : base(ErrorCode.WMC0150, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_Deprecated, name, message);
        }
    }

    internal class XamlValidationErrorWrongContract : XamlCompileWarning
    {
        public XamlValidationErrorWrongContract(XamlDomObject domObject, string typeName, string contractName, string runtimeVer, string parseVer)
            : base(ErrorCode.WMC0151, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_WrongTypeContract, typeName, contractName, runtimeVer, parseVer);
        }

        public XamlValidationErrorWrongContract(XamlDomMember domMember, string typeName, string contractName, string runtimeVer, string parseVer)
            : base(ErrorCode.WMC0151, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_WrongMemberContract, typeName, contractName, runtimeVer, parseVer, domMember.Member.Name);
        }
    }

    internal class XamlValidationErrorContractDoesNotExist : XamlCompileWarning
    {
        public XamlValidationErrorContractDoesNotExist(XamlDomObject domObject, string typeName, string contractName, string runtimeVer)
            : base(ErrorCode.WMC0152, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_TypeContractDoesNotExist, typeName, contractName, runtimeVer);
        }

        public XamlValidationErrorContractDoesNotExist(XamlDomMember domMember, string typeName, string contractName, string runtimeVer)
            : base(ErrorCode.WMC0152, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_MemberContractDoesNotExist, typeName, contractName, runtimeVer, domMember.Member.Name);
        }
    }

    internal class XamlValidationErrorAmbiguousEvent : XamlCompileError
    {
        public XamlValidationErrorAmbiguousEvent(XamlDomMember domMember)
            : base(ErrorCode.WMC0154, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlValidationError_AmbiguousEvent);
        }
    }

    internal class XamlSuccinctSyntaxError : XamlCompileError
    {
        public XamlSuccinctSyntaxError(int line, int col, string offendingToken, string fileName)
            : base(ErrorCode.WMC0155, fileName, line, col)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlValidationError_SuccinctSyntaxError, line, col, offendingToken);
        }
    }

    internal class XamlRewriterErrorEventLongForm : XamlCompileError
    {
        public XamlRewriterErrorEventLongForm(int line, int column)
            : base(ErrorCode.WMC0500, line, column)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlRewriter_EventsCannotBeInElementForm);
        }
    }

    internal class XamlRewriterErrorLineBreak : XamlCompileError
    {
        public XamlRewriterErrorLineBreak(int line, int column)
            : base(ErrorCode.WMC0501, line, column)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlRewriter_EventsAcrossLine);
        }
    }
    internal class XamlRewriterErrorFileOpenFailure : XamlCompileError
    {
        public XamlRewriterErrorFileOpenFailure(string xamlFileName, string message)
            : base(ErrorCode.WMC0502, xamlFileName, 0, 0)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_FileOpenFailure, message);
        }
    }

    internal class XamlRewriterErrorCompiledBindingLongForm : XamlCompileError
    {
        public XamlRewriterErrorCompiledBindingLongForm(int line, int column)
            : base(ErrorCode.WMC0503, line, column)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlRewriter_CompiledBindingsCannotBeInElementForm);
        }
    }

    internal class XamlRewriterErrorDataTypeLongForm : XamlCompileError
    {
        public XamlRewriterErrorDataTypeLongForm(int line, int column)
            : base(ErrorCode.WMC0504, line, column)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlRewriter_XamlRewriterErrorDataTypeLongForm);
        }
    }

    internal class XbfOutputFileOpenFailure : XamlCompileError
    {
        public XbfOutputFileOpenFailure(string xbfFile, string message)
            : base(ErrorCode.WMC0600, xbfFile, 0, 0)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XbfGeneration_XbfOutputFileOpenFailure, message);
        }
    }

    internal class XbfInputFileOpenFailure : XamlCompileError
    {
        public XbfInputFileOpenFailure(string xbfFile, string message)
            : base(ErrorCode.WMC0601, xbfFile, 0, 0)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XbfGeneration_XamlInputFileOpenFailure, message);
        }
    }

    internal class XbfGenerationGeneralFailure : XamlCompileError
    {
        public XbfGenerationGeneralFailure(string message)
            : base(ErrorCode.WMC0605)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XbfGeneration_GeneralFailure, message);
        }
    }

    internal class XbfGenerationParseError : XamlCompileError
    {
        public XbfGenerationParseError(string fileName, int line, int column, int xbfErrorCode)
            : base(ErrorCode.WMC0610, fileName, line, column)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XbfGeneration_SyntaxError, "0x" + xbfErrorCode.ToString("x4"));
        }
    }

    internal class XbfGenerationPropertyNotFoundError : XamlCompileError
    {
        public XbfGenerationPropertyNotFoundError(string fileName, int line, int column)
            : base(ErrorCode.WMC0612, fileName, line, column)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XbfGeneration_PropertyNotFoundError);
        }
    }

    internal class XbfGeneration_NonMeInCurlyBraces : XamlCompileError
    {
        public XbfGeneration_NonMeInCurlyBraces(string fileName, int line, int column, string nonMeName, int xbfErrorCode)
            : base(ErrorCode.WMC0615, fileName, line, column)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XbfGeneration_SyntaxErrorME, nonMeName, "0x" + xbfErrorCode.ToString("x4"));
        }
    }

    internal class XbfGeneration_NoWindowsSdk : XamlCompileError
    {
        public XbfGeneration_NoWindowsSdk()
            : base(ErrorCode.WMC0620)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XbfGeneration_MissingGenXbfPath);
        }
    }

    internal class XbfGeneration_CouldNotLoadXbfGenerator : XamlCompileError
    {
        public XbfGeneration_CouldNotLoadXbfGenerator(string path)
            : base(ErrorCode.WMC0621, path, 0, 0)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XbfGeneration_CouldNotLoadXbfGenerator, path);
        }
    }

    internal class XamlSchemaError_BadBindablePropertyProvider : XamlCompileError
    {
        public XamlSchemaError_BadBindablePropertyProvider(string typeName)
            : base(ErrorCode.WMC0800)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.DuiSchema_BadBindablePropertyProvider, typeName);
        }
    }

    internal class XamlSchemaError_TypeLoadException : XamlCompileError
    {
        public XamlSchemaError_TypeLoadException(string typeName, string asmName)
            : base(ErrorCode.WMC0805)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.DuiSchema_TypeLoadException, typeName, asmName);
        }

        // this version has line numbers.
        public XamlSchemaError_TypeLoadException(XamlDomObject domObject, string typeName, string innerMessage)
            : base(ErrorCode.WMC0806, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.DuiSchema_TypeLoadExceptionMessage, typeName, innerMessage);
        }
    }

    internal class XamlSchemaError_CustomAttributesTypeLoadException : XamlCompileError
    {
        public XamlSchemaError_CustomAttributesTypeLoadException(string asmName)
            : base(ErrorCode.WMC0810)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.DuiSchema_CustomAttributesTypeLoadException, asmName);
        }
    }

    internal class XamlSchemaError_WRTAssembliesMissing : XamlCompileError
    {
        public XamlSchemaError_WRTAssembliesMissing()
            : base(ErrorCode.WMC0815)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.DuiSchema_WRTAssembliesMissing);
        }
    }

    internal class XamlSchemaError_AmbiguousCollectionAdd : XamlCompileError
    {
        public XamlSchemaError_AmbiguousCollectionAdd(string typeName, string methodName, int argumentCount)
            : base(ErrorCode.WMC0820)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.DuiSchema_AmbiguousCollectionAdd, typeName, methodName, argumentCount);
        }
    }

    internal class XamlSchemaError_BindableNotSupportedOnGeneric : XamlCompileError
    {
        public XamlSchemaError_BindableNotSupportedOnGeneric(string typeName)
            : base(ErrorCode.WMC0821)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.DuiSchema_BindableNotSupportedOnGeneric, typeName);
        }
    }

    internal class XamlSchemaError_UnknownTypeError : XamlCompileError
    {
        public XamlSchemaError_UnknownTypeError(string typeName)
            : base(ErrorCode.WMC0822)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_UnknownTypeError, typeName);
        }
    }


    internal class XamlValidationErrorInvalidFieldModifier : XamlCompileError
    {
        public XamlValidationErrorInvalidFieldModifier(XamlDomObject domObject, string invalidModifier)
            : base(ErrorCode.WMC0905, domObject)
        {
            XamlDomMember nameMember = DomHelper.GetAliasedMemberNode(domObject, XamlLanguage.Name);

            // If this node isn't named, we tell them about the object
            if (nameMember == null)
            {
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_InvalidFieldModifier, invalidModifier, domObject.Type.Name);
            }
            else
            {
                Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_InvalidFieldModifier, invalidModifier, DomHelper.GetStringValueOfProperty(nameMember));
            }
        }
    }

    internal class XamlValidationError_DeferLoadStrategyInvalidValue : XamlCompileError
    {
        public XamlValidationError_DeferLoadStrategyInvalidValue(XamlDomMember domMember)
            : base(ErrorCode.WMC0906, domMember)
        {
            XamlMember member = domMember.Member;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlValidationError_DeferLoadStrategyInvalidValue, member.Name);
        }
    }

    internal class XamlValidationError_LoadInvalidValue : XamlCompileError
    {
        public XamlValidationError_LoadInvalidValue(XamlDomMember domMember)
            : base(ErrorCode.WMC0906, domMember)
        {
            XamlMember member = domMember.Member;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlValidationError_InvalidAttributeValue, member.Name);
        }
    }

    internal class XamlValidationError_DeferLoadStrategyMissingXName : XamlCompileError
    {
        public XamlValidationError_DeferLoadStrategyMissingXName(XamlDomObject domObject)
            : base(ErrorCode.WMC0907, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_DeferLoadStrategyMissingXName);
        }
    }

    internal class XamlValidationError_LoadMissingName : XamlCompileError
    {
        public XamlValidationError_LoadMissingName(XamlDomObject domObject)
            : base(ErrorCode.WMC0907, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_LoadMissingName);
        }
    }

    internal class XamlValidationError_DataTypeOnlyAllowedOnDataTemplate : XamlCompileError
    {
        public XamlValidationError_DataTypeOnlyAllowedOnDataTemplate(XamlDomObject domObject)
            : base(ErrorCode.WMC0908, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlValidationError_DataTypeOnlyAllowedOnDataTemplate);
        }
    }

    internal class XamlValidationError_CantResolveDataType : XamlCompileError
    {
        public XamlValidationError_CantResolveDataType(XamlDomObject domObject, string dataTypeName)
            : base(ErrorCode.WMC0909, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CantResolveDataType, dataTypeName);
        }
    }

    internal class XamlValidationError_InvalidValueForPhase: XamlCompileError
    {
        public XamlValidationError_InvalidValueForPhase(XamlDomObject domObject)
            : base(ErrorCode.WMC0910, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_InvalidValueForPhase);
        }
    }

    internal class XamlValidationError_PhaseCanBeUsedOnlyWithBind : XamlCompileError
    {
        public XamlValidationError_PhaseCanBeUsedOnlyWithBind(XamlDomObject domObject)
            : base(ErrorCode.WMC0911, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_PhaseMustHaveAssociatedBind);
        }
    }

    internal class XamlValidationError_PhaseOnlyAllowedInDataTemplate : XamlCompileError
    {
        public XamlValidationError_PhaseOnlyAllowedInDataTemplate(XamlDomObject domObject)
            : base(ErrorCode.WMC0912, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_PhaseMustBeUsedWithinADataTemplate);
        }
    }

    internal class XamlValidationError_CannotHaveDeferLoadStrategy : XamlCompileError
    {
        public XamlValidationError_CannotHaveDeferLoadStrategy(XamlDomMember domMember)
            : base(ErrorCode.WMC0913, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CannotHaveDeferLoadStrategy);
        }
    }

    internal class XamlValidationError_LoadNotSupported : XamlCompileError
    {
        public XamlValidationError_LoadNotSupported(XamlDomMember domMember)
            : base(ErrorCode.WMC0913, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_LoadNotSupported);
        }
    }

    internal class XamlValidationError_LoadConflict : XamlCompileError
    {
        public XamlValidationError_LoadConflict(XamlDomMember domMember)
            : base(ErrorCode.WMC0914, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_LoadConflict);
        }
    }

    internal class XamlValidationCreateFromStringError : XamlCompileError
    {
        public XamlValidationCreateFromStringError(string typeName, string createFromStringMethodName, string message, XamlDomNode locationForErrors)
    : base(ErrorCode.WMC0915, locationForErrors)
        {
            Message = string.Format(message, createFromStringMethodName, typeName);
        }
    }

    internal class XamlValidationConditionalNamespaceError : XamlCompileError
    {
        public XamlValidationConditionalNamespaceError(string expressionBeingParsed, string message, XamlDomNode domNode)
            : base(ErrorCode.WMC0916, domNode)
        {
            Message = ResourceUtilities.FormatString(
                XamlCompilerResources.ConditionalNamespace_FailedToParse,
                expressionBeingParsed, message);
        }
    }

    internal class XamlValidationError_DefaultBindModeInvalidValue : XamlCompileError
    {
        public XamlValidationError_DefaultBindModeInvalidValue(XamlDomMember domMember)
            : base(ErrorCode.WMC0917, domMember)
        {
            XamlMember member = domMember.Member;
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlValidationError_DefaultBindModeInvalidValue, member.Name);
        }
    }

    internal class XamlValidationPlatformConditionalStrict : XamlCompileError
    {
        public XamlValidationPlatformConditionalStrict(XamlDomNode domNode)
            : base(ErrorCode.WMC0918, domNode)
        {
            Message = XamlCompilerResources.ConditionalNamespace_ConditionalInStandard;
        }
    }

    /// <summary>
    /// Error moved from top level driver into XBF generator.
    /// </summary>
    internal class XamlFileMustEndInDotXaml : XamlCompileError
    {
        public XamlFileMustEndInDotXaml(string fileName)
            : base(ErrorCode.WMC1010, fileName, 0, 0)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XamlFileMustEndInDotXaml);
        }
    }

    /// <summary>
    /// Error moved from top level driver into XBF generator.
    /// </summary>
    internal class XamlXBindParseError : XamlCompileError
    {
        public XamlXBindParseError(IXamlDomNode node, CompiledBindingParseException ex)
            : base(ErrorCode.WMC1110, node.SourceFilePath, node.StartLineNumber, ex.StartCharacterPosition)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.BindAssignment_XamlXBindParseError, ex.ExpressionBeingParsed, ex.Message);
        }


        public XamlXBindParseError(BindAssignmentBase bindAssignment, CompiledBindingParseException ex)
        : this(bindAssignment, ex.StartCharacterPosition, ex.ExpressionBeingParsed, ex.Message)
        {
        }

        public XamlXBindParseError(BindAssignmentBase bindAssignment, int startCharacterPosition, string expressionBeingParsed, string exceptionMessage)
            : base(ErrorCode.WMC1110, bindAssignment.ConnectionIdElement.ParentFileCodeInfo.FullPathToXamlFile,
                  bindAssignment.LineNumberInfo.StartLineNumber, startCharacterPosition)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.BindAssignment_XamlXBindParseError, expressionBeingParsed, exceptionMessage);
        }
    }

    internal class XamlXBindParseWarning : XamlCompileWarning
    {
        public XamlXBindParseWarning(XamlDomObject domObject, string message)
            : base(ErrorCode.WMC1507, domObject)
        {
            Message = message;
        }
    }

    internal class XamlXClassDerivedFromXClassWarning : XamlCompileWarning
    {
        public XamlXClassDerivedFromXClassWarning(XamlDomObject domObject, string derivedClass, string baseClass)
            : base(ErrorCode.WMC1508, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XClassDerivesFromXClass, derivedClass, baseClass);
        }
    }

    internal class XamlLocalAssemblyNotFound : XamlCompileWarning
    {
        public XamlLocalAssemblyNotFound()
            : base(ErrorCode.WMC1509)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_LocalAssemblyMissingWarning);
        }
    }

    internal class XamlXBindDataTemplateDoesNotDefineDataTypeError : XamlCompileError
    {
        public XamlXBindDataTemplateDoesNotDefineDataTypeError(IXamlDomNode node)
            : base(ErrorCode.WMC1111, node)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.Harvester_DataTemplateDoesNotDefineDataType);
        }
    }

    internal class XamlXBindControlTemplateDoesNotDefineTargetTypeError : XamlCompileError
    {
        public XamlXBindControlTemplateDoesNotDefineTargetTypeError(IXamlDomNode node)
            : base(ErrorCode.WMC1111, node)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.Harvester_ControlTemplateDoesNotDefineTargetType);
        }
    }

    internal class XamlXBindUsedInStyleError : XamlCompileError
    {
        public XamlXBindUsedInStyleError(IXamlDomNode node)
            : base(ErrorCode.WMC1112, node)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.BindAssignment_XamlXBindUsedInStyleError);
        }
    }

    internal class XamlXBindTwoWayBindingToANonDependencyPropertyError : XamlCompileError
    {
        public XamlXBindTwoWayBindingToANonDependencyPropertyError(XamlDomMember domMember)
            : base(ErrorCode.WMC1118, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_TwoWayTargetNotADependencyProperty, domMember.Member.Name);
        }
    }

    internal class XamlXBindWithoutCodeBehindError : XamlCompileError
    {
        public XamlXBindWithoutCodeBehindError(XamlDomMember domMember)
            : base(ErrorCode.WMC1119, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XBindWithoutCodeBehind);
        }
    }

    internal class XamlXBindTargetNullValueOnNonNullableTypeError : XamlCompileError
    {
        public XamlXBindTargetNullValueOnNonNullableTypeError(XamlDomMember domMember)
            : base(ErrorCode.WMC1120, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XBindTargetNullValueOnNonNullableType,
                domMember.Member.Name, domMember.Member.Type.Name);
        }
    }

    internal class BindAssignmentValidationError : XamlCompileError
    {
        public BindAssignmentValidationError(IXamlDomNode node, string message)
            : base(ErrorCode.WMC1121, node)
        {
            Message = ResourceUtilities.FormatString(
                XamlCompilerResources.BindAssignment_XamlXBindAssignmentValidationError,
                message);
        }
    }

    internal class BindAssignmentValidationWarning : XamlCompileWarning
    {
        public BindAssignmentValidationWarning(
            IXamlDomNode node, ErrorCode errorCode, string message)
            : base(errorCode, node)
        {
            Message = message;
        }
    }

    internal class XamlXBindInsideXBindError : XamlCompileError
    {
        public XamlXBindInsideXBindError(XamlDomMember domMember)
            : base(ErrorCode.WMC1122, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XBindInsideXBind, domMember.Member.Name);
        }
    }

    internal class XamlXBindOnControlTemplateError : XamlCompileError
    {
        public XamlXBindOnControlTemplateError(XamlDomMember domMember)
            : base(ErrorCode.WMC1123, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XBindOnControlTemplate);
        }
    }

    internal class XamlXBindOutOfScopeUnsupported : XamlCompileError
    {
        public XamlXBindOutOfScopeUnsupported(BindAssignment ba, string elementName, int namedElementLineNumber)
            : base(ErrorCode.WMC1124, ba.ConnectionIdElement.ParentFileCodeInfo.FullPathToXamlFile,
                  ba.LineNumberInfo.StartLineNumber, ba.LineNumberInfo.StartLinePosition)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XBindOutOfScopeUnsupported, elementName, namedElementLineNumber);
        }
    }

    internal class XamlXBindRootNoLoadingEvent : XamlCompileError
    {
        public XamlXBindRootNoLoadingEvent(XamlDomMember domMember, string rootElementType)
            : base(ErrorCode.WMC1125, domMember)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XBindRootMustHaveLoading, rootElementType);
        }
    }

    internal class XamlValidationWarningDeprecated : XamlCompileWarning
    {
        public XamlValidationWarningDeprecated(IXamlDomNode domObject, string name, string message)
            : base(ErrorCode.WMC1500, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_Deprecated, name, message);
        }
    }

    internal class XamlValidationWarningExperimental : XamlCompileWarning
    {
        public XamlValidationWarningExperimental(ErrorCode warningCode, string name)
            : base(warningCode)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_Experimental, name);
        }
        public XamlValidationWarningExperimental(ErrorCode warningCode, IXamlDomNode domNode, string name)
            : base(warningCode, domNode)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_Experimental, name);
        }
    }

    internal class XamlValidationWarningPreview : XamlCompileWarning
    {
        public XamlValidationWarningPreview(ErrorCode warningCode, string name)
            : base(warningCode)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_Preview, name);
        }
    }

    internal class XamlValidationWarningNoXaml: XamlCompileWarning
    {
        public XamlValidationWarningNoXaml()
            : base(ErrorCode.WMC1001)
        {
            Message = XamlCompilerResources.XamlCompiler_NoXamlGiven;
        }
    }

    internal class XamlValidationWarningUsingCodeGenFlags : XamlCompileWarning
    {
        public XamlValidationWarningUsingCodeGenFlags(CodeGenCtrlFlags flags)
            : base(ErrorCode.WMC1004)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CodeGenString_Using, flags.ToString());
        }
    }

    internal class XamlValidationWarningUnsupportedCodeGenFlags : XamlCompileWarning
    {
        public XamlValidationWarningUnsupportedCodeGenFlags(CodeGenCtrlFlags flags)
            : base(ErrorCode.WMC1014)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CodeGenString_NotSupported, flags.ToString());
        }
    }

    internal class XamlTypeInfoReflectionTypeNamingConventionViolation : XamlCompileWarning
    {
        public XamlTypeInfoReflectionTypeNamingConventionViolation(string typeName, string asmName)
            : base(ErrorCode.WMC1005)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.TypeInfoReflection_TypeViolatesNamingConvention, typeName, asmName);
        }
    }

    internal class XamlErrorDuplicateType : XamlCompileError
    {
        public XamlErrorDuplicateType(string fullName)
            : base(ErrorCode.WMC0901)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_DuplicateTypeName, fullName);
        }
    }

    internal class ErrorXPropertyUsageNotSupported : XamlCompileError
    {
        public ErrorXPropertyUsageNotSupported(XamlDomObject domObject, Language language)
            : base(ErrorCode.WMC0505, domObject)
        {
            Message = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XPropertyUsageNotSupportedForLanguage, language.Name);
        }
    }
}
