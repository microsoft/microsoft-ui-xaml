// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

grammar ConditionalNamespace;
options { 
	language=CSharp; 
}

import CSharpIdentifier; // defines lexer token IDENTIFIER

/*
To Update:
1) install java (on a VM)
2) download antlr-4.5.1-complete.jar from http://www.antlr.org/download.html
3) place antlr-4.5.1-complete.jar and this g4 file in the same (temp) folder
4) open a cmd window, cd to the folder then run this command
java -jar "x:\Work Folders\_dev\antlr-4.5.3-complete.jar" ConditionalNamespace.g4
5) overwrite existing cs files with generated ones

Ex: http://schemas.microsoft.com/winfx/2006/xaml?IsApiContractPresent(Windows.Foundation.UniversalApiContract,3,0)
*/

program : expression EOF;

 expression returns [Microsoft.UI.Xaml.Markup.Compiler.ApiInformation ApiInformation, Microsoft.UI.Xaml.Markup.Compiler.Platform TargetPlatform]
  : uri '?' query_string
  ;

uri
  : (. | '/' | ':' | '-' | '.')*?
  ;

unquoted_namespace
  : IDENTIFIER ( '.' IDENTIFIER )*
  ;

api_information returns [Microsoft.UI.Xaml.Markup.Compiler.ApiInformation ApiInformation]
  : IDENTIFIER '(' function_param (',' function_param )* ')' // foo(), foo(a) or foo(a, b, c)
  ;

function_param returns [Microsoft.UI.Xaml.Markup.Compiler.ApiInformationParameter ApiInformationParameter]
  : unquoted_namespace
  | IDENTIFIER
  | QuotedString
  | Digits
  ;

 target_platform_value
  : PlatformUWP
  | PlatformiOS
  | PlatformAndroid
  ;

 query_string returns [Microsoft.UI.Xaml.Markup.Compiler.ApiInformation ApiInformation, Microsoft.UI.Xaml.Markup.Compiler.Platform TargetPlatform]
  : query_string_component (';' query_string_component)*
  ;

 query_string_component returns [Microsoft.UI.Xaml.Markup.Compiler.ApiInformation ApiInformation, Microsoft.UI.Xaml.Markup.Compiler.Platform TargetPlatform]
  : target_platform_func #QueryStringTargetPlatform
  | api_information #QueryStringApiInformation 
  ;

 target_platform_func returns [Microsoft.UI.Xaml.Markup.Compiler.Platform TargetPlatform]
  : TargetPlatformString '(' target_platform_value ')'
  ;

/* Lexer Rules */

WS :            [ \t]+ -> skip;
ESCAPEDQUOTE:   ( '^"' | '^\'' );
QUOTE :         '\'';
DOUBLE_QUOTE :  '"';

TargetPlatformString: 'TargetPlatform';
PlatformUWP: 'UWP';
PlatformiOS: 'iOS';
PlatformAndroid: 'Android';

Digits
  : Decimal_digit_character+
  ;

QuotedString
  : QUOTE (ESCAPEDQUOTE|.)*? QUOTE
  | DOUBLE_QUOTE (ESCAPEDQUOTE|.)*? DOUBLE_QUOTE
  ;