// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.ApplicationModel.Search
{
    [Imported("searchui.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public interface ISearchQueryLinguisticDetails
    {
    }

    [Imported("searchui.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    [Implements(typeof(ISearchQueryLinguisticDetails))]
    public class SearchQueryLinguisticDetails
    {
    }

    [Imported("searchui.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class LocalContentSuggestionSettings
    {
    }
}
