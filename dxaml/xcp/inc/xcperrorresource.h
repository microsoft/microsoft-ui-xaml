// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This file contains a large number of error codes. Some of them map to strings
// in the DLL resource file. Most of them are probably no longer used and could be
// safely removed on a rainy day.

#pragma once

#define TEXTFILE        256

// Error Code, shareable by both Core and Host code.
#define   AG_E_UNKNOWN_ERROR                   1001

// The range 1002-2000 is reserved for non-string resources defined in mediaresource.h

#define   AG_E_PARSER_NO_DTDS                  2001
#define   AG_E_PARSER_INVALID_ENTITY_REF       2002
#define   AG_E_PARSER_INVALID_ENCODING         2004
#define   AG_E_PARSER_MULT_PROP_VALUES         2008
#define   AG_E_PARSER_INVALID_CONTENT          2010
#define   AG_E_PARSER_INVALID_ATTR_VALUE       2024
#define   AG_E_PARSER_DUPLICATE_NAME           2028
#define   AG_E_PARSER_RESOURCE_NAMENOTSET      2034
#define   AG_E_PARSER_NO_CUSTOM_RESOURCE_LOADER 2037

#define   AG_E_INIT_CALLBACK                   2100
#define   AG_E_INIT_ROOTVISUAL                 2101


#define   AG_E_RUNTIME_INVALID_CALL            2201
#define   AG_E_RUNTIME_SETVALUE                2203
#define   AG_E_INVALID_ARGUMENT                2210
#define   AG_E_RUNTIME_HTML_ACCESS_RESTRICTED  2211

#define   AG_E_RUNTIME_SB_BEGIN_NO_TARGET               2212
#define   AG_E_RUNTIME_SB_BEGIN_INVALID_TARGET          2213
#define   AG_E_RUNTIME_SB_BEGIN_INVALID_PROP            2214
#define   AG_E_RUNTIME_SB_BEGIN_INCOMPATIBLE_TYPE       2215
#define   AG_E_RUNTIME_SB_BEGIN_ANIM_COMPOSITION        2216
#define   AG_E_RUNTIME_SB_BEGIN_INVALID_KEYTIME         2217
#define   AG_E_RUNTIME_SB_MODIFY_ACTIVE_ANIMATION       2218
#define   AG_E_RUNTIME_SB_MUST_BE_ROOT                  2219

#define   AG_E_RUNTIME_STYLE_TARGETTYPE                 2223
#define   AG_E_RUNTIME_STYLE_DEFAULTSTYLERESOURCEURI_NOT_FOUND 2224

#define   AG_E_RUNTIME_MANAGED_UNKNOWN_ERROR            2250
#define   AG_E_RUNTIME_MANAGED_ACTIVATION               2251
#define   AG_E_RUNTIME_MANAGED_ASSEMBLY_DOWNLOAD        2252
#define   AG_E_RUNTIME_MANAGED_ASSEMBLY_LOAD            2253
#define   AG_E_PARSER_UNKNOWN_TYPE                      2254
#define   AG_E_PARSER_BAD_TYPE                          2255
#define   AG_E_PARSER_BAD_NATIVE_TYPE                   2256
#define   AG_E_PARSER_CREATE_OBJECT_FAILED              2257
#define   AG_E_PARSER_PROPERTY_NOT_FOUND                2258
#define   AG_E_PARSER_BAD_PROPERTY_TYPE                 2259
#define   AG_E_PARSER_BAD_PROPERTY_VALUE                2260
#define   AG_E_PARSER_ROOT_NOT_CUSTOM                   2261
#define   AG_E_PARSER_NAMESPACE_NOT_SUPPORTED           2262
#define   AG_E_PARSER_MISSING_DEFAULT_NAMESPACE         2263
#define   AG_E_PARSER_INVALID_XMLNS                     2264
#define   AG_E_PARSER_INVALID_CLASS                     2265
#define   AG_E_RUNTIME_MANAGED_BAD_DLR_SCRIPT           2266
#define   AG_E_PARSER_RESOURCE_KEYANDNAMESET            2269
#define   AG_E_PARSER_FAILED_RESOURCE_FIND              2272
#define   AG_E_PARSER_RESOURCE_KEYCONFLICT              2273
#define   AG_E_RUNTIME_MANAGED_ASSEMBLY_NOT_FOUND       2274
#define   AG_E_INVALID_SOURCE_URI                       2275
#define   AG_E_RUNTIME_CHROME_WRONG_PARENT              2276
#define   AG_E_RUNTIME_INVALID_RESOURCE                 2277

#define   AG_E_RUNTIME_SHOWN_HIDDEN_MIXED_WITH_ECP      2278

// New parser strings
#define   AG_E_PARSER2_UNKNOWN_PROP_ON_TYPE             2500
#define   AG_E_PARSER2_UNKNOWN_ATTACHABLE_PROP_ON_TYPE  2501
#define   AG_E_PARSER2_UNKNOWN_TYPE                     2502
#define   AG_E_PARSER2_NESTED_PROP_ELEM                 2503
#define   AG_E_PARSER2_CANT_SET_PROPS_ON_PROP_ELEM      2504
#define   AG_E_PARSER2_UNKNOWN_NAMESPACE                2505
#define   AG_E_PARSER2_UNDECLARED_PREFIX                2506
#define   AG_E_PARSER2_INTERNAL_OW_UNKNOWN_TYPE         2508
#define   AG_E_PARSER2_INTERNAL_OW_GENERIC              2510
#define   AG_E_PARSER2_OW_DUPLICATE_MEMBER              2511
#define   AG_E_PARSER2_OW_TYPE_CONVERSION_FAILED        2512
#define   AG_E_PARSER2_OW_ITEMS_IN_DICTIONARY_MUST_HAVE_KEY  2513
#define   AG_E_PARSER2_SCANNER_UNKNOWN_ERROR            2514
#define   AG_E_PARSER2_PARSER_UNKNOWN_ERROR             2515
#define   AG_E_PARSER2_OW_WRONG_ITEM_TYPE_FOR_COLLECTION  2516
#define   AG_E_PARSER2_MES_UNCLOSED_QUOTE               2517
#define   AG_E_PARSER2_OW_MARKUP_EXTENSION_COULD_NOT_PROVIDE_VALUE  2518
#define   AG_E_PARSER2_INTERNAL_MES_GENERIC             2519
#define   AG_E_PARSER2_MES_UNEXPECTED_QUOTE             2520
#define   AG_E_PARSER2_MES_EXPECTED_SYMBOL              2521
#define   AG_E_PARSER2_MES_EXPECTED_TYPE                2522
#define   AG_E_PARSER2_MES_EXPECTED_PROPERTY            2523
#define   AG_E_PARSER2_MES_EXPECTED_VALUE               2524
#define   AG_E_PARSER2_INTERNAL_MES_UNKNOWN_TOKEN       2525
#define   AG_E_PARSER2_MES_ONLY_ONE_POSITIONAL          2526
#define   AG_E_PARSER2_MES_UNEXPECTED_TEXT_AFTER_ME     2527
#define   AG_E_PARSER2_MES_TRAILING_COMMA               2528
#define   AG_E_PARSER2_MES_NOT_A_MARKUP_EXTENSION       2529
#define   AG_E_PARSER2_INVALID_TYPENAME                 2530
#define   AG_E_PARSER2_OW_CANT_SET_VALUE_GENERIC        2531
#define   AG_E_PARSER2_OW_DUPLICATE_KEY                 2532
#define   AG_E_PARSER2_OW_X_CLASS_MUST_BE_ON_ROOT       2533
#define   AG_E_PARSER2_NO_EVENT_ROOT                    2534
#define   AG_E_PARSER2_TYPE_UNCREATEABLE                2535
#define   AG_E_PARSER2_OW_CANT_ADD_TEXT_TO_COLLECTION   2536
#define   AG_E_PARSER2_PROPERTY_ELEMENT_AT_ROOT         2537
#define   AG_E_PARSER2_NO_DEFAULT_NAMESPACE             2538
#define   AG_E_PARSER2_CANNOT_ADD_ANY_CHILDREN          2539
#define   AG_E_PARSER2_OW_NOT_ASSIGNABLE_FROM           2540
#define   AG_E_PARSER2_OW_NO_CTOR                       2541
#define   AG_E_PARSER2_OW_READ_ONLY                     2542
#define   AG_E_PARSER2_OW_COLLECTION_NULL               2543
#define   AG_E_PARSER2_OW_INVALID_DICTIONARY_KEY_TYPE   2544
#define   AG_E_PARSER2_CANNOT_RESOLVE_PROP_FROM_UID     2545
#define   AG_E_PARSER2_XBF_METADATA_OFFSET              2546
#define   AG_E_PARSER2_XBF_METADATA_DESERIALIZE         2547
#define   AG_E_PARSER2_XBF_METADATA_VERSION             2548
#define   AG_E_PARSER2_XBF_METADATA_ASSEMBLY_LIST       2549
#define   AG_E_PARSER2_XBF_METADATA_TYPE_NAMESPACE_LIST 2550
#define   AG_E_PARSER2_XBF_TYPE_LIST                    2551
#define   AG_E_PARSER2_XBF_METADATA_PROPERTY_LIST       2552
#define   AG_E_PARSER2_XBF_METADATA_XML_NAMESPACE_LIST  2553
#define   AG_E_PARSER2_DEFERRED_ELEMENT_MUST_HAVE_NAME  2554
#define   AG_E_PARSER2_DEFERRED_ELEMENT_CANNOT_BE_ROOT  2555
#define   AG_E_PARSER2_INVALID_DEFERLOADSTRATEGY_VALUE  2556
#define   AG_E_PARSER2_INVALID_PROPERTY_ON_DEFERRED_ELEMENT  2557
#define   AG_E_PARSER2_INVALID_REALIZE_VALUE            2558
#define   AG_E_PARSER2_INVALID_USE_OF_SETTER_PROPERTY   2559
#define   AG_E_PARSER2_DIRECTIVE_CANNOT_BE_CONDITIONAL  2560
#define   AG_E_PARSER2_TEMPLATEBINDING_NOT_ALLOWED_ON_PROPERTY  2561
#define   AG_E_PARSER2_BINDING_NOT_ALLOWED_ON_PROPERTY  2562
#define   AG_E_PARSER2_NONSHAREABLE_OBJECT_NOT_ALLOWED_ON_STYLE_SETTER  2563
#define   AG_E_PARSER2_OW_COLLECTION_ITEM_TYPE_MISSING_TYPE_CONVERTER    2564
#define   AG_E_PARSER2_INVALID_INITIALIZATION_STRING_TO_PARSE    2565

//

#define   AG_E_UNABLE_TO_PLAY                           3000
#define   AG_E_INVALID_FILE_FORMAT                      3001
#define   AG_E_NOT_FOUND                                3002
#define   AG_E_MEDIA_DISCONNECTED                       3003
#define   AG_E_ATTRIBUTENOTFOUND                        3009
#define   AG_E_END_OF_STREAM                            3010
#define   AG_E_INVALIDINDEX                             3011
#define   AG_E_INVALIDSTREAMNUMBER                      3012
#define   AG_E_NO_SAMPLE_DURATION                       3013
#define   AG_E_NO_SAMPLE_TIMESTAMP                      3014
#define   AG_E_SHUTDOWN                                 3015
#define   AG_E_INVALIDMEDIATYPE                         3016
#define   AG_E_INVALIDTYPE                              3017
#define   AG_E_INVALID_FORMAT                           3018
#define   AG_E_UNSUPPORTED_REPRESENTATION               3019
#define   AG_E_INVALIDREQUEST                           3020


//***********************************************************

#define   AG_E_NETWORK_ERROR                            4001

#define   AG_E_INVALID_LAYOUT_OPERATION                 4007
#define   AG_E_LAYOUT_CYCLE                             4008
#define   AG_E_MANAGED_ELEMENT_ASSOCIATED               4009
#define   AG_E_ITEMS_CONTROL_INVALID_PANEL              4010
#define   AG_E_USER_CONTROL_OP_UNSUPPORTED              4011
#define   AG_E_ITEMS_PANEL_TEMPLATE_CHILDREN            4012
#define   AG_E_RESOURCE_CYCLE_DETECTED                  4013
#define   AG_E_RESOURCE_LOCAL_VALUES_NOT_ALLOWED        4014
#define   AG_E_STYLE_BASEDON_CIRCULAR_REF               4015
#define   AG_E_STYLE_BASEDON_INVALID_TARGETTYPE         4016
#define   AG_E_STYLE_BASEDON_SELF                       4017
#define   AG_E_STYLE_CHANGE_AFTER_SEALED                4018
#define   AG_E_STYLE_BASEDON_TARGETTYPE_NULL            4019
#define   AG_E_PROPERTY_INVALID                         4020

#define   AG_E_CAPTURE_DEVICE_IN_USE                    4024
#define   AG_E_CAPTURE_DEVICE_REMOVED                   4025
#define   AG_E_CAPTURE_DEVICE_ACCESS_DENIED             4026
#define   AG_E_CAPTURE_SOURCE_NOT_STOPPED               4027
#define   AG_E_CAPTURE_DEVICE_NOT_AVAILABLE             4028
#define   AG_E_TEMPLATEBINDING_TEMPLATEDPARENT_NULL     4031
#define   AG_E_NO_VECTOR_PRINT                          4032

#define   AG_E_RESOURCE_THEME_NOT_A_DICTIONARY          4036
#define   AG_E_RELATIVEPANEL_INVALID_TYPE               4037
#define   AG_E_RELATIVEPANEL_CIRCULAR_DEP               4038
#define   AG_E_RELATIVEPANEL_REF_NOT_FOUND              4039
#define   AG_E_RELATIVEPANEL_NAME_NOT_FOUND             4040
#define   AG_E_SETTER_AMBIGUOUS_TARGET                  4041
#define   AG_E_TARGETPROPERTYPATH_REQUIRES_PATH         4042
#define   AG_E_TARGETPROPERTYPATH_REQUIRES_TARGET       4043
#define   AG_E_VSM_SETTER_MISSING_TARGET                4044
#define   AG_E_VSM_SETTER_MISSING_VALUE                 4045
#define   AG_E_TARGETPROPERTYPATH_CANT_RESOLVE_PATH     4047
#define   AG_E_STYLE_SETTER_EXPLICIT_TARGET_OBJECT      4048
#define   AG_E_TARGETPROPERTYPATH_CANT_RESOLVE_TARGET   4049
#define   AG_E_FOCUSENGAGEMENT_CANT_ENGAGE_WITHOUT_FOCUS 4050
#define   AG_E_FOCUSENGAGEMENT_CANT_ENGAGE_WITHOUT_ENGAGEMENT_ENABLED 4051
#define   AG_E_COLLECTION_MODIFIED_DURING_LAYOUT        4052
#define   AG_E_LOSTFOCUS_BINDING_REQUIRES_UIELEMENT     4053

#define TEXT_FRAME_NAVIGATION_FAILED_UNHANDLED  4054

// Binding logging strings
#define TEXT_BINDINGTRACE_BINDINGEXPRESSION_TRACE                                       4055
#define TEXT_BINDINGTRACE_CONVERT_FAILED                                                4056
#define TEXT_BINDINGTRACE_INT_INDEXER_FAILED                                            4057
#define TEXT_BINDINGTRACE_INT_INDEXER_CONNECTION_FAILED                                 4058
#define TEXT_BINDINGTRACE_GETTER_FAILED                                                 4059
#define TEXT_BINDINGTRACE_PROPERTY_CONNECTION_FAILED                                    4060
#define TEXT_BINDINGTRACE_STR_INDEXER_CONNECTION_FAILED                                 4061
#define TEXT_BINDINGTRACE_STR_INDEXER_FAILED                                            4062
#define TEXT_BINDINGTRACE_SETTER_FAILED                                                 4063

#define AG_E_PARSER_RESOURCEDICTIONARY_KEY_INVALIDARG                                   4064

#define ERROR_INVALID_DOUBLE_VALUE                                                      4065
#define ERROR_INVALID_MULTIPLE_SELECT                                                   4066
#define ERROR_TEMPLATE_TARGETTYPE_WRONG                                                 4067
#define ERROR_OBJECT_IS_FROZEN                                                          4068

#define ERROR_SCROLLVIEWER_MINZOOMFACTOR                                                4069
#define ERROR_SCROLLVIEWER_MAXZOOMFACTOR                                                4070
#define ERROR_NAVIGATION_UNSUPPORTED_PARAM_TYPE_FOR_SERIALIZATION                       4071

#define ERROR_INCORRECT_PANEL_FOR_CONTROL                                               4072
#define ERROR_INCORRECT_PANEL_FOR_GROUPITEM                                             4073

#define ERROR_SWAPCHAINBACKGROUNDPANEL_MUSTBEROOTVISUAL_OR_CHILDOFPAGE                  4074
#define ERROR_RESOURCEDICTIONARY_RESOURCE_LOOKUP_FAILED                                 4075

#define ERROR_FLYOUTBASE_NO_ATTACHED_FLYOUT                                             4077

#define ERROR_WEBVIEW_CANNOT_GO_BACK                                                    4078
#define ERROR_WEBVIEW_CANNOT_GO_FORWARD                                                 4079

#define ERROR_DEPENDENCYOBJECTCOLLECTION_OUTOFRANGE                                     4080
#define ERROR_GENERICXAML_INVALID_FILEPROTOCOL                                          4081

// Empty: 4082
// Empty: 4083

#define ERROR_FRAME_NAVIGATING                                                          4084
#define ERROR_PAGESTACK_ENTRY_OWNED                                                     4085

#define ERROR_SCROLLVIEWER_UNSUPPORTED_HORIZONTALALIGNMENT_WITH_HEADER                  4086
#define ERROR_SCROLLVIEWER_UNSUPPORTED_VERTICALALIGNMENT_WITH_HEADER                    4087
#define ERROR_SCROLLVIEWER_UNSUPPORTED_ISCROLLINFO_WITH_HEADER                          4088
#define ERROR_ELEMENT_ASSOCIATED                                                        4089
#define ERROR_HUB_DEFAULT_SECTION_INDEX_OUT_OF_RANGE                                    4090
#define ERROR_LISTVIEWBASE_REORDER_GROUPED_SOURCE                                       4092
#define ERROR_CONTENTDIALOG_MULTIPLE_OPEN                                               4093
#define ERROR_INCORRECT_PANEL_FOR_SERIALIZATION                                         4094
#define ERROR_ITEMSPANELROOT_NOT_READY                                                  4095
#define ERROR_DEFERRAL_COMPLETED                                                        4096
#define ERROR_ITEMCOLLECTION_REENTRANCY                                                 4097
#define ERROR_CALENDAR_NUMBER_OF_WEEKS_OUTOFRANGE                                       4098
#define ERROR_CALENDAR_CANNOT_SELECT_MORE_DATES                                         4099
#define ERROR_CALENDAR_CANNOT_SELECT_BLACKOUT_DATE                                      4100
#define ERROR_CALENDAR_INVALID_MIN_MAX_DATE                                             4101
#define ERROR_LISTVIEWBASE_GROUPHEADERCONTAINER_ALREADY_IN_USE                          4103
#define ERROR_SOFTWAREBITMAPSOURCE_INVALID_FORMAT                                       4104
#define ERROR_INVALID_PASSWORDBOX_INPUTSCOPE_VALUE                                      4105
#define ERROR_BIND_CANNOT_CONVERT_VALUE_TO_TYPE                                         4106
#define ERROR_ELEMENTSOUNDPLAYER_VOLUME_OUTOFRANGE                                      4107
#define ERROR_FOCUSRECT_NOSOLIDCOLORBRUSH                                               4108
#define ERROR_ACCESSKEYS_ACCESSKEYOWNER_ISSCOPE_FALSE                                   4109
#define ERROR_ACCESSKEYS_ACCESSKEYOWNER_CDO                                             4110
#define ERROR_FOCUSMANAGER_MOVING_FOCUS                                                 4112
#define ERROR_INVALID_FINDNEXTELEMENT_OPTION_DIRECTION                                  4113
#define ERROR_TEXTHIGHLIGHTER_NOSOLIDCOLORBRUSH                                         4114
#define ERROR_STALE_METADATA_ACCESS                                                     4115
#define ERROR_PROPERTY_SOLIDCOLORBRUSHONLY                                              4116
#define IDS_ELEMENT_ENABLED_ONLY_IN_APP                                                 4117
#define ERROR_ACCESS_DENIED_WITH_PROPERTY_IN_USE                                        4118
#define ERROR_ACCESSING_NON_STRICT_API_ON_STRICT_TYPE                                   4119
#define ERROR_FOCUSMANAGER_ASYNCOP_INPROGRESS                                           4120
#define ERROR_FLYOUT_EMPTYSHOWOPTIONS                                                   4122
#define ERROR_ANIMATION_TARGET_UNSPECIFIED                                              4124
#define ERROR_ANIMATION_PROPERTY_UNRECOGNIZED                                           4125
#define ERROR_ANIMATION_MULTI_TARGET_UNSUPPORTED                                        4126
#define ERROR_STANDARDUICOMMAND_KINDNOTSET                                              4127
#define ERROR_ANCESTOR_ELEMENT_CANNOT_BE_SHADOW_RECEIVER                                4128
#define ERROR_DESKTOPWINDOWXAMLSOURCE_WINDOW_IS_ON_DIFFERENT_THREAD                     4130
#define ERROR_INVALIDATEVIEWPORT_REQUIRES_SCROLLER                                      4131
#define ERROR_POPUP_XAMLROOT_NOT_SET                                                    4132
#define ERROR_ANIMATION_PROPERTY_READONLY                                               4133
#define ERROR_HIT_TEST_IS_NOT_ASSOCIATED_WITH_CONTENT_TREE                              4134
#define ERROR_POPUP_SHOULDCONSTRAINTOROOTBOUNDS_CANNOT_BE_CHANGED_AFTER_OPEN            4135
#define ERROR_XAMLROOT_AMBIGUOUS                                                        4136
#define ERROR_CANNOT_SET_XAMLROOT_WHEN_NOT_NULL                                         4137
#define ERROR_RASTERIZATIONSCALE_MUST_BE_POSITIVE                                       4139
#define ERROR_CONNECTED_ANIMATIONS_BETWEEN_ELEMENTS_IN_DIFFERENT_XAMLROOTS_ARE_NOT_SUPPORTED 4140
#define ERROR_WINDOW_DESKTOP_BOUNDS_FAILED                                              4141 
#define ERROR_WINDOW_DESKTOP_TITLE_FAILED                                               4143 
#define ERROR_WINDOW_DESKTOP_SIZE_OR_POSITION_FAILED                                    4145
#define ERROR_WINDOW_DESKTOP_ALREADY_CLOSED                                             4147 
#define ERROR_WINDOW_DESKTOP_NOT_IMPLEMENTED                                            4148
#define ERROR_API_NOT_IMPLEMENTED_UWP                                                   4149 
#define ERROR_XAMLRENDERINGBACKGROUNDTASK_NOT_AVAILABLE_IN_WINUI3                       4150
#define ERROR_INVALID_SEARCHROOT_WIN32                                                  4151
#define ERROR_API_NOT_SUPPORTED_WIN32                                                   4152
#define ERROR_FINDNEXTELEMENT_WITHOUT_FINDNEXTELEMENTOPTIONS_WIN32                      4153
#define ERROR_TRYMOVEFOCUS_WITHOUT_FINDNEXTELEMENTOPTIONS_WIN32                         4154
#define ERROR_XAMLROOT_REQUIRED                                                         4155

//
//
//
// NOTE: The range 5000-5999 is reserved for localized string resources
// See localizedResource.h
//
//
//

// Xap Verification Error Codes
#define AG_E_INVALID_APP_SIGNATURE                         6052

#define AG_E_REENTRANCY_DETECTED                           6501

#define UIA_OPERATION_CANNOT_BE_PERFORMED                  6901
#define UIA_INVALID_ITEMSCONTROL_PARENT                    6902
#define UIA_ELEMENT_IS_VIRTUALIZED                         6903
#define UIA_GETTEXT_OUTOFRANGE_LENGTH                      6905

#define AG_E_PAGE_INVALID_PROPERTY_SET_HAS_SCBP_CHILD               6910
#define AG_E_SWAPCHAINBACKGROUNDPANEL_UNSUPPORTED_ALPHAMODE         6911
#define AG_E_SWAPCHAINBACKGROUNDPANEL_UNSUPPORTED_SWAPCHAINTYPE     6912
#define AG_E_SWAPCHAINBACKGROUNDPANEL_ERROR_SETUNSUPPORTEDPROPERTY  6913
#define AG_E_PAGE_MUST_BE_ROOT_WHEN_SCBP_CHILD                      6914
#define AG_E_SWAPCHAINPANEL_UNSUPPORTED_SWAPCHAINTYPE     6915
#define AG_E_SWAPCHAINPANEL_ERROR_SETUNSUPPORTEDPROPERTY  6916

#define AG_E_INVALID_MIXED_MANIPULATION_MODE               6920

// XmlLite XML parser error codes (these each map to an HRESULT returned by XmlLite)

#define  XMLLITE_MX_E_INPUTEND                         7000
#define  XMLLITE_MX_E_ENCODING                         7001
#define  XMLLITE_MX_E_ENCODINGSWITCH                   7002
#define  XMLLITE_MX_E_ENCODINGSIGNATURE                7003
#define  XMLLITE_WC_E_WHITESPACE                       7004
#define  XMLLITE_WC_E_SEMICOLON                        7005
#define  XMLLITE_WC_E_GREATERTHAN                      7006
#define  XMLLITE_WC_E_QUOTE                            7007
#define  XMLLITE_WC_E_EQUAL                            7008
#define  XMLLITE_WC_E_LESSTHAN                         7009
#define  XMLLITE_WC_E_HEXDIGIT                         7010
#define  XMLLITE_WC_E_DIGIT                            7011
#define  XMLLITE_WC_E_LEFTBRACKET                      7012
#define  XMLLITE_WC_E_LEFTPAREN                        7013
#define  XMLLITE_WC_E_XMLCHARACTER                     7014
#define  XMLLITE_WC_E_NAMECHARACTER                    7015
#define  XMLLITE_WC_E_SYNTAX                           7016
#define  XMLLITE_WC_E_CDSECT                           7017
#define  XMLLITE_WC_E_COMMENT                          7018
#define  XMLLITE_WC_E_CONDSECT                         7019
#define  XMLLITE_WC_E_DECLATTLIST                      7020
#define  XMLLITE_WC_E_DECLDOCTYPE                      7021
#define  XMLLITE_WC_E_DECLELEMENT                      7022
#define  XMLLITE_WC_E_DECLENTITY                       7023
#define  XMLLITE_WC_E_DECLNOTATION                     7024
#define  XMLLITE_WC_E_NDATA                            7025
#define  XMLLITE_WC_E_PUBLIC                           7026
#define  XMLLITE_WC_E_SYSTEM                           7027
#define  XMLLITE_WC_E_NAME                             7028
#define  XMLLITE_WC_E_ROOTELEMENT                      7029
#define  XMLLITE_WC_E_ELEMENTMATCH                     7030
#define  XMLLITE_WC_E_UNIQUEATTRIBUTE                  7031
#define  XMLLITE_WC_E_TEXTXMLDECL                      7032
#define  XMLLITE_WC_E_LEADINGXML                       7033
#define  XMLLITE_WC_E_TEXTDECL                         7034
#define  XMLLITE_WC_E_XMLDECL                          7035
#define  XMLLITE_WC_E_ENCNAME                          7036
#define  XMLLITE_WC_E_PUBLICID                         7037
#define  XMLLITE_WC_E_PESINTERNALSUBSET                7038
#define  XMLLITE_WC_E_PESBETWEENDECLS                  7039
#define  XMLLITE_WC_E_NORECURSION                      7040
#define  XMLLITE_WC_E_ENTITYCONTENT                    7041
#define  XMLLITE_WC_E_UNDECLAREDENTITY                 7042
#define  XMLLITE_WC_E_PARSEDENTITY                     7043
#define  XMLLITE_WC_E_NOEXTERNALENTITYREF              7044
#define  XMLLITE_WC_E_PI                               7045
#define  XMLLITE_WC_E_SYSTEMID                         7046
#define  XMLLITE_WC_E_QUESTIONMARK                     7047
#define  XMLLITE_WC_E_CDSECTEND                        7048
#define  XMLLITE_WC_E_MOREDATA                         7049
#define  XMLLITE_WC_E_DTDPROHIBITED                    7050
#define  XMLLITE_NC_E_QNAMECHARACTER                   7051
#define  XMLLITE_NC_E_QNAMECOLON                       7052
#define  XMLLITE_NC_E_NAMECOLON                        7053
#define  XMLLITE_NC_E_DECLAREDPREFIX                   7054
#define  XMLLITE_NC_E_UNDECLAREDPREFIX                 7055
#define  XMLLITE_NC_E_EMPTYURI                         7056
#define  XMLLITE_NC_E_XMLPREFIXRESERVED                7057
#define  XMLLITE_NC_E_XMLNSPREFIXRESERVED              7058
#define  XMLLITE_NC_E_XMLURIRESERVED                   7059
#define  XMLLITE_NC_E_XMLNSURIRESERVED                 7060
#define  XMLLITE_XML_E_INVALID_DECIMAL                 7061
#define  XMLLITE_XML_E_INVALID_HEXIDECIMAL             7062
#define  XMLLITE_XML_E_INVALID_UNICODE                 7063
#define  XMLLITE_XML_E_INVALIDENCODING                 7064


// These are HRESULTs that will be returned by the managed framework,
// and they should be kept in sync with errors.cs.

#define CLR_E_RUNTIME_MANAGED_UNKNOWN_ERROR             0x800F08CA
#define CLR_E_RUNTIME_MANAGED_ACTIVATION                0x800F08CB
#define CLR_E_RUNTIME_MANAGED_ASSEMBLY_DOWNLOAD         0x800F08CC
#define CLR_E_RUNTIME_MANAGED_ASSEMBLY_LOAD             0x800F08CD
#define CLR_E_PARSER_UNKNOWN_TYPE                       0x800F08CE
#define CLR_E_PARSER_BAD_TYPE                           0x800F08CF
#define CLR_E_PARSER_BAD_NATIVE_TYPE                    0x800F08C0
#define CLR_E_PARSER_CREATE_OBJECT_FAILED               0x800F08D1
#define CLR_E_PARSER_PROPERTY_NOT_FOUND                 0x800F08D2
#define CLR_E_PARSER_BAD_PROPERTY_TYPE                  0x800F08D3
#define CLR_E_PARSER_BAD_PROPERTY_VALUE                 0x800F08D4
#define CLR_E_PARSER_ROOT_NOT_CUSTOM                    0x800F08D5
#define CLR_E_PARSER_NAMESPACE_NOT_SUPPORTED            0x800F08D6
#define CLR_E_PARSER_MISSING_DEFAULT_NAMESPACE          0x800F08D7
#define CLR_E_PARSER_INVALID_XMLNS                      0x800F08D8
#define CLR_E_PARSER_INVALID_CLASS                      0x800F08D9
#define CLR_E_RUNTIME_MANAGED_ASSEMBLY_NOT_FOUND        0x800F08DB
#define CLR_E_PARSER_INVALID_CONTENT                    0x800F08DC
#define CLR_E_PARSER_PROP_READONLY                      0x800F08DD
#define CLR_E_APPSERVICE_NO_INTERFACE                   0x800F08DE

// These are HRESULTs that will be returned by ASX parser.
#define ASX_E_INVALID_ELEMENT                           0x800F08E0
#define ASX_E_INVALID_ATTRIBUTE                         0x800F08E1
#define ASX_E_UNSUPPORTED_ELEMENT                       0x800F08E2
#define ASX_E_UNSUPPORTED_ATTRIBUTE                     0x800F08E3
#define ASX_E_PARSE_ERROR                               0x800F08E4

#define E_HTMLACCESS_DENIED                             0x800F0900
#define E_DO_RESOURCE_NAMENOTSET                        0x800F0901
#define E_DO_RESOURCE_KEYCONFLICT                       0x800F0902
#define E_DO_RESOURCE_KEYANDNAMESET                     0x800F0903
#define E_DO_INVALID_CONTENT                            0x800F0904

// The inheritance context needs to be updated before the storyboard can begin.
#define E_DO_INHERITANCE_CONTEXT_NEEDED                 0x800F0905

// New setvalue return codes for SL4 parser
#define E_SV_NO_EVENT_ROOT                              0x800F0A00

//---------------

// The following HRESULTs indicate to that a native Error object has
// been registered for this failure. NER = Native Error Registered.
#define E_NER_INVALID_OPERATION                         0x800F1000
#define E_NER_ARGUMENT_EXCEPTION                        0x800F1001

// This HRESULT has the same meaning as E_FAIL. It is used by managed
// code when marshalling some exceptions to HRESULTS, and indicates
// that the full exception object has been stored on the managed side.
//
// If this HRESULT is marshalled back into an exception on the
// managed side, the stored exception object can be used.
#define E_FAIL_MANAGED                                  0x800F2000

#define E_INVALID_CHARS_IN_URI                          0x80000012
#define E_PLATFORM_EXTENSION_FAILED                     0x80000013
#define E_INVALID_APP_SIGNATURE                         0x80000016


// These are UIA specific HRESULTS used for Marshalling
// These are all in FACILITY_ITF
#define UIA_E_ELEMENTNOTENABLED      0x80040200
#define UIA_E_ELEMENTNOTAVAILABLE    0x80040201
#define UIA_E_NOCLICKABLEPOINT       0x80040202
#define UIA_E_PROXYASSEMBLYNOTLOADED 0x80040203

// Can be thrown by providers to indicate they
// explicitly don't support a pattern or property
#define UIA_E_NOTSUPPORTED           0x80040204

// The following are COR error codes, included here as a convenience
// (equivalent codes are in <corerror.h>)
#define UIA_E_INVALIDOPERATION       0x80131509 // COR_E_INVALIDOPERATION
#define UIA_E_TIMEOUT                0x80131505 // COR_E_TIMEOUT
