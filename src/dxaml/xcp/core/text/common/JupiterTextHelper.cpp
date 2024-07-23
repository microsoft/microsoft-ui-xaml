// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "JupiterTextHelper.h"
#include <TextSelectionManager.h>
#include <RichTextBlockView.h>
#include <TextBoxView.h>
#include <TextBoxBase.h>

using namespace Microsoft::WRL;
using namespace RichTextServices;

HRESULT JupiterTextHelper::GetGripperData(
    _In_  void*                element,
    _Out_ JupiterGripperData*  data)
{
    HRESULT hr = S_OK;
    CTextBlock*              publicTextBlock = NULL;
    CRichTextBlock*          publicRichTextBlock = NULL;
    CRichTextBlockOverflow*  richTextBlockOverflow = NULL;
    CDependencyObject*       dependencyObj = NULL;
    TextSelectionManager*    selectionManager = NULL;

    dependencyObj = reinterpret_cast<CDependencyObject *>(element);
    IFCPTR(dependencyObj);

    switch (dependencyObj->GetTypeIndex())
    {
    case KnownTypeIndex::RichTextBlock:
        publicRichTextBlock = reinterpret_cast<CRichTextBlock *>(dependencyObj);
        IFCPTR(publicRichTextBlock);

        selectionManager = reinterpret_cast<TextSelectionManager *>(publicRichTextBlock->m_pSelectionManager);
        IFCPTR(selectionManager);

        IFC(SetGripperData(selectionManager, data));

        break;

    case KnownTypeIndex::RichTextBlockOverflow:
        richTextBlockOverflow = reinterpret_cast<CRichTextBlockOverflow *>(dependencyObj);
        IFCPTR(richTextBlockOverflow);

        publicRichTextBlock = reinterpret_cast<CRichTextBlock *>(richTextBlockOverflow->GetMaster());
        IFCPTR(publicRichTextBlock);

        selectionManager = reinterpret_cast<TextSelectionManager *>(publicRichTextBlock->m_pSelectionManager);
        IFCPTR(selectionManager);

        IFC(SetGripperData(selectionManager, data));

    case KnownTypeIndex::TextBlock:
        publicTextBlock = reinterpret_cast<CTextBlock *>(dependencyObj);
        IFCPTR(publicTextBlock);

        selectionManager = reinterpret_cast<TextSelectionManager *>(publicTextBlock->m_pSelectionManager);
        IFCPTR(selectionManager);

        IFC(SetGripperData(selectionManager, data));

        break;

        // RichEdit is not supported right now
    case KnownTypeIndex::TextBox:
    case KnownTypeIndex::PasswordBox:
    case KnownTypeIndex::RichEditBox:
    default:
        hr = E_NOTIMPL;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT JupiterTextHelper::SetGripperData(
    _In_ TextSelectionManager* selectionManager,
    _Out_ JupiterGripperData*  data)
{
    CTextSelectionGripper* gripperStart = NULL;
    CTextSelectionGripper* gripperEnd = NULL;

    gripperStart = reinterpret_cast<CTextSelectionGripper *>(selectionManager->m_pGripperElementStart);
    IFCPTR_RETURN(gripperStart);
    gripperEnd = reinterpret_cast<CTextSelectionGripper *>(selectionManager->m_pGripperElementEnd);
    IFCPTR_RETURN(gripperEnd);

    ((data->Start).CenterLocalCoordinate).X = (gripperStart->m_centerLocalCoordinate).x;
    ((data->Start).CenterLocalCoordinate).Y = (gripperStart->m_centerLocalCoordinate).y;

    ((data->End).CenterLocalCoordinate).X = (gripperEnd->m_centerLocalCoordinate).x;
    ((data->End).CenterLocalCoordinate).Y = (gripperEnd->m_centerLocalCoordinate).y;

    return S_OK;
}
