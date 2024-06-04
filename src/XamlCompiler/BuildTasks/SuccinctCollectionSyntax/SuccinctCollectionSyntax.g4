// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

grammar SuccinctCollectionSyntax;

/*
To Update:
1) install java (on a VM)
2) download antlr-4.5.1-complete.jar from http://www.antlr.org/download.html
3) place antlr-4.5.1-complete.jar and this g4 file in the same (temp) folder
4) open a cmd window, cd to the folder then run this command
java -jar antlr-4.5.1-complete.jar -Dlanguage=CSharp SuccinctCollectionSyntax.g4
5) overwrite existing cs files with generated ones
*/


program 
  : items EOF
  ;

items
  : item z
  ;

z
  : (COMMA item z)?
  ;

item
  : text
  | SINGLE_QUOTE literal_text SINGLE_QUOTE
  | DOUBLE_QUOTE literal_text DOUBLE_QUOTE
  ;

text
  : (CHARACTER text)?
  | BACKSLASH sequence text 
  ;

literal_text
  : (CHARACTER literal_text)?
  | (BACKSLASH sequence literal_text)
  | (RESERVED_SYMBOL literal_text)
  | (COMMA literal_text)
  ; 

sequence
  : CHARACTER
  | RESERVED_SYMBOL
  | QUOTE
  | COMMA
  | BACKSLASH 
  ;

/* Lexer Rules */

SINGLE_QUOTE : '\'';
DOUBLE_QUOTE : '"';
OPEN_SQUARE_BRACE : '[';
CLOSE_SQUARE_BRACE : ']';
CHARACTER : ~[,\[\]\'\"\\];
COMMA: ',';
BACKSLASH: '\\';
RESERVED_SYMBOL : [\[\]]; 
QUOTE : [\'\"];
