// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ObjectWriterNodeList;
class XamlBinaryFormatSubReader2;

// Contains everything the XBFv2 encoder needs to save into the
// XBFv2 file format to enable deferred object creation.
class SubObjectWriterResult
{
public:
    SubObjectWriterResult(
        std::shared_ptr<ObjectWriterNodeList> nodeList)
        : m_nodeList(std::move(nodeList))
    {}

    SubObjectWriterResult(
        std::shared_ptr<XamlBinaryFormatSubReader2> spSubReader)
        : m_spSubReader(std::move(spSubReader))
    {}

    const std::shared_ptr<ObjectWriterNodeList>& GetNodeList() const { return m_nodeList; }

    const std::shared_ptr<XamlBinaryFormatSubReader2>& GetSubReader() const { return m_spSubReader;  }

private:
    std::shared_ptr<ObjectWriterNodeList> m_nodeList;
    std::shared_ptr<XamlBinaryFormatSubReader2> m_spSubReader;
};

