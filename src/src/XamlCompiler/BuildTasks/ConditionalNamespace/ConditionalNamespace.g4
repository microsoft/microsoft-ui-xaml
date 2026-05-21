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
2) download antlr-4.5.3-complete.jar from http://www.antlr.org/download.html
3) place antlr-4.5.3-complete.jar and this g4 file in the same (temp) folder
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

api_information returns [Microsoft.UI.Xaml.Markup.Compiler.ApiInformation ApiInformation]
  : (IDENTIFIER ':' ) ? IDENTIFIER LPAREN function_param RPAREN // foo(any content as single string)
  ;

function_param returns [string Value]
  : ( ~(LPAREN | RPAREN) | LPAREN function_param RPAREN )*   // matches any tokens including balanced nested parens; Value is set by the listener
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
  : TargetPlatformString LPAREN target_platform_value RPAREN
  ;

/* Lexer Rules */

WS :            [ \t]+ -> skip;
ESCAPEDQUOTE:   ( '^"' | '^\'' );
QUOTE :         '\'';
DOUBLE_QUOTE :  '"';
LPAREN :        '(';
RPAREN :        ')';
COMMA :         ',';

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