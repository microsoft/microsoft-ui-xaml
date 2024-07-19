// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "QualifierFactory.h"
#include "MinWidthQualifier.h"
#include "MinHeightQualifier.h"
#include "MultiQualifier.h"
#include "ExtensibleQualifier.h"

std::shared_ptr<IQualifier> QualifierFactory::Create(_In_ QualifierFlags flags, _In_ const XINT32& value)
{
    if((flags & QualifierFlags::Width) != QualifierFlags::None)
    {
        return std::make_shared<MinWidthQualifier>(value);
    }
    else if((flags & QualifierFlags::Height) != QualifierFlags::None)
    {
        return std::make_shared<MinHeightQualifier>(value);
    }

    return nullptr;
};

std::shared_ptr<IQualifier> QualifierFactory::Create(
                _In_ const int& width,
                _In_ const int& height)
{
    std::shared_ptr<IQualifier> pQualifier;

    if(width >= 0)
    {
        pQualifier = std::make_shared<MinWidthQualifier>(width);
    }

    if(height >= 0)
    {
        if(pQualifier)
        {
            std::shared_ptr<MultiQualifier> multiQualifier = std::make_shared<MultiQualifier>();
            multiQualifier->Add(std::make_shared<MinHeightQualifier>(height));
            multiQualifier->Add(pQualifier);

            return multiQualifier;
        }

        pQualifier = std::make_shared<MinHeightQualifier>(height);
    }

    if(!pQualifier)
    {
        pQualifier = std::make_shared<MultiQualifier>();
    }

    return pQualifier;
};

std::shared_ptr<IQualifier> QualifierFactory::Create(_In_ const std::vector<int>& qualifier)
{
    size_t length = qualifier.size();
    std::shared_ptr<IQualifier> pQualifier;

    if(length > 2)
    {
        // create a multiqualifier
        std::shared_ptr<MultiQualifier> pMultiQualifier = std::make_shared<MultiQualifier>();

        // Add these items to the multiqualifier
        for(size_t i = 0; i < length; i+=2)
        {
            switch(static_cast<QualifierFlags>(qualifier[i]))
            {
                case QualifierFlags::Width:
                case QualifierFlags::Height:
                    pQualifier = QualifierFactory::Create(static_cast<QualifierFlags>(qualifier[i]),
                        static_cast<XINT32>(qualifier[i+1]));
                    pMultiQualifier->Add(pQualifier);
                break;
            }
        }

        pQualifier = pMultiQualifier;
    }
    else if (length == 0)
    {
        // AdaptiveTrigger was specified without attributes
        pQualifier = std::make_shared<MultiQualifier>();
    }
    else
    {
        // create a single qualifier
        switch(static_cast<QualifierFlags>(qualifier[0]))
        {
            case QualifierFlags::Width:
            case QualifierFlags::Height:
                pQualifier = QualifierFactory::Create(static_cast<QualifierFlags>(qualifier[0]),
                static_cast<XINT32>(qualifier[1]));
            break;
        }
    }

    return pQualifier;
}


std::shared_ptr<IQualifier> QualifierFactory::Create(
                _In_ const bool* pBool)
{
    return std::make_shared<ExtensibleQualifier>(pBool);
};
