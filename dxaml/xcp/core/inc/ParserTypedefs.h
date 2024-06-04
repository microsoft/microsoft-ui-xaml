// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <stack_vector.h>
#include <vector_map.h>
#include <xref_ptr.h>

class XamlProperty;
struct XamlQualifiedObject;
class XamlNamespace;
class ObjectWriterStack;
class CDependencyObject;

template<typename T, typename U> class xpairlist;
typedef xpairlist<std::shared_ptr<XamlProperty>, std::shared_ptr<XamlQualifiedObject> > MapPropertyToQO;
typedef std::shared_ptr< MapPropertyToQO > SP_MapPropertyToQO;

typedef std::shared_ptr< ObjectWriterStack > SP_ObjectWriterStack;

typedef std::shared_ptr< containers::vector_map<xstring_ptr, std::shared_ptr<XamlNamespace>> > tNamespaceMap;

typedef Jupiter::stack_vector<xref_ptr<CDependencyObject>, 8> AmbientValuesVector;
