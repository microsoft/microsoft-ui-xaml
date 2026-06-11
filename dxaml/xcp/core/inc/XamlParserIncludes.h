// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// container classes
#include "xvector.h"
#include "xlist.h"
#include "xstack.h"
#include "xqueue.h"
#include "xmap.h"
#include <map>
#include <array>

#include "ctypes.h"

#include "ParserTypeDefs.h"

#include "XamlTypeTokens.h"

#include "XStringBuilder.h"
#include "XamlTypeInfoProvider.h"
#include "NativeTypeInfoProvider.h"
#include "ManagedTypeInfoProvider.h"
#include "XamlQualifiedObject.h"
#include "XamlServiceProviderContext.h"
#include "XamlTextSyntax.h"
#include "XamlRuntime.h"
#include "XamlManagedRuntime.h"
#include "XamlNativeRuntime.h"
#include "XamlAssembly.h"
#include "LineInfo.h"
#include "ParserErrorService.h"
#include "IXamlSchemaObject.h"

#include "INamescope.h"
#include "XamlName.h"
#include "XamlPropertyName.h"
#include "XamlQualifiedName.h"
#include "XamlTypeName.h"
#include "XamlText.h"
#include "XamlScannerNode.h"
#include "XamlScannerStack.h"
#include "XamlContext.h"
#include "SavedContext.h"
#include "XamlParserContext.h"
#include "XamlTextReaderSettings.h"
#include "XamlSortedAttributes.h"
#include "XamlScanner.h"
#include "XamlNode.h"
#include "XamlParserState.h"
#include "MeScanner.h"
#include "XamlNodeStreamValidator.h"
#include "MePullParser.h"
#include "XamlPullParser.h"
#include "XamlReader.h"
#include "XamlWriter.h"
#include "XamlOptimizedNodeList.h"
#include "TemplateNamescope.h"
#include "ObjectWriterSettings.h"
#include "DeferringWriter.h"
#include "XamlTextReader.h"
#include "UriXStringGetters.h"
#include "XamlBinaryMetadata.h"
