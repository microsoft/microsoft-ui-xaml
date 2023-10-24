// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ObjectWriterNode.h"
#include "TypeTable.g.h"
#include "MetadataAPI.h"
#include "XamlPredicateHelpers.h"

#include <SubObjectWriterResult.h>
#include <CustomWriterRuntimeData.h>
#include <XStringUtils.h>

using namespace DirectUI;

ObjectWriterNode::~ObjectWriterNode() WI_NOEXCEPT
{
}

ObjectWriterNode::ObjectWriterNode() WI_NOEXCEPT
    : m_NodeType(ObjectWriterNodeType::None)
    , m_intValue(0)
{
}

//
// [AddNamespace],[NamespacePrefix],[Namespace]
//
// eg.
//   AddNamespace (prefix=x, namespace=http://schemas.microsoft.com/winfx/2006/xaml)
//
ObjectWriterNode ObjectWriterNode::MakeAddNamespaceNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const xstring_ptr& strNamespacePrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spNamespace) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::AddNamespace, lineInfo, strNamespacePrefix, spNamespace);
}

//
// [PushScopeAddNamespace],[NamespacePrefix],[Namespace]
//
// eg.
//   PushScopeAddNamespace (prefix=x, namespace=http://schemas.microsoft.com/winfx/2006/xaml)
//
ObjectWriterNode ObjectWriterNode::MakePushScopeAddNamespaceNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const xstring_ptr& strNamespacePrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spNamespace) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScopeAddNamespace, lineInfo, strNamespacePrefix, spNamespace);
}

//
// [CreateType],[XamlType]
//
// eg.
//   CreateType (type=Microsoft.UI.Xaml.Style)
//
ObjectWriterNode ObjectWriterNode::MakeCreateTypeNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::CreateType, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType));
}

//
// [CreateTypeBeginInit],[XamlType]
//
// eg.
//   CreateTypeBeginInit (type=Microsoft.UI.Xaml.Style)
//
ObjectWriterNode ObjectWriterNode::MakeCreateTypeBeginInitNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::CreateTypeBeginInit, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType));
}

//
// [PushScopeCreateType],[XamlType]
//
// eg.
//   PushScopeCreateType (type=Microsoft.UI.Xaml.Style)
//
ObjectWriterNode ObjectWriterNode::MakePushScopeCreateTypeNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScopeCreateType, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType));
}

//
// [PushScopeCreateTypeBeginInit],[XamlType]
//
// eg.
//   PushScopeCreateTypeBeginInit (type=Microsoft.UI.Xaml.Style)
//
ObjectWriterNode ObjectWriterNode::MakePushScopeCreateTypeBeginInitNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScopeCreateTypeBeginInit, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType));
}

//
// [CreateTypeWithInitialValue],[XamlType]
//
// eg.
//   PushConstant(type = valueString, value = Red)
//   TypeConvert(type convertor = Microsoft.UI.Xaml.Media.SolidColorBrush)
//   CreateTypeWithInitialValue(type = Microsoft.UI.Xaml.Media.SolidColorBrush)
//
ObjectWriterNode ObjectWriterNode::MakeCreateTypeWithInitialValueNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::CreateTypeWithInitialValue, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType));
}

//
// [TypeConvertValue],[TypeConverter]
//
// eg.
//   PushConstant(type = valueString, value = Red)
//   TypeConvert(type convertor = Microsoft.UI.Xaml.Media.SolidColorBrush)
//
ObjectWriterNode ObjectWriterNode::MakeTypeConvertValueNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntaxConverter) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::TypeConvertValue, lineInfo, std::shared_ptr<XamlTextSyntax>(spTextSyntaxConverter));
}

//
// [PushConstant],[BoxedConstant]
//
// eg.
//   PushConstant (type=valueString, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakePushConstantNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushConstant, lineInfo, spValue);
}

//
// [PushResolvedType],[XamlType]
//
// eg.
//   PushResolvedType (TypeProxy)
//
ObjectWriterNode ObjectWriterNode::MakePushResolvedTypeNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushResolvedType, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType));
}

//
// [PushResolvedProperty],[XamlProperty]
//
// eg.
//   PushResolvedProperty (PropertyProxy)
//
ObjectWriterNode ObjectWriterNode::MakePushResolvedPropertyNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushResolvedProperty, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty));
}

//
// [AddToCollection]
//
// eg.
//   CreateType(type = Microsoft.UI.Xaml.Media.Animation.TransitionCollection)
//   BeginInit
//     CreateType(type = Microsoft.UI.Xaml.Media.Animation.PaneThemeTransition)
//     BeginInit
//     EndInit
//     AddToCollection
//
ObjectWriterNode ObjectWriterNode::MakeAddToCollectionNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::AddToCollection, lineInfo);
}

//
// [AddToDictionary]
//
// eg.
//   GetValue (property=Microsoft.UI.Xaml.FrameworkElement.Resources)
//     CreateTypeWithInitialValue (type=Microsoft.UI.Xaml.Media.SolidColorBrush)
//     BeginInit
//     EndInit
//     AddToDictionary
//
ObjectWriterNode ObjectWriterNode::MakeAddToDictionaryNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::AddToDictionary, lineInfo);
}

//
// [AddToDictionaryWithKey],[Key]
//
// eg.
//   GetValue (property=Microsoft.UI.Xaml.FrameworkElement.Resources)
//     CreateTypeWithInitialValue (type=Microsoft.UI.Xaml.Media.SolidColorBrush)
//     BeginInit
//     EndInit
//     AddToDictionary (Key=AppPersonalityColorBrush, KeyIsType? No)
//
ObjectWriterNode ObjectWriterNode::MakeAddToDictionaryWithKeyNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKeyValue) WI_NOEXCEPT
{
    // spKeyValue can be null if it is an implicit key
    return ObjectWriterNode(ObjectWriterNodeType::AddToDictionaryWithKey, lineInfo, std::shared_ptr<XamlQualifiedObject>(spKeyValue));
}

//
// [SetConnectiondId],[Id]
//
// eg.
//   CreateType (type=Microsoft.UI.Xaml.Controls.Button)
//   BeginInit
//     SetConnectionId (Id=1)
//
ObjectWriterNode ObjectWriterNode::MakeSetConnectionIdNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKeyValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetConnectionId, lineInfo, std::shared_ptr<XamlQualifiedObject>(spKeyValue));
}

//
// [SetName],[Name]
//
// eg.
//   CreateType (type=Microsoft.UI.Xaml.Shapes.Rectangle)
//   BeginInit
//     SetValue(property = Microsoft.UI.Xaml.FrameworkElement.Name)
//     SetName(x : Name = FocusVisualBlack)
//
ObjectWriterNode ObjectWriterNode::MakeSetNameNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spNameValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetName, lineInfo, std::shared_ptr<XamlQualifiedObject>(spNameValue));
}

//
// [ProvideValue]
//
// eg.
//   CreateType(type = Microsoft.UI.Xaml.Controls.Grid)
//   BeginInit
//     CreateType(type = Microsoft.UI.Xaml.StaticResource)
//     BeginInit
//       PushConstant(type = valueString, value = AppPersonalityColorBrush)
//       TypeConvert(type convertor = Windows.Foundation.String)
//       SetValue(property = Microsoft.UI.Xaml.StaticResource.ResourceKey)
//     EndInit
//     ProvideValue
//
ObjectWriterNode ObjectWriterNode::MakeProvideValueNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::ProvideValue, lineInfo);
}

//
// [ProvideStaticResourceValue]
//
// eg.
//   Aggregates the following set of commands:
//
//   CreateType(type = Microsoft.UI.Xaml.Controls.Grid)
//   BeginInit
//     CreateType(type = Microsoft.UI.Xaml.StaticResource)
//     BeginInit
//       PushConstant(type = valueString, value = AppPersonalityColorBrush)
//       TypeConvert(type convertor = Windows.Foundation.String)
//       SetValue(property = Microsoft.UI.Xaml.StaticResource.ResourceKey)
//     EndInit
//     ProvideValue
//
ObjectWriterNode ObjectWriterNode::MakeProvideStaticResourceValueNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::ProvideStaticResourceValue, lineInfo, std::shared_ptr<XamlQualifiedObject>(spValue));
}

//
// [ProvideThemeResourceValue]
//
// eg.
//   Aggregates the following set of commands:
//
//   CreateType(type = Microsoft.UI.Xaml.Controls.Grid)
//   BeginInit
//     CreateType(type = Microsoft.UI.Xaml.ThemeResource)
//     BeginInit
//       PushConstant(type = valueString, value = AppPersonalityColorBrush)
//       TypeConvert(type convertor = Windows.Foundation.String)
//       SetValue(property = Microsoft.UI.Xaml.ThemeResource.ResourceKey)
//     EndInit
//     ProvideValue
//
ObjectWriterNode ObjectWriterNode::MakeProvideThemeResourceValueNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::ProvideThemeResourceValue, lineInfo, std::shared_ptr<XamlQualifiedObject>(spValue));
}

//
// [ProvideTemplateBindingValue]
//
// eg.
//   Aggregates the following set of commands:
//
//   CreateType(type = Microsoft.UI.Xaml.Controls.Grid)
//   BeginInit
//     CreateType(type = Microsoft.UI.Xaml.TemplateBinding)
//     BeginInit
//       PushConstant(type = valueString, value = Width)
//       TypeConvert(type convertor = Microsoft.UI.Xaml.DependencyProperty)
//       SetValue(property = Microsoft.UI.Xaml.TemplateBinding.Property)
//     EndInit
//     ProvideValue
//
ObjectWriterNode ObjectWriterNode::MakeProvideTemplateBindingValueNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spPropertyProxy) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::ProvideTemplateBindingValue, lineInfo, std::shared_ptr<IXamlSchemaObject>(spPropertyProxy));
}

//
// [SetDirectiveProperty],[DirectiveProperty]
//
// eg.
//   PushConstant (type=valueString, value=AboutBorder)
//   TypeConvert (type convertor=Windows.Foundation.String)
//   SetDirectiveProperty Id=Name
//
ObjectWriterNode ObjectWriterNode::MakeSetDirectivePropertyNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<DirectiveProperty>& spProperty) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetDirectiveProperty, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty));
}

//
// [GetResourcePropertyBag],[Id]
//
// eg.
//   CreateType (type=Microsoft.UI.Xaml.Controls.Button)
//   BeginInit
//     GetResourcePropertyBag (Id=AboutControlBackButton)
//
ObjectWriterNode ObjectWriterNode::MakeGetResourcePropertyBagNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKeyValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::GetResourcePropertyBag, lineInfo, std::shared_ptr<XamlQualifiedObject>(spKeyValue));
}

//
// [BeginInit]
//
// eg.
//   CreateType (type=Microsoft.UI.Xaml.Controls.Button)
//   BeginInit
//
ObjectWriterNode ObjectWriterNode::MakeBeginInitNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::BeginInit, lineInfo);
}

//
// [EndInit]
//
// eg.
//     CreateType(type = Microsoft.UI.Xaml.Media.Animation.PaneThemeTransition)
//     BeginInit
//     EndInit
//
ObjectWriterNode ObjectWriterNode::MakeEndInitNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::EndInit, lineInfo);
}

//
// [EndInitPopScope]
//
// eg.
//     EndInitPopScope
//
ObjectWriterNode ObjectWriterNode::MakeEndInitPopScopeNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::EndInitPopScope, lineInfo);
}

//
// [EndInitProvideValuePopScope]
//
// eg.
//     Aggregates the following node sequence:
//
//         EndInit
//         ProvideValue
//         AddToDictionaryWithKey  -- gets moved after the resulting aggregate node
//     PopScope
//
ObjectWriterNode ObjectWriterNode::MakeEndInitProvideValuePopScopeNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::EndInitProvideValuePopScope, lineInfo);
}

//
// [PushScope]
//
// eg.
//   GetValue (property=Microsoft.UI.Xaml.Controls.Panel.Children)
//   PushScope
//
ObjectWriterNode ObjectWriterNode::MakePushScopeNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScope, lineInfo);
}

//
// [PopScope]
//
// eg.
//   PushScope
//     PushConstant (type=valueString, value=AboutControlBackButton)
//     TypeConvert (type convertor=Windows.Foundation.String)
//     SetValue (property=Microsoft.UI.Xaml.FrameworkElement.Name)
//   PopScope
//
ObjectWriterNode ObjectWriterNode::MakePopScopeNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PopScope, lineInfo);
}

//
// [GetValue],[Property]
//
// eg.
//   GetValue (property=Microsoft.UI.Xaml.Controls.Panel.Children)
//
ObjectWriterNode ObjectWriterNode::MakeGetValueNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::GetValue, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty));
}

//
// [PushScopeGetValue],[Property]
//
// eg.
//   PushScopeGetValue (property=Microsoft.UI.Xaml.Controls.Panel.Children)
//
ObjectWriterNode ObjectWriterNode::MakePushScopeGetValueNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScopeGetValue, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty));
}

//
// [SetValue],[Property]
//
// eg.
//   PushConstant (type=valueString, value=AboutControlBackButton)
//   TypeConvert (type convertor=Windows.Foundation.String)
//   SetValue (property=Microsoft.UI.Xaml.FrameworkElement.Name)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValue, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty));
}

//
// [SetValueFromStaticResource],[Property],[StaticResourceKey]
//
// eg.
//   SetValueFromStaticResource (property=Microsoft.UI.Xaml.FrameworkElement.Name, Key=MyName)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueFromStaticResourceNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValueFromStaticResource, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty), spValue);
}

//
// [SetValueFromThemeResource],[Property],[ThemeResourceKey]
//
// eg.
//   SetValueFromThemeResource (property=Microsoft.UI.Xaml.FrameworkElement.Background, Key=MyBackground)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueFromThemeResourceNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValueFromThemeResource, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty), spValue);
}

//
// [SetValueFromTemplateBinding],[Property],[TemplateBindingPoperty]
//
// eg.
//   SetValueFromTemplateBinding (property=Microsoft.UI.Xaml.FrameworkElement.Width, TemplateBindingPoperty=BorderWidth)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueFromTemplateBindingNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlProperty>& spPropertyProxy) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValueFromTemplateBinding, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty), std::shared_ptr<IXamlSchemaObject>(spPropertyProxy));
}

//
// [SetValueFromMarkupExtension],[Property]
//
// eg.
//   SetValueFromMarkupExtension (property=Microsoft.UI.Xaml.FrameworkElement.Width)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueFromMarkupExtensionNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValueFromMarkupExtension, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty));
}

//
// [SetValueTypeConvertedConstant],[Property],[TypeConverter],[Constant]
//
// eg.
//   SetValueTypeConvertedConstant (property=Microsoft.UI.Xaml.FrameworkElement.Name, type convertor=Windows.Foundation.String, type=valueString, value=AboutControlBackButton)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueTypeConvertedConstantNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValueTypeConvertedConstant, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty), spValue, spTextSyntax);
}

//
// [SetValueTypeConvertedResolvedType],[Property],[TypeConverter],[ResolvedType]
//
// eg.
//   SetValueTypeConvertedResolvedType (property = Microsoft.UI.Xaml.Style.TargetType, type converter = Microsoft.UI.Xaml.Interop.TypeName, value = Microsoft.UI.Xaml.Controls.RadioButton)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueTypeConvertedResolvedTypeNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
    _In_ const std::shared_ptr<XamlType>& spTypeProxy) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValueTypeConvertedResolvedType, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty), std::shared_ptr<IXamlSchemaObject>(spTypeProxy), spTextSyntax);
}

//
// [SetValueTypeConvertedResolvedProperty],[Property],[TypeConverter],[ResolvedProperty]
//
// eg.
//   SetValueTypeConvertedResolvedProperty (property=Microsoft.UI.Xaml.Setter.Property, type converter=Microsoft.UI.Xaml.DependencyProperty, value=FrameworkElement.Template)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueTypeConvertedResolvedPropertyNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
    _In_ const std::shared_ptr<XamlProperty>& spPropertyProxy) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValueTypeConvertedResolvedProperty, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty), std::shared_ptr<IXamlSchemaObject>(spPropertyProxy), spTextSyntax);
}

//
// [SetValueConstant],[Property],[Constant]
//
// eg.
//   SetValueConstant (property=Microsoft.UI.Xaml.FrameworkElement.Name, value=AboutControlBackButton)
//
ObjectWriterNode ObjectWriterNode::MakeSetValueConstantNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetValueConstant, lineInfo, std::shared_ptr<IXamlSchemaObject>(spProperty), spValue);
}

//
// [PushScopeCreateTypeWithConstant],[Type],[Constant]
//
// eg.
//   PushScopeCreateTypeWithConstant (type=Microsoft.UI.Xaml.SolidColorBrush, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakePushScopeCreateTypeWithConstantNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScopeCreateTypeWithConstant, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType), spValue);
}

//
// [PushScopeCreateTypeWithConstantBeginInit],[Type],[Constant]
//
// eg.
//   PushScopeCreateTypeWithConstantBeginInit (type=Microsoft.UI.Xaml.SolidColorBrush, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakePushScopeCreateTypeWithConstantBeginInitNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScopeCreateTypeWithConstantBeginInit, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType), spValue);
}

//
// [CreateTypeWithConstant],[Type],[Constant]
//
// eg.
//   CreateTypeWithConstant (type=Microsoft.UI.Xaml.SolidColorBrush, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakeCreateTypeWithConstantNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::CreateTypeWithConstant, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType), spValue);
}

//
// [CreateTypeWithConstantBeginInit],[Type],[Constant]
//
// eg.
//   CreateTypeWithConstantBeginInit (type=Microsoft.UI.Xaml.SolidColorBrush, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakeCreateTypeWithConstantBeginInitNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::CreateTypeWithConstantBeginInit, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType), spValue);
}

//
// [PushScopeCreateTypeWithTypeConvertedConstantBeginInit],[Type],[Constant]
//
// eg.
//   PushScopeCreateTypeWithTypeConvertedConstantBeginInit (type=Microsoft.UI.Xaml.SolidColorBrush, typeConverter=Color, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakePushScopeCreateTypeWithTypeConvertedConstantBeginInitNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstantBeginInit, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType), spValue, spTextSyntax);
}

//
// [PushScopeCreateTypeWithTypeConvertedConstant],[Type],[Constant]
//
// eg.
//   PushScopeCreateTypeWithTypeConvertedConstant (type=Microsoft.UI.Xaml.SolidColorBrush, typeConverter=Color, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakePushScopeCreateTypeWithTypeConvertedConstantNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstant, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType), spValue, spTextSyntax);
}

//
// [CreateTypeWithTypeConvertedConstantBeginInit],[Type],[Constant]
//
// eg.
//   CreateTypeWithTypeConvertedConstantBeginInit (type=Microsoft.UI.Xaml.SolidColorBrush, typeConverter=Color, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakeCreateTypeWithTypeConvertedConstantBeginInitNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::CreateTypeWithTypeConvertedConstantBeginInit, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType), spValue, spTextSyntax);
}

//
// [CreateTypeWithTypeConvertedConstant],[Type],[Constant]
//
// eg.
//   CreateTypeWithTypeConvertedConstant (type=Microsoft.UI.Xaml.SolidColorBrush, typeConverter=Color, value=Red)
//
ObjectWriterNode ObjectWriterNode::MakeCreateTypeWithTypeConvertedConstantNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::CreateTypeWithTypeConvertedConstant, lineInfo, std::shared_ptr<IXamlSchemaObject>(spType), spValue, spTextSyntax);
}

//
// [CheckPeerType],[ClassName]
//
// eg.
//   CheckPeerType (x:Class=SimpleTest.MainPage)
//
ObjectWriterNode ObjectWriterNode::MakeCheckPeerTypeNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const xstring_ptr& strClassName) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::CheckPeerType, lineInfo, strClassName);
}

//
// TODO
//
ObjectWriterNode ObjectWriterNode::MakeSetCustomRuntimeData(
    _In_ const XamlLineInfo& lineInfo,
    _In_ std::shared_ptr<CustomWriterRuntimeData> customWriterData,
    _In_ std::shared_ptr<SubObjectWriterResult> customWriterNodeStream) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetCustomRuntimeData, lineInfo, customWriterData, customWriterNodeStream);
}

//
// TODO
//
ObjectWriterNode ObjectWriterNode::MakeSetResourceDictionaryItems(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetResourceDictionaryItems, lineInfo);
}

//
// [SetDeferredProperty],[DeferredProperty],[NodeList],[ResourceList]
//
// eg.
//   SetDeferredProperty (property=__DeferredStoryboard, nodelist=[ObjectWriterNodeList], ResourceList=[reference resources])
//
ObjectWriterNode ObjectWriterNode::MakeSetDeferredPropertyNode(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<ObjectWriterNodeList>& spNodeList,
    _In_ std::vector<std::pair<bool, xstring_ptr>>&& vecResourceList) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetDeferredProperty, lineInfo, spProperty, spNodeList, std::move(vecResourceList));
}

// This is a runtime only node that is capable of replacing a nodeList with an in-memory
// representation of the runtime processed deferred property in order to speed up
// template stamping.
//
// [SetDeferredPropertyWithValue],[DeferredProperty],[Instance]
//
// eg.
//   SetDeferredPropertyWithValue (property=__DeferredStoryboard, instance=TemplateContent)
//
ObjectWriterNode ObjectWriterNode::MakeSetDeferredPropertyNodeWithValue(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetDeferredProperty, lineInfo, spProperty, spInstance);
}

// This is the disk version.
//
// [SetDeferredPropertyWithReader],[DeferredProperty],[Reader],[ResourceList]
//
// eg.
//   SetDeferredPropertyWithValue (property=__DeferredStoryboard, reader=..., ResourceList=[reference resources])
//
ObjectWriterNode ObjectWriterNode::MakeSetDeferredPropertyNodeWithReader(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlBinaryFormatSubReader2>& spReader,
    _In_ std::vector<std::pair<bool, xstring_ptr>>&& vecResourceList) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::SetDeferredProperty, lineInfo, spProperty, spReader, std::move(vecResourceList));
}

//
// [EndOfStream]
//
ObjectWriterNode ObjectWriterNode::MakeEndOfStreamNode(
_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::EndOfStream, lineInfo);
}

ObjectWriterNode ObjectWriterNode::MakeStreamOffsetMarker(
_In_ const UINT32 tokenIndex) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::StreamOffsetMarker, tokenIndex);
}

ObjectWriterNode ObjectWriterNode::MakeBeginConditionalScope(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::BeginConditionalScope, lineInfo, std::shared_ptr<IXamlSchemaObject>(xamlPredicateAndArgs));
}

ObjectWriterNode ObjectWriterNode::MakeEndConditionalScope(_In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
{
    return ObjectWriterNode(ObjectWriterNodeType::EndConditionalScope, lineInfo);
}

const ObjectWriterNodeType ObjectWriterNode::GetNodeType() const WI_NOEXCEPT
{
    return m_NodeType;
}

const std::shared_ptr<XamlType> ObjectWriterNode::GetXamlType() const WI_NOEXCEPT
{
    return std::static_pointer_cast<XamlType>(m_spShared);
}

const std::shared_ptr<XamlType> ObjectWriterNode::GetXamlTypeProxy() const WI_NOEXCEPT
{
    return std::static_pointer_cast<XamlType>(m_spShared2);
}

const std::shared_ptr<XamlProperty> ObjectWriterNode::GetXamlProperty() const WI_NOEXCEPT
{
    return std::static_pointer_cast<XamlProperty>(m_spShared);
}

const std::shared_ptr<XamlProperty> ObjectWriterNode::GetXamlPropertyProxy() const WI_NOEXCEPT
{
    return std::static_pointer_cast<XamlProperty>(m_spShared2);
}

const std::shared_ptr<XamlNamespace> ObjectWriterNode::GetNamespace() const WI_NOEXCEPT
{
    return std::static_pointer_cast<XamlNamespace>(m_spShared);
}

const std::shared_ptr<XamlQualifiedObject>& ObjectWriterNode::GetValue() const WI_NOEXCEPT
{
    return m_spValue;
}

const xstring_ptr& ObjectWriterNode::GetStringValue() const WI_NOEXCEPT
{
    return m_strValue;
}

const std::shared_ptr<XamlTextSyntax>& ObjectWriterNode::GetTypeConverter() const WI_NOEXCEPT
{
    return m_spTextSyntaxConverter;
}

const XamlLineInfo& ObjectWriterNode::GetLineInfo() const WI_NOEXCEPT
{
    return m_LineInfo;
}

const std::shared_ptr<Parser::XamlPredicateAndArgs> ObjectWriterNode::GetXamlPredicateAndArgs() const WI_NOEXCEPT
{
    return std::static_pointer_cast<Parser::XamlPredicateAndArgs>(m_spShared);
}

const std::shared_ptr<ObjectWriterNodeList>& ObjectWriterNode::GetNodeList() const WI_NOEXCEPT
{
    if (m_NodeType == ObjectWriterNodeType::SetCustomRuntimeData)
    {
        return m_CustomWriterNodeStream->GetNodeList();
    }
    else
    {
        return m_spNodeList;
    }

}

const std::shared_ptr<XamlBinaryFormatSubReader2>& ObjectWriterNode::GetSubReader() const WI_NOEXCEPT
{
    return m_spSubReader;
}

const std::vector<std::pair<bool, xstring_ptr>>& ObjectWriterNode::GetListOfReferences() const WI_NOEXCEPT
{
    return m_vecResourceList;
}

const std::shared_ptr<CustomWriterRuntimeData>& ObjectWriterNode::GetCustomWriterData() const WI_NOEXCEPT
{
    return m_CustomWriterData;
}

const std::shared_ptr<SubObjectWriterResult>& ObjectWriterNode::GetCustomWriterNodeStream() const WI_NOEXCEPT
{
    return m_CustomWriterNodeStream;
}

const bool ObjectWriterNode::RequiresNewScope() const WI_NOEXCEPT
{
    if (m_NodeType == ObjectWriterNodeType::PushScope ||
        m_NodeType == ObjectWriterNodeType::PushScopeAddNamespace ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateType ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeBeginInit ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeWithConstant ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeWithConstantBeginInit ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstant ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstantBeginInit ||
        m_NodeType == ObjectWriterNodeType::PushScopeGetValue)
    {
        return true;
    }

    return false;
}

const bool ObjectWriterNode::RequiresScopeToEnd() const WI_NOEXCEPT
{
    if (m_NodeType == ObjectWriterNodeType::PopScope ||
        m_NodeType == ObjectWriterNodeType::EndInitPopScope ||
        m_NodeType == ObjectWriterNodeType::EndInitProvideValuePopScope)
    {
        return true;
    }

    return false;
}

const bool ObjectWriterNode::ProvidesCustomBinaryData() const WI_NOEXCEPT
{
    if (m_NodeType == ObjectWriterNodeType::SetDeferredProperty ||
        m_NodeType == ObjectWriterNodeType::SetCustomRuntimeData)
    {
        return true;
    }

    return false;
}

// Debuggable Information for the Node
HRESULT ObjectWriterNode::GetLineInfoAsString(_Out_ xstring_ptr& strValue) const
{
    strValue = StringCchPrintfWWrapper(
        L"Line=%d, Column=%d",
        m_LineInfo.LineNumber(),
        m_LineInfo.LinePosition());

    return S_OK;
}

// Debuggable Information for the Node
HRESULT ObjectWriterNode::ToString(_Out_ xstring_ptr& strValue) const
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strPushScope, L"PushScope");

    xstring_ptr strPushScopePrefix = xstring_ptr::EmptyString();

    if (m_NodeType == ObjectWriterNodeType::PushScopeAddNamespace ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateType ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeBeginInit ||
        m_NodeType == ObjectWriterNodeType::PushScopeGetValue ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeWithConstant ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeWithConstantBeginInit ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstant ||
        m_NodeType == ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstantBeginInit)
    {
        strPushScopePrefix = c_strPushScope;
    }


    switch (m_NodeType)
    {
        case ObjectWriterNodeType::PushScopeAddNamespace:
        case ObjectWriterNodeType::AddNamespace:
        {
            auto& strPrefix = m_strValue;
            auto spNamespace = std::static_pointer_cast<XamlNamespace>(m_spShared);
            xstring_ptr spNamespaceUri = spNamespace->get_TargetNamespace();

            strValue = StringCchPrintfWWrapper(
                L"%sAddNamespace (prefix=%s, namespace=%s)",
                strPushScopePrefix.GetBuffer(),
                strPrefix.GetBuffer(),
                spNamespaceUri.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::AddToCollection:
        {
            strValue = StringCchPrintfWWrapper(
                L"AddToCollection");
        }
        break;

        case ObjectWriterNodeType::AddToDictionary:
        case ObjectWriterNodeType::AddToDictionaryWithKey:
        {
            auto& spKeyValue = m_spValue;
            bool bKeyIsType = true;
            xstring_ptr ssKey = xstring_ptr::EmptyString();

            if (spKeyValue)
            {
                // Currently we only support strings as keys to native ResourceDictionary.
                IFC_RETURN(spKeyValue->GetCopyAsString(&ssKey));
                bKeyIsType = FALSE;

                if (ssKey.IsNull())
                {
                    if (spKeyValue->GetValue().GetType() == valueObject)
                    {
                        // TODO: Doesn't link well with GenXbf.dll
                        // Temporarily disabling.
                        //CType *pType = do_pointer_cast<CType>(spKeyValue->GetDependencyObject());
                        //if (pType)
                        //{
                        //    IFC(pType->GetClassName(&ssKey));
                        //    bKeyIsType = TRUE;
                        //}
                    }
                }
            }

            strValue = StringCchPrintfWWrapper(
                L"AddToDictionary (Key=%s, KeyIsType? %s)",
                ssKey.GetBuffer(),
                (bKeyIsType == TRUE ? L"Yes" : L"No"));
        }
        break;

        case ObjectWriterNodeType::BeginInit:
        {
            strValue = StringCchPrintfWWrapper(
                L"BeginInit");
        }
        break;

        case ObjectWriterNodeType::CheckPeerType:
        {
            auto& strClassName = m_strValue;
            strValue = StringCchPrintfWWrapper(
                L"CheckPeerType (x:Class=%s)",
                strClassName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::PushScopeCreateType:
        case ObjectWriterNodeType::CreateType:
        {
            auto spType = std::static_pointer_cast<XamlType>(m_spShared);
            xstring_ptr spTypeName;
            IFC_RETURN(spType->get_FullName(&spTypeName));
            strValue = StringCchPrintfWWrapper(
                L"%sCreateType (type=%s)",
                strPushScopePrefix.GetBuffer(),
                spTypeName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::CreateTypeBeginInit:
        {
            auto spType = std::static_pointer_cast<XamlType>(m_spShared);
            xstring_ptr spTypeName;
            IFC_RETURN(spType->get_FullName(&spTypeName));
            strValue = StringCchPrintfWWrapper(
                L"CreateTypeBeginInit (type=%s)",
                spTypeName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::PushResolvedType:
        {
            auto spType = std::static_pointer_cast<XamlType>(m_spShared);
            xstring_ptr spTypeName;
            IFC_RETURN(spType->get_FullName(&spTypeName));
            strValue = StringCchPrintfWWrapper(
                L"PushResolvedType (type=%s)",
                spTypeName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::PushScopeCreateTypeBeginInit:
        {
            auto spType = std::static_pointer_cast<XamlType>(m_spShared);
            xstring_ptr spTypeName;
            IFC_RETURN(spType->get_FullName(&spTypeName));
            strValue = StringCchPrintfWWrapper(
                L"%sCreateTypeBeginInit (type=%s)",
                strPushScopePrefix.GetBuffer(),
                spTypeName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::CreateTypeWithInitialValue:
        {
            auto spType = std::static_pointer_cast<XamlType>(m_spShared);
            xstring_ptr spTypeName;
            IFC_RETURN(spType->get_FullName(&spTypeName));
            strValue = StringCchPrintfWWrapper(
                L"CreateTypeWithInitialValue (type=%s)",
                spTypeName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::PushScopeCreateTypeWithConstantBeginInit:
        case ObjectWriterNodeType::CreateTypeWithConstantBeginInit:
        {
            std::shared_ptr<XamlQualifiedObject> spValue = m_spValue;
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());
            auto spType = std::static_pointer_cast<XamlType>(m_spShared);
            xstring_ptr spTypeName;
            IFC_RETURN(spType->get_FullName(&spTypeName));

            strValue = StringCchPrintfWWrapper(
                L"%sCreateTypeWithConstantBeginInit (type=%s, %s)",
                strPushScopePrefix.GetBuffer(),
                spTypeName.GetBuffer(),
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::PushScopeCreateTypeWithTypeConvertedConstantBeginInit:
        case ObjectWriterNodeType::CreateTypeWithTypeConvertedConstantBeginInit:
        {
            xstring_ptr spTypeConvertorTypeName = xstring_ptr::EmptyString();
            auto& spTypeConverter = m_spTextSyntaxConverter;
            std::shared_ptr<XamlQualifiedObject> spValue = m_spValue;
            auto spType = std::static_pointer_cast<XamlType>(m_spShared);
            xstring_ptr spTypeName;
            IFC_RETURN(spType->get_FullName(&spTypeName));
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());

            const CClassInfo* pClassInfo = MetadataAPI::GetClassInfoByIndex(spTypeConverter->get_TextSyntaxToken().GetHandle());
            spTypeConvertorTypeName = pClassInfo->GetFullName();

            strValue = StringCchPrintfWWrapper(
                L"%sCreateTypeWithTypeConvertedConstantBeginInit (type=%s, type converter=%s, %s)",
                strPushScopePrefix.GetBuffer(),
                spTypeName.GetBuffer(),
                spTypeConvertorTypeName.GetBuffer(),
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::EndInit:
        {
            strValue = StringCchPrintfWWrapper(
                L"EndInit");
        }
        break;

        case ObjectWriterNodeType::EndInitPopScope:
        {
            strValue = StringCchPrintfWWrapper(
                L"EndInitPopScope");
        }
        break;

        case ObjectWriterNodeType::EndInitProvideValuePopScope:
        {
            strValue = StringCchPrintfWWrapper(
                L"EndInitProvideValuePopScope");
        }
        break;

        case ObjectWriterNodeType::GetResourcePropertyBag:
        {
            auto& spUidValue = m_spValue;
            xstring_ptr ssUid;
            IFC_RETURN(spUidValue->GetCopyAsString(&ssUid));

            strValue = StringCchPrintfWWrapper(
                L"GetResourcePropertyBag (x:Uid=%s)",
                ssUid.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::PushScopeGetValue:
        case ObjectWriterNodeType::GetValue:
        {
            auto spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            xstring_ptr spPropertyName;
            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            strValue = StringCchPrintfWWrapper(
                L"%sGetValue (property=%s)",
                strPushScopePrefix.GetBuffer(),
                spPropertyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::PushResolvedProperty:
        {
            auto spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            xstring_ptr spPropertyName;
            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            strValue = StringCchPrintfWWrapper(
                L"PushResolvedProperty (property=%s)",
                spPropertyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::None:
        {
            strValue = StringCchPrintfWWrapper(
                L"Noop");
        }
        break;

        case ObjectWriterNodeType::PopScope:
        {
            strValue = StringCchPrintfWWrapper(
                L"PopScope");
        }
        break;

        case ObjectWriterNodeType::ProvideStaticResourceValue:
        {
            auto& spValue = m_spValue;
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());

            strValue = StringCchPrintfWWrapper(
                L"ProvideStaticResourceValue (key=%s)",
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::ProvideThemeResourceValue:
        {
            auto& spValue = m_spValue;
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());

            strValue = StringCchPrintfWWrapper(
                L"ProvideThemeResourceValue (key=%s)",
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValueFromStaticResource:
        {
            auto& spValue = m_spValue;
            auto spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            xstring_ptr strPropertyName;
            IFC_RETURN(spProperty->get_FullName(&strPropertyName));
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());

            strValue = StringCchPrintfWWrapper(
                L"SetValueFromStaticResource (property=%s, StaticResourceKey=%s)",
                strPropertyName.GetBuffer(),
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValueFromThemeResource:
        {
            auto& spValue = m_spValue;
            auto spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            xstring_ptr strPropertyName;
            IFC_RETURN(spProperty->get_FullName(&strPropertyName));
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());
            strValue = StringCchPrintfWWrapper(
                L"SetValueFromThemeResource (property=%s, ThemeResourceKey=%s)",
                strPropertyName.GetBuffer(),
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::ProvideValue:
        {
            strValue = StringCchPrintfWWrapper(
                L"ProvideValue");
        }
        break;

        case ObjectWriterNodeType::PushConstant:
        {
            auto& spValue = m_spValue;
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());

            strValue = StringCchPrintfWWrapper(
                L"PushConstant (%s)",
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::PushScope:
        {
            strValue = StringCchPrintfWWrapper(
                L"PushScope");
        }
        break;

        case ObjectWriterNodeType::SetCustomRuntimeData:
        {
            xstring_ptr strRuntimeData;

            IFC_RETURN(m_CustomWriterData->ToString(false, strRuntimeData));
            strValue = StringCchPrintfWWrapper(
                L"SetCustomRuntimeData (dataFormat=%s)",
                strRuntimeData.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetConnectionId:
        {
            xstring_ptr strCValueString = ToStringCValueHelper(m_spValue->GetValue());

            strValue = StringCchPrintfWWrapper(
                L"SetConnectionId (Id=%s)",
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetDirectiveProperty:
        {
            auto spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            xstring_ptr ssPropertyName;
            if (spProperty)
            {
                IFC_RETURN(spProperty->get_FullName(&ssPropertyName));
            }

            strValue = StringCchPrintfWWrapper(
                L"SetDirectiveProperty (property=%s)",
                ssPropertyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetName:
        {
            auto& spNameValue = m_spValue;
            xstring_ptr ssName;
            if (spNameValue)
            {
                IFC_RETURN(spNameValue->GetCopyAsString(&ssName));
            }

            strValue = StringCchPrintfWWrapper(
                L"SetName (x:Name=%s)",
                ssName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetResourceDictionaryItems:
        {
            strValue = StringCchPrintfWWrapper(
                L"SetResourceDictionaryItems");
        }
        break;

        case ObjectWriterNodeType::SetDeferredProperty:
        {
            std::shared_ptr<XamlProperty> spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            xstring_ptr spPropertyName;
            IFC_RETURN(spProperty->get_FullName(&spPropertyName));

            strValue = StringCchPrintfWWrapper(
                L"SetDeferredProperty (property=%s)",
                spPropertyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValue:
        {
            std::shared_ptr<XamlProperty> spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            xstring_ptr spPropertyName;
            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            strValue = StringCchPrintfWWrapper(
                L"SetValue (property=%s)",
                spPropertyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValueFromMarkupExtension:
        {
            std::shared_ptr<XamlProperty> spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            xstring_ptr spPropertyName;
            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            strValue = StringCchPrintfWWrapper(
                L"SetValueFromMarkupExtension (property=%s)",
                spPropertyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValueConstant:
        {
            std::shared_ptr<XamlProperty> spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            std::shared_ptr<XamlQualifiedObject> spValue = m_spValue;
            xstring_ptr spPropertyName;
            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());

            strValue = StringCchPrintfWWrapper(
                L"SetValueConstant (property=%s, %s)",
                spPropertyName.GetBuffer(),
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValueTypeConvertedConstant:
        {
            std::shared_ptr<XamlProperty> spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            std::shared_ptr<XamlQualifiedObject> spValue = m_spValue;
            xstring_ptr spPropertyName;
            xstring_ptr spTypeConvertorTypeName = xstring_ptr::EmptyString();
            auto& spTypeConverter = m_spTextSyntaxConverter;

            const CClassInfo* pClassInfo = MetadataAPI::GetClassInfoByIndex(spTypeConverter->get_TextSyntaxToken().GetHandle());
            spTypeConvertorTypeName = pClassInfo->GetFullName();

            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            xstring_ptr strCValueString = ToStringCValueHelper(spValue->GetValue());

            strValue = StringCchPrintfWWrapper(
                L"SetValueTypeConvertedConstant (property=%s, type converter=%s, %s)",
                spPropertyName.GetBuffer(),
                spTypeConvertorTypeName.GetBuffer(),
                strCValueString.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValueTypeConvertedResolvedType:
        {
            std::shared_ptr<XamlProperty> spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            std::shared_ptr<XamlType> spTypeProxy = std::static_pointer_cast<XamlType>(m_spShared2);
            xstring_ptr spPropertyName;
            xstring_ptr spTypeProxyName;
            xstring_ptr spTypeConvertorTypeName = xstring_ptr::EmptyString();
            auto& spTypeConverter = m_spTextSyntaxConverter;

            const CClassInfo* pClassInfo = MetadataAPI::GetClassInfoByIndex(spTypeConverter->get_TextSyntaxToken().GetHandle());
            spTypeConvertorTypeName = pClassInfo->GetFullName();

            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            IFC_RETURN(spTypeProxy->get_FullName(&spTypeProxyName));

            strValue = StringCchPrintfWWrapper(
                L"SetValueTypeConvertedResolvedType (property=%s, type converter=%s, type proxy=%s)",
                spPropertyName.GetBuffer(),
                spTypeConvertorTypeName.GetBuffer(),
                spTypeProxyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValueTypeConvertedResolvedProperty:
        {
            std::shared_ptr<XamlProperty> spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            std::shared_ptr<XamlProperty> spPropertyProxy = std::static_pointer_cast<XamlProperty>(m_spShared2);
            xstring_ptr spPropertyName;
            xstring_ptr spPropertyProxyName;
            xstring_ptr spTypeConvertorTypeName = xstring_ptr::EmptyString();
            auto& spTypeConverter = m_spTextSyntaxConverter;

            const CClassInfo* pClassInfo = MetadataAPI::GetClassInfoByIndex(spTypeConverter->get_TextSyntaxToken().GetHandle());
            spTypeConvertorTypeName = pClassInfo->GetFullName();

            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            IFC_RETURN(spPropertyProxy->get_FullName(&spPropertyProxyName));

            strValue = StringCchPrintfWWrapper(
                L"SetValueTypeConvertedResolvedProperty (property=%s, type converter=%s, property proxy=%s)",
                spPropertyName.GetBuffer(),
                spTypeConvertorTypeName.GetBuffer(),
                spPropertyProxyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::SetValueFromTemplateBinding:
        {
            std::shared_ptr<XamlProperty> spProperty = std::static_pointer_cast<XamlProperty>(m_spShared);
            std::shared_ptr<XamlProperty> spPropertyProxy = std::static_pointer_cast<XamlProperty>(m_spShared2);
            xstring_ptr spPropertyName;
            xstring_ptr spPropertyProxyName;

            IFC_RETURN(spProperty->get_FullName(&spPropertyName));
            IFC_RETURN(spPropertyProxy->get_FullName(&spPropertyProxyName));

            strValue = StringCchPrintfWWrapper(
                L"SetValueFromTemplateBinding (property=%s, templatebinding property=%s)",
                spPropertyName.GetBuffer(),
                spPropertyProxyName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::StreamOffsetMarker:
        {
            UINT32 offsetMarkerIndex = m_intValue;

            strValue = StringCchPrintfWWrapper(
                L"OffsetMarker (index=%d)",
                offsetMarkerIndex);
        }
        break;

        case ObjectWriterNodeType::TypeConvertValue:
        {
            xstring_ptr spTypeConvertorTypeName = xstring_ptr::EmptyString();
            auto& spTypeConverter = m_spTextSyntaxConverter;

            const CClassInfo* pClassInfo = MetadataAPI::GetClassInfoByIndex(spTypeConverter->get_TextSyntaxToken().GetHandle());
            spTypeConvertorTypeName = pClassInfo->GetFullName();

            strValue = StringCchPrintfWWrapper(
                L"TypeConvert (type converter=%s)",
                spTypeConvertorTypeName.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::BeginConditionalScope:
        {
            auto& predicateAndArgs = GetXamlPredicateAndArgs();

            xstring_ptr predicateName;
            IFC_RETURN(predicateAndArgs->PredicateType->get_FullName(&predicateName));

            strValue = StringCchPrintfWWrapper(
                L"BeginConditionalScope (predicate=%s, arguments=%s",
                predicateName.GetBuffer(),
                predicateAndArgs->Arguments.GetBuffer());
        }
        break;

        case ObjectWriterNodeType::EndConditionalScope:
        {
            strValue = StringCchPrintfWWrapper(
                L"EndConditionalScope");
        }
        break;

        default:
        {
            // ToString() implementation for this type is unavailable.
            // please consider providing an implementation that can dump
            // more information.
            ASSERT(false);
        }
        break;
    }

    return S_OK;
}

xstring_ptr ObjectWriterNode::ToStringCValueHelper(_In_ const CValue& inValue) const
{
    XTHICKNESS* thicknessValue = nullptr;
    XGRIDLENGTH* gridLengthValue = nullptr;
    xstring_ptr strValue;

    switch (inValue.GetType())
    {
        case valueEnum:
        case valueEnum8:
        {
            uint32_t enumValue = 0;
            KnownTypeIndex enumTypeIndex = KnownTypeIndex::UnknownType;
            inValue.GetEnum(enumValue, enumTypeIndex);

            auto type = DirectUI::MetadataAPI::GetClassInfoByIndex(enumTypeIndex);
            const wchar_t* buffer = type->GetName().GetBuffer();

            strValue = StringCchPrintfWWrapper(
                L"type=Enum, value=[type=%s, value=%u]", buffer == nullptr ? L"" : buffer, enumValue);
            break;
        }

        case valueBool:
        {
            strValue = StringCchPrintfWWrapper(
                L"type=Bool, value=%s",
                (inValue.AsBool() == FALSE ? L"False" : L"True"));
            break;
        }

        case valueFloat:
        {
            strValue = StringCchPrintfWWrapper(
                L"type=Float, value=%f",
                inValue.AsFloat());
            break;
        }

        case valueSigned:
        {
            strValue = StringCchPrintfWWrapper(
                L"type=Signed, value=%d",
                inValue.AsSigned());
            break;
        }

        case valueString:
        {
            strValue = StringCchPrintfWWrapper(
                L"type=String, value=%s",
                (inValue.AsString().GetCount()>128 ? xstring_ptr::EmptyString().GetBuffer() : inValue.AsString().GetBuffer()));
            break;
        }

        case valueThickness:
        {
            thicknessValue = inValue.AsThickness();
            strValue = StringCchPrintfWWrapper(
                L"type=Thickness, value=[left=%f, right=%f, top=%f, bottom=%f]",
                thicknessValue->left,
                thicknessValue->right,
                thicknessValue->top,
                thicknessValue->bottom);
            break;
        }

        case valueGridLength:
        {
            gridLengthValue = inValue.AsGridLength();
            strValue = StringCchPrintfWWrapper(
                L"type=GridLength, value=[value=%f, type=%d]",
                gridLengthValue->value,
                gridLengthValue->type);
            break;
        }

        case valueColor:
        {
            strValue = StringCchPrintfWWrapper(
                L"type=Color, value=%x",
                inValue.AsColor());
            break;
        }

        default:
        {
            strValue = StringCchPrintfWWrapper(
                L"type=Unknown");
            break;
        }
    }

    return strValue;
}
