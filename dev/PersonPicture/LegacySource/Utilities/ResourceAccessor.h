// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace People { namespace Controls
{
    /// <summary>
    /// Resource Accessor
    /// </summary>
    public ref class ResourceAccessor sealed
    {
    private:
        /// <summary>
        /// Declare a private constructor to prevent instance creation.
        /// </summary>
        ResourceAccessor();

        /// <summary>
        /// The suffix for plural strings relating to quantities
        /// containing a single item.
        /// </summary>
        static Platform::String^ s_singularSuffix;

        /// <summary>
        /// The suffix for plural strings relating to quantities
        /// greater than twenty that end in one.
        /// </summary>
        static Platform::String^ s_plural1Suffix;

        /// <summary>
        /// The suffix for plural strings relating to quantities of
        /// three or four items.
        /// </summary>
        static Platform::String^ s_plural2Suffix;

        /// <summary>
        /// The suffix for plural strings relating to quantities 
        /// than twenty that end in two, three, or four.
        /// </summary>
        static Platform::String^ s_plural3Suffix;

        /// <summary>
        /// The suffix for plural strings relating to all quantities
        /// not covered by the others.
        /// </summary>
        static Platform::String^ s_plural4Suffix;

        /// <summary>
        /// The suffix for plural strings relating to quantities of
        /// five to ten items.
        /// </summary>
        static Platform::String^ s_plural5Suffix;

        /// <summary>
        /// The suffix for plural strings relating to quantities of
        /// eleven to nineteen items.
        /// </summary>
        static Platform::String^ s_plural6Suffix;

        /// <summary>
        /// The suffix for plural strings relating to quantities of
        /// two items.
        /// </summary>
        static Platform::String^ s_plural7Suffix;

        /// <summary>
        /// The resource loader.
        /// </summary>
        static Windows::ApplicationModel::Resources::IResourceLoader^ s_resourceLoader;

        /// <summary>
        /// String containing the resource location
        /// </summary>
        static const Platform::StringReference c_resourceLoc;

        /// <summary>
        /// Gets the full resource name for a plural resource string based on the given 
        /// resourceNamePrefix and numericValue.
        /// </summary>
        /// <param name="resourceNamePrefix">The prefix of the unique-identifier / name of the plural string resource.</param>
        /// <param name="numericValue">The value for which a numeric (plural/singular) resource is required.</param>
        /// <returns>The full resource name for the desired plural resource string.</returns>
        static Platform::String^ _GetPluralResourceName(Platform::String^ resourceNamePrefix, unsigned int numericValue);

    public:
        /// <summary>
        /// Returns the localized string value for the given resourceName
        /// </summary>
        /// <param name="resourceName">The unique-identifier / name of the string resource</param>
        /// <returns>A localized string</returns>
        static Platform::String^ GetLocalizedStringResource(Platform::String^ resourceName);

        /// <summary>
        /// Returns the localized plural string value for the given resourceName and numericValue
        /// </summary>
        /// <remarks>
        /// <para>
        /// Different languages have different rules for how singular and plural forms of nouns are written.
        /// There are a total of 7 plural forms and one singular form required in order to cover all of the different cultures.
        /// </para>
        /// <para>
        /// This method provides the correct resource key for a specified value.
        /// When calling this method, specify the prefix of your resource key (e.g. "PluralString_XMonthsAgoFormat")
        /// and the actual number the UX is about to present (non-negative). This method will compose the correct
        /// resource key and fetch the desired value from the resources.
        /// </para>
        /// </remarks>
        /// <param name="resourceNamePrefix">The prefix of the unique-identifier / name of the plural string resource.</param>
        /// <param name="numericValue">The value for which a numeric (plural/singular) resource is required.</param>
        /// <returns>A localized plural string</returns>
        static Platform::String^ GetLocalizedPluralStringResource(
            Platform::String^ resourceNamePrefix,
            uint32 numericValue);
    };
}}} // namespace Microsoft { namespace People { namespace Controls