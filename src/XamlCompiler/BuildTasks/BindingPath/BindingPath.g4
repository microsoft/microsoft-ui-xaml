// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

grammar BindingPath;

import CSharpIdentifier; // defines lexer token IDENTIFIER

/*
To Update:
1) install java (on a VM)
2) download antlr-4.5.1-complete.jar from http://www.antlr.org/download.html
3) place antlr-4.5.1-complete.jar and this g4 file in the same (temp) folder
4) open a cmd window, cd to the folder then run this command
java -jar antlr-4.5.1-complete.jar -Dlanguage=CSharp BindingPath.g4
5) overwrite existing cs files with generated ones
*/

program : path EOF;

decimal_value
  : '-'? Digits ('.' Digits)?
  ;

boolean_value
  : TRUE
  | FALSE
  ;

namespace_qualifier
  : IDENTIFIER ':'  // sys:
  ;

static_type
  : namespace_qualifier IDENTIFIER  // sys:DateTime
  ;

attached_expr
  : '(' static_type '.' IDENTIFIER ')'  // foo.(sys:String.Length)
  | '(' IDENTIFIER '.' IDENTIFIER ')'   // foo.(Button.Length)
  ;

cast_expr
  : '(' static_type ')'
  | '(' IDENTIFIER ')'
  ;

function
  : IDENTIFIER '(' function_param? (',' function_param )* ')'   // foo(), foo(a) or foo(a, b, c)
  ;

path returns [Microsoft.UI.Xaml.Markup.Compiler.BindPathStep PathStep]
  : IDENTIFIER                  #PathIdentifier         // foo
  | path '.' IDENTIFIER         #PathDotIdentifier      // foo.bar
  | static_type '.' IDENTIFIER  #PathStaticIdentifier   // sys:DateTime.Today - needed to be able to get to static types / values

  | path '[' Digits ']'         #PathIndexer            // foo[indexer]
  | path '[' QuotedString ']'   #PathStringIndexer      // foo["indexer"]

  | path '.' attached_expr      #PathDotAttached        // foo.(sys:String.Length) - converts foo to a string then gets the length
  | attached_expr               #PathCastInvalid        // (Grid.Row) - cast beginning with an attached property error message
  | cast_expr                   #PathCast               // (local:foo) or (Button) - no path, just the cast
  | cast_expr path              #PathCastPath           // (string)foo.bar - casts the leaf of the expr
  | '(' cast_expr path ')'      #PathCastPathParen      // ((string)foo.bar) - casts the leaf of the expr, usable for sub sections of the path

  | function                    #PathFunction           // foo(), foo(a) or foo(a, b, c)
  | path '.' function           #PathPathToFunction     // path.foo(...)
  | static_type '.' function    #PathStaticFuction      // ns:Type.func(...)
  ;

function_param returns [Microsoft.UI.Xaml.Markup.Compiler.FunctionParam Param]
  : function                    #FunctionParameterInvalid
  | path                        #FunctionParamPath
  | boolean_value               #FunctionParamBool
  | decimal_value               #FunctionParamNumber
  | QuotedString                #FunctionParamString
  | NULL                        #FunctionParamNullValue
  ;

/* Lexer Rules */

WS :            [ \t]+ -> skip;
ESCAPEDQUOTE:   ( '^"' | '^\'' );
QUOTE :         '\'';
DOUBLE_QUOTE :  '"';
TRUE:           'x:True';
FALSE:          'x:False';
NULL:           'x:Null';

Digits
  : Decimal_digit_character+
  ;

QuotedString
  : QUOTE (ESCAPEDQUOTE|.)*? QUOTE
  | DOUBLE_QUOTE (ESCAPEDQUOTE|.)*? DOUBLE_QUOTE
  ;