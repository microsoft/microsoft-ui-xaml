/* Copyright (c) Microsoft Corporation.  All rights reserved. */

lexer grammar XbfGrammarLexer;

options 
{ 
    language = CSharp;
}

BYTE
    : '\u0000'..'\u00FF'
    ;

//NODETYPE
//  : NODETYPE_PushScope
//    | NODETYPE_PopScope
//    | NODETYPE_AddNamespace
//    | NODETYPE_PushConstant
//    | NODETYPE_SetValue
//    | NODETYPE_SetValueFromMarkupExtension
//    | NODETYPE_AddToCollection
//    | NODETYPE_AddToDictionary
//    | NODETYPE_AddToDictionaryWithKey
//    | NODETYPE_CheckPeerType
//    | NODETYPE_SetConnectionId
//    | NODETYPE_SetName
//    | NODETYPE_GetResourcePropertyBag
//    | NODETYPE_ProvideValue
//    | NODETYPE_SetDeferredProperty
//    | NODETYPE_SetCustomRuntimeData
//    | NODETYPE_PushScopeAddNamespace
//    | NODETYPE_PushScopeGetValue
//    | NODETYPE_PushScopeCreateTypeBeginInit
//    | NODETYPE_PushScopeCreateTypeWithConstantBeginInit
//    | NODETYPE_PushScopeCreateTypeWithTypeConvertedConstantBeginInit
//    | NODETYPE_CreateTypeBeginInit
//    | NODETYPE_CreateTypeWithConstantBeginInit
//    | NODETYPE_CreateTypeWithTypeConvertedConstantBeginInit
//    | NODETYPE_SetValueConstant
//    | NODETYPE_SetValueTypeConvertedConstant
//    | NODETYPE_SetValueTypeConvertedResolvedType
//    | NODETYPE_SetValueTypeConvertedResolvedProperty
//    | NODETYPE_ProvideStaticResourceValue
//    | NODETYPE_SetValueFromStaticResource
//    | NODETYPE_ProvideThemeResourceValue
//    | NODETYPE_SetValueFromThemeResource
//    | NODETYPE_SetValueFromTemplateBinding
//    | NODETYPE_EndInitPopScope
//    | NODETYPE_BeginConditionalScope
//    | NODETYPE_EndConditionalScope
//  ;

// ObjectWriterNodeType definitions
//NODETYPE_PushScope                                                  : '\u0001' ;
//NODETYPE_PopScope                                                   : '\u0002' ;
//NODETYPE_AddNamespace                                               : '\u0003' ;
//NODETYPE_PushConstant                                               : '\u0004' ;
//NODETYPE_SetValue                                                   : '\u0007' ;
//NODETYPE_SetValueFromMarkupExtension                                : '\u0020' ;
//NODETYPE_AddToCollection                                            : '\u0008' ;
//NODETYPE_AddToDictionary                                            : '\u0009' ;
//NODETYPE_AddToDictionaryWithKey                                     : '\u000A' ;
//NODETYPE_CheckPeerType                                              : '\u000B' ;
//NODETYPE_SetConnectionId                                            : '\u000C' ;
//NODETYPE_SetName                                                    : '\u000D' ;
//NODETYPE_GetResourcePropertyBag                                     : '\u000E' ;
//NODETYPE_ProvideValue                                               : '\u008B' ;
//NODETYPE_SetDeferredProperty                                        : '\u0011' ;
//NODETYPE_SetCustomRuntimeData                                       : '\u000F' ;
//NODETYPE_PushScopeAddNamespace                                      : '\u0012' ;
//NODETYPE_PushScopeGetValue                                          : '\u0013' ;
//NODETYPE_PushScopeCreateTypeBeginInit                               : '\u0014' ;
//NODETYPE_PushScopeCreateTypeWithConstantBeginInit                   : '\u0015' ;
//NODETYPE_PushScopeCreateTypeWithTypeConvertedConstantBeginInit      : '\u0016' ;
//NODETYPE_CreateTypeBeginInit                                        : '\u0017' ;
//NODETYPE_CreateTypeWithConstantBeginInit                            : '\u0018' ;
//NODETYPE_CreateTypeWithTypeConvertedConstantBeginInit               : '\u0019' ;
//NODETYPE_SetValueConstant                                           : '\u001A' ;
//NODETYPE_SetValueTypeConvertedConstant                              : '\u001B' ;
//NODETYPE_SetValueTypeConvertedResolvedType                          : '\u001D' ;
//NODETYPE_SetValueTypeConvertedResolvedProperty                      : '\u001C' ;
//NODETYPE_ProvideStaticResourceValue                                 : '\u0022' ;
//NODETYPE_SetValueFromStaticResource                                 : '\u001E' ;
//NODETYPE_ProvideThemeResourceValue                                  : '\u0023' ;
//NODETYPE_SetValueFromThemeResource                                  : '\u0024' ;
//NODETYPE_SetValueFromTemplateBinding                                : '\u001F' ;
//NODETYPE_EndInitPopScope                                            : '\u0021' ;
//NODETYPE_BeginConditionalScope                                      : '\u0026' ;
//NODETYPE_EndConditionalScope                                        : '\u0027' ;