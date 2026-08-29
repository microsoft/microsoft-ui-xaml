/* Copyright (c) Microsoft Corporation.  All rights reserved. */

grammar XbfGrammarParser;

options 
{ 
    language = CSharp;
    tokenVocab = XbfGrammarLexer;
}

@header
{

}

@parser::members
{
    internal struct Version 
    { 
        internal uint Major; 
        internal uint Minor; 
    }

    internal Version XbfVersion { get; set; }
    internal bool ShouldNullTerminateStrings() { return XbfVersion.Major == 2 && XbfVersion.Minor >= 1; }
    internal bool HasLineInfo { get; set; } = false;

    // Converts the token of a BYTE terminal and returns the integer value
    internal uint GetByteValue(IToken token)
    {
        if (string.IsNullOrEmpty(token.Text))
        {
            throw new ArgumentException("token");
        }
        return Convert.ToUInt32(token.Text[0]);
    }
}

// Rules for various data types

// 16-bit unsigned integers are encoded as a series of 2 bytes in little endian order
uint16 returns [uint value]
    : b0 = BYTE b1 = BYTE
      { $value =   (GetByteValue($b0) << 0)
                 | (GetByteValue($b1) << 8); }
    ;

// 32-bit unsigned integers are encoded as a series of 4 bytes in little endian order
uint32 returns [uint value]
    : b0 = BYTE b1 = BYTE b2 = BYTE b3 = BYTE
      { $value =   (GetByteValue($b0) << 0)
                 | (GetByteValue($b1) << 8)
                 | (GetByteValue($b2) << 16)
                 | (GetByteValue($b3) << 24); }
    ;

// 64-bit unsigned integers are encoded as a series of 8 bytes in little endian order
uint64 returns [UInt64 value]
    : b0 = BYTE b1 = BYTE b2 = BYTE b3 = BYTE 
      b4 = BYTE b5 = BYTE b6 = BYTE b7 = BYTE
      { $value =   (GetByteValue($b0) << 0)
                 | (GetByteValue($b1) << 8)
                 | (GetByteValue($b2) << 16)
                 | (GetByteValue($b3) << 24)
                 | (GetByteValue($b4) << 32)
                 | (GetByteValue($b5) << 40)
                 | (GetByteValue($b6) << 48)
                 | (GetByteValue($b7) << 56); }
    ;

wchar returns [char value]
    : b0 = BYTE b1 = BYTE
      { $value = (char)((GetByteValue($b0) << 0) | (GetByteValue($b1) << 8)); }
    ;

// To store compressed 32-bit unsigned integers, XBF uses the LEB128 encoding scheme whereby 
// the integer is divided into 7-bit groups. One encoded byte is output for each group, from 
// the least significant to  the most significant group. Each encoded byte stores the group 
// in the 7 least significant bits, with the most significant bit of all but the final byte 
// set to '1'.
leb128encodedint returns [uint value, uint bytesRead]
    : result = leb128encodedint_helper[0, 0]
      { $value = $result.value; $bytesRead = $result.bytesRead; }
    ;
leb128encodedint_helper[uint initial_value, int initial_shift] returns [uint value, int shift, uint bytesRead]
    @init
    {
        $bytesRead = 0;
    }
    : group = BYTE { $value = $initial_value | ((GetByteValue($group) & 0x7F) << $initial_shift); $bytesRead++; } ({(GetByteValue($group) & 0x80) > 0}? leb128encodedint_helper[$value, $initial_shift + 7])*
    ;

// Strings are encoded as the length followed by a series of 16-bit Unicode characters (WCHARs)
// In versions >= 2.1, the null terminator is encoded along with the string
string returns [string value] locals [StringBuilder builder]
    : count = uint32 string_helper[$count.value] { $value = $string_helper.value; } {ShouldNullTerminateStrings()}? BYTE BYTE
    ;

string_helper[uint length] returns [String value] locals[StringBuilder builder, uint index = 0]
    @init
    {
        $builder = new StringBuilder(64);
    }
    @after
    {
        $value = $builder.ToString();
    }
    : ({$index < $length}? character = wchar { $builder.Append($character.value); $index++; })*
    ;

// Vectors are encoded with the item count followed by the items that make up its contents.
vector_string locals [uint i = 0]
    : count = uint32 ( {$i < $count.value}? strings += string { $i++; } )*
    ;
vector_xamlAssembly locals [uint i = 0]
    : count = uint32 ( {$i < $count.value}? assemblies += xamlAssembly { $i++; } )*
    ;
vector_xamlTypeNamespace locals [uint i = 0]
    : count = uint32 ( {$i < $count.value}? typeNamespace += xamlTypeNamespace { $i++; } )*
    ;
vector_xamlType locals [uint i = 0]
    : count = uint32 ( {$i < $count.value}? types += xamlType { $i++; } )*
    ;
vector_xamlProperty locals [uint i = 0]
    : count = uint32 ( {$i < $count.value}? properties += xamlProperty { $i++; } )*
    ;
vector_xamlXmlNamespaceList locals [uint i = 0]
    : count = uint32 ( {$i < $count.value}? xmlNamespaces += xamlXmlNamespace { $i++; } )*
    ;

// Persisted metadata types
xamlAssembly
    : assemblyKind = uint32 assemblyNameId = uint32
    ;
xamlTypeNamespace
    : assemblyId = uint32 typeNamespaceNameId = uint32
    ;
xamlType
    : typeFlags = uint32 namespaceId = uint32 typeNameId = uint32
    ;
xamlProperty
    : propertyFlags = uint32 declaringTypeId = uint32 propertyNameId = uint32
    ;
xamlXmlNamespace
    : namespaceUriId = uint32
    ;

// Rules for various sections of the XBF file
document
    : header metadata substream_table substreams EOF
    ;

substreams locals[uint streamIndex = 1]
    : { HasLineInfo }? nodestream[0] linestream[0] ({ $streamIndex < 5 /* TODO: check against actual substream count */}? nodestream[$streamIndex] linestream[$streamIndex] { $streamIndex++; } )+
    | nodestream[0] ({ $streamIndex < 5 /* TODO: check against actual substream count */}? nodestream[$streamIndex] { $streamIndex++; } )+
    ;

header
    : xbf_magic_number metadata_size = uint32 nodestream_size = uint32 format_version
    ;

// Listener should validate that this is the sequence '\u0058' '\u0042' '\u0046' '\u0000' i.e. the null-terminated ASCII string "XBF"
xbf_magic_number
    : BYTE BYTE BYTE BYTE   # ReadXbfMagicNumber
    ;

format_version
    : major = uint32 minor = uint32
     { XbfVersion = new Version() { Major = $major.value, Minor = $minor.value }; }
    ;

metadata
    : metadata_header 
      stringTable = vector_string assemblyList = vector_xamlAssembly 
      typeNamespaceList = vector_xamlTypeNamespace typeList = vector_xamlType
      propertyList = vector_xamlProperty xmlNamespaceList = vector_xamlXmlNamespaceList
    ;

metadata_header
    : stringTableOffset = uint64 assemblyListOffset = uint64 
      typeNamespaceListOffset = uint64 typeListOffset = uint64 
      propertyListOffset = uint64 xmlNamespaceListOffset = uint64 
      hash = xbf_hash
    ;

// The XBF hash is an array of 64 bytes
xbf_hash
    : BYTE BYTE BYTE BYTE BYTE BYTE BYTE BYTE
      BYTE BYTE BYTE BYTE BYTE BYTE BYTE BYTE
      BYTE BYTE BYTE BYTE BYTE BYTE BYTE BYTE
      BYTE BYTE BYTE BYTE BYTE BYTE BYTE BYTE
      BYTE BYTE BYTE BYTE BYTE BYTE BYTE BYTE
      BYTE BYTE BYTE BYTE BYTE BYTE BYTE BYTE
      BYTE BYTE BYTE BYTE BYTE BYTE BYTE BYTE
      BYTE BYTE BYTE BYTE BYTE BYTE BYTE BYTE
    ;

// Vector of substream_info
substream_table locals [uint i = 0]
    : count = uint32 ( {$i < $count.value}? substream_info { $i++; } )*
    ;

substream_info
    : nodeStreamOffset = uint32 lineStreamOffset = uint32
    ;

nodestream[uint streamIndex] locals [uint bytesRead = 0]
    : node0 = node { $bytesRead += $node0.bytesRead; } ( { true /* TODO: look up nodestream size and compare against bytesRead */ }? nextNode = node { $bytesRead += $nextNode.bytesRead; })*
    ;

node returns[uint bytesRead]
    : node_PushScope { $bytesRead = $node_PushScope.bytesRead; }
    | node_PopScope { $bytesRead = $node_PopScope.bytesRead; }
    | node_AddNamespace { $bytesRead = $node_AddNamespace.bytesRead; }
    | node_PushConstant { $bytesRead = $node_PushConstant.bytesRead; }
    | node_SetValue { $bytesRead = $node_SetValue.bytesRead; }
    | node_SetValueFromMarkupExtension { $bytesRead = $node_SetValueFromMarkupExtension.bytesRead; }
    | node_AddToCollection { $bytesRead = $node_AddToCollection.bytesRead; }
    | node_AddToDictionary { $bytesRead = $node_AddToDictionary.bytesRead; }
    | node_AddToDictionaryWithKey { $bytesRead = $node_AddToDictionaryWithKey.bytesRead; }
    | node_CheckPeerType { $bytesRead = $node_CheckPeerType.bytesRead; }
    | node_SetConnectionId { $bytesRead = $node_SetConnectionId.bytesRead; }
    | node_SetName { $bytesRead = $node_SetName.bytesRead; }
    | node_GetResourcePropertyBag { $bytesRead = $node_GetResourcePropertyBag.bytesRead; }
    | node_ProvideValue { $bytesRead = $node_ProvideValue.bytesRead; }
    | node_SetDeferredProperty { $bytesRead = $node_SetDeferredProperty.bytesRead; }
    | node_SetCustomRuntimeData { $bytesRead = $node_SetCustomRuntimeData.bytesRead; }
    | node_PushScopeAddNamespace { $bytesRead = $node_PushScopeAddNamespace.bytesRead; }
    | node_PushScopeGetValue { $bytesRead = $node_PushScopeGetValue.bytesRead; }
    | node_PushScopeCreateTypeBeginInit { $bytesRead = $node_PushScopeCreateTypeBeginInit.bytesRead; }
    | node_PushScopeCreateTypeWithConstantBeginInit { $bytesRead = $node_PushScopeCreateTypeWithConstantBeginInit.bytesRead; }
    | node_PushScopeCreateTypeWithTypeConvertedConstantBeginInit { $bytesRead = $node_PushScopeCreateTypeWithTypeConvertedConstantBeginInit.bytesRead; }
    | node_CreateTypeBeginInit { $bytesRead = $node_CreateTypeBeginInit.bytesRead; }
    | node_CreateTypeWithConstantBeginInit { $bytesRead = $node_CreateTypeWithConstantBeginInit.bytesRead; }
    | node_CreateTypeWithTypeConvertedConstantBeginInit { $bytesRead = $node_CreateTypeWithTypeConvertedConstantBeginInit.bytesRead; }
    | node_SetValueConstant { $bytesRead = $node_SetValueConstant.bytesRead; }
    | node_SetValueTypeConvertedConstant { $bytesRead = $node_SetValueTypeConvertedConstant.bytesRead; }
    | node_SetValueTypeConvertedResolvedType { $bytesRead = $node_SetValueTypeConvertedResolvedType.bytesRead; }
    | node_SetValueTypeConvertedResolvedProperty { $bytesRead = $node_SetValueTypeConvertedResolvedProperty.bytesRead; }
    | node_ProvideStaticResourceValue { $bytesRead = $node_ProvideStaticResourceValue.bytesRead; }
    | node_SetValueFromStaticResource { $bytesRead = $node_SetValueFromStaticResource.bytesRead; }
    | node_ProvideThemeResourceValue { $bytesRead = $node_ProvideThemeResourceValue.bytesRead; }
    | node_SetValueFromThemeResource { $bytesRead = $node_SetValueFromThemeResource.bytesRead; }
    | node_SetValueFromTemplateBinding { $bytesRead = $node_SetValueFromTemplateBinding.bytesRead; }
    | node_EndInitPopScope { $bytesRead = $node_EndInitPopScope.bytesRead; }
    | node_BeginConditionalScope { $bytesRead = $node_BeginConditionalScope.bytesRead; }
    | node_EndConditionalScope { $bytesRead = $node_EndConditionalScope.bytesRead; }
    ;

node_PushScope returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0001'
    ;

node_PopScope returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0002'
    ;

node_AddNamespace returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0003' data = persistedXamlNode prefix = string { $bytesRead += $data.bytesRead + (uint)$prefix.value.Length; }
    ;

node_PushConstant returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0004'
    ;

node_SetValue returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0007'
    ;

node_SetValueFromMarkupExtension returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0020'
    ;

node_AddToCollection returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0008'
    ;

node_AddToDictionary returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0009'
    ;

node_AddToDictionaryWithKey returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u000A'
    ;

node_CheckPeerType returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u000B'
    ;

node_SetConnectionId returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u000C'
    ;

node_SetName returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u000D'
    ;

node_GetResourcePropertyBag returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u000E'
    ;

node_ProvideValue returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u008B'
    ;

node_SetDeferredProperty returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0011'
    ;

node_SetCustomRuntimeData returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u000F'
    ;

node_PushScopeAddNamespace returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0012'
    ;

node_PushScopeGetValue returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0013'
    ;

node_PushScopeCreateTypeBeginInit returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0014'
    ;

node_PushScopeCreateTypeWithConstantBeginInit returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0015'
    ;

node_PushScopeCreateTypeWithTypeConvertedConstantBeginInit returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0016'
    ;

node_CreateTypeBeginInit returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0017'
    ;

node_CreateTypeWithConstantBeginInit returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0018'
    ;

node_CreateTypeWithTypeConvertedConstantBeginInit returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0019'
    ;

node_SetValueConstant returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u001A'
    ;

node_SetValueTypeConvertedConstant returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u001B'
    ;

node_SetValueTypeConvertedResolvedType returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u001D'
    ;

node_SetValueTypeConvertedResolvedProperty returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u001C'
    ;

node_ProvideStaticResourceValue returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0022'
    ;

node_SetValueFromStaticResource returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u001E'
    ;

node_ProvideThemeResourceValue returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0023'
    ;

node_SetValueFromThemeResource returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0024'
    ;

node_SetValueFromTemplateBinding returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u001F'
    ;

node_EndInitPopScope returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0021'
    ;

node_BeginConditionalScope returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0026'
    ;

node_EndConditionalScope returns[uint bytesRead]
    @init
    {
        $bytesRead = 2; // Two bytes always read for node type
    }
    : '\u0027'
    ;

persistedXamlNode returns[uint bytesRead]
    : uint16 { $bytesRead = 2; }    # ReadPersistedXamlNode
    ;

linestream[uint streamIndex] locals [uint bytesRead = 0]
    : ( { true /* TODO: look up linestream size and compare against bytesRead */ }? streamOffsetDelta = leb128encodedint lineOffsetDelta = leb128encodedint columnOffsetDelta = leb128encodedint { $bytesRead += $streamOffsetDelta.bytesRead + $lineOffsetDelta.bytesRead + $columnOffsetDelta.bytesRead ; })*
    ;