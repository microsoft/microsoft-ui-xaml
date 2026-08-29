// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WinBluePropertyTypeCompatHelper.h"

// This helper class is used to fix MSFT: 3493208 - Bug 3493208:BARCHECK-[Compat] Incorrect metadata crashes Win81 app when compiled from Win10 machine
// In TH we added new ContentPresenter.Padding, ContentPresenter.HorizontalContentAlignment, ContentPresenter.VerticalContentAlignment properties as part of the the Border chrome work.
// Due to a complier bug, this conflicted with existing properties GridViewItemPresenter.HorizontalContentAlignment, etc (GridViewItemPresenter derives from ContentPresenter)
// So we had to rename the GridViewItemPresenter.HorizontalContentAlignment to GridViewItemPresenter.GridViewItemPresenterHorizontalContentAlignment and kept it the same slot in the IDL.
// In that way, we don't have app compat issues (existing windows 8.1 app binaries continues to run on TH without issues).
//
// However, there is a issue when compling an 8.1 XAML app on Windows 10. 
// When the xaml script is compiled, it is parsed into a binary format (XBF), For example when the following xaml is compiled on Windows 10:
//
// <GridViewItemPresenter x:Name="gridViewItemPresenter" HorizontalContentAlignment="Right"/>
//
// Since GridViewItemPresenter.HorizontalContentAlignment no longer exists on Windows 10,
// the type will be resolved to ContentPresenter and stored in the XBF.
// Later when this XBF is loaded on a 8.1 machine,it will try to set HorizontalContentAlignment property on the ContentPresenter,
// which doesn't exist. Then the app crashed.
// This helper class is used when parsing (generate xbf1) the xaml script, 
// we explicitly detect the above scenario and make sure the type is adujsted to GridViewItemPresenter/ListViewItemPresenter in the XBF. 


using namespace Parser;


void WinBluePropertyTypeCompatHelper::IncreaseScopeDepth(_In_ const std::shared_ptr<XamlType>& currentXamlObjectType)
{
    // Some properties on GridViewItemPresenter/ListViewItemPresenter have been renamed in TH, their type information should be cached, when they are in scope.
    switch(currentXamlObjectType->get_TypeToken().GetHandle())
    {
        case KnownTypeIndex::GridViewItemPresenter:
        case KnownTypeIndex::ListViewItemPresenter:
        {
            m_spCurrentXamlObjectType = currentXamlObjectType;
            ASSERT(m_scopeDepth == 0);
            break;
        }
    }
    if (m_spCurrentXamlObjectType)
    {
        m_scopeDepth++;
    }
}

void WinBluePropertyTypeCompatHelper::DecreaseScopeDepth()
{
    if (m_spCurrentXamlObjectType)
    {
        m_scopeDepth--;
        if (m_scopeDepth == 0)
        {
            m_spCurrentXamlObjectType.reset();
        }
    }
}

// This method is used to detect whether we are having compat issues and should adjust the object type to be the original object type. 
// For now we only expect to ajust for GridViewItemPresenter and ListViewItemPresenter.
bool WinBluePropertyTypeCompatHelper::ShouldUseBlueMappingForObjectType(_In_ const std::shared_ptr<XamlProperty>& inXamlProperty) const
{
   bool result = false;
   if (m_spCurrentXamlObjectType)
   {
       ASSERT (m_spCurrentXamlObjectType->get_TypeToken().GetHandle() == KnownTypeIndex::GridViewItemPresenter
           || m_spCurrentXamlObjectType->get_TypeToken().GetHandle() == KnownTypeIndex::ListViewItemPresenter);

       switch(inXamlProperty->get_PropertyToken().GetHandle())
       {
           case KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment:
           case KnownPropertyIndex::ContentPresenter_VerticalContentAlignment:
           case KnownPropertyIndex::ContentPresenter_Padding:
               result = true;
               break;
       }
   }
   return result;
}
const std::shared_ptr<XamlType>& WinBluePropertyTypeCompatHelper::GetWinBlueDeclaringTypeForProperty(_In_ const std::shared_ptr<XamlProperty>& inXamlProperty, _In_ const std::shared_ptr<XamlType>& inXamlType) const
{
    if (ShouldUseBlueMappingForObjectType(inXamlProperty))
    {
        return m_spCurrentXamlObjectType;
    }
    else
    {
        return inXamlType;
    }
}

