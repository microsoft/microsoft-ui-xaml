// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define USE_SHARED_PTR_POOLED_ALLOCATOR 1

#include <minxcptypes.h>
#include <minerror.h>
#include <palcore.h>

#include <IParserCoreServices.h>

class XamlSchemaContext;
struct IErrorService;
class XamlNodeStreamCacheManager;
class ParserErrorReporter;
class XamlNode;
struct IPALStream;
struct IPALMemory;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class ParserUtilities
    {
    public:
        std::shared_ptr<XamlSchemaContext> GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter);        
        std::vector<XamlNode> ParseTextXaml(const xstring_ptr& strXamlFragment);
        IParserCoreServices* GetMockParserServices();
        xref_ptr<IPALStream> GetPALStreamFromPALMemory(_In_ xref_ptr<IPALMemory>& palMemory);
        xref_ptr<IPALMemory> GetPALMemoryFromBuffer(_In_ uint32_t cBuffer, _In_ uint8_t* buffer);
    };

    class SimpleMemoryBuffer : public IPALMemory
    {
    public:
        SimpleMemoryBuffer(_In_ uint8_t* bufferPtrArg, _In_ uint32_t bufferSizeArg) 
          : bufferSize(bufferSizeArg)
          , bufferPtr(bufferPtrArg)
          , refCount(1)
        {
        }

        // IPALMemory implementation
        uint32_t AddRef()  const override 
        { 
            return ++refCount; 
        }

        uint32_t Release() const override 
        { 
            uint32_t newRefCount = --refCount; 
            if (refCount == 0) 
            {
                delete this; 
            }
            return newRefCount;
        }

        uint32_t GetSize() const override { return bufferSize; }
        void* GetAddress() const override { return bufferPtr; }

    private:
        uint32_t bufferSize;
        uint8_t* bufferPtr;
        mutable uint32_t refCount;
    };

} } } } }

