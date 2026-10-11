// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewLocalizationTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 17 of the TableView test plan: localization and resource resolution.
    //
    // WHY THIS CATEGORY EXISTS, AND WHY IT CANNOT GO THROUGH THE AUTOMATION PEERS.
    // Every TableView string goes through LocalizedOrFallbackForTableViewAutomation or
    // FormatLocalizedOrFallback (TableViewAutomationHelpers.h), both of which swallow the lookup
    // failure and return a HARD-CODED ENGLISH LITERAL:
    //
    //     LocalizedOrFallbackForTableViewAutomation(SR_TableViewBooleanTrue, L"True")
    //     FormatLocalizedOrFallback(SR_TableViewCellNameFormat, L"%1!s!, %2!s!", ...)
    //
    // That fallback is correct product behaviour - a missing string must not blank an automation
    // name - but it means a peer-level assertion is BLIND to resource failure. A test that loads a
    // table and checks "the cell peer has a non-empty Name" passes identically whether the resource
    // map resolved or the control silently announced English to a Japanese screen-reader user.
    // Issue #12070 (TableView could not resolve its default strings from the Tabular resource map)
    // shipped for exactly that reason, and was fixed with no regression test.
    //
    // So this category asserts against the RESOURCE MAP ITSELF, through the same WinAppSDK
    // ResourceManager the product uses (ResourceAccessor.cpp:34-46). Section 17.4 then cross-checks
    // that what a peer announces is actually built from the resolved resource rather than from the
    // fallback - the one peer-level assertion that can tell the two apart.
    //
    // WHY NOT Windows.ApplicationModel.Resources.Core. LocalizationTests.cs:30 is [Ignore]d with
    // "Disabled after converting MUXControlsTestApp to a desktop .NET 5 app". That is the UWP
    // resource stack, which does not resolve in a desktop app. The product moved to
    // Microsoft.Windows.ApplicationModel.Resources; these tests follow it.
    //
    // NO ENGLISH LITERAL IS PINNED. TableView_AutomationPeer_APITests.cs:1283 states the rule -
    // "pinning English text turns the test into a localization tripwire". These tests assert
    // resolution, placeholder arity and cross-consistency, never a translated value.
    [TestClass]
    public class TableViewLocalizationTests : TableViewApiTestBase
    {
        #region 17.1 The resource map resolves every string the control asks for

        [TestMethod]
        [TestProperty("Description", "Verifies every SR_TableView* resource the control references is present in the resource map and resolves to a non-empty string.")]
        public void VerifyEveryTableViewResourceKeyResolves()
        {
            RunOnUIThread.Execute(() =>
            {
                var map = RequireTableViewResourceMap();
                var unresolved = new List<string>();

                foreach (var key in AllResourceKeys)
                {
                    var value = TryResolve(map, key);

                    if (string.IsNullOrWhiteSpace(value))
                    {
                        unresolved.Add(key);
                    }
                    else
                    {
                        Log.Comment($"{key} = '{value}'");
                    }
                }

                // The direct #12070 guard. If the Tabular resource subtree stops resolving, every key
                // fails here at once, where a peer-level test would see nothing at all.
                Verify.AreEqual(0, unresolved.Count,
                    $"Every TableView resource must resolve. Unresolved: {string.Join(", ", unresolved)}.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies no TableView resource resolves to its own key name, which is how a missing string surfaces as a plausible-looking value.")]
        public void VerifyNoResourceResolvesToItsOwnKeyName()
        {
            RunOnUIThread.Execute(() =>
            {
                var map = RequireTableViewResourceMap();

                foreach (var key in AllResourceKeys)
                {
                    var value = TryResolve(map, key);

                    // A resource stack that echoes the identifier back produces a string that is
                    // non-empty, so every other assertion in this file would accept it.
                    Verify.AreNotEqual(key, value,
                        $"{key} resolved to its own key name, which means the lookup failed rather than succeeded.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the control's resources live under a single resource subtree rather than being split across maps.")]
        public void VerifyAllResourcesShareOneSubtree()
        {
            RunOnUIThread.Execute(() =>
            {
                var subtrees = AllResourceKeys
                    .Select(key => SubtreeOf(FindResourcePath(key)))
                    .Where(path => path != null)
                    .Distinct()
                    .ToList();

                Log.Comment($"Subtrees in use: {string.Join(", ", subtrees)}");

                // #12070 was a subtree-resolution failure. A split across two subtrees means half the
                // strings are one build-flag change away from silently falling back to English.
                Verify.AreEqual(1, subtrees.Count,
                    $"All TableView resources must share one subtree; found {subtrees.Count}: {string.Join(", ", subtrees)}.");
            });
        }

        #endregion

        #region 17.2 Localized values exist for the locales the control ships

        [TestMethod]
        [TestProperty("Description", "Verifies every TableView resource resolves in each shipped locale rather than silently falling back to English.")]
        public void VerifyEveryResourceResolvesInEveryShippedLocale()
        {
            RunOnUIThread.Execute(() =>
            {
                var map = RequireTableViewResourceMap();
                var gaps = new List<string>();
                var resolvedSomewhere = 0;

                foreach (var language in RepresentativeLocales)
                {
                    foreach (var key in AllResourceKeys)
                    {
                        if (string.IsNullOrWhiteSpace(TryResolveForLanguage(map, key, language)))
                        {
                            gaps.Add($"{language}/{key}");
                        }
                        else
                        {
                            resolvedSomewhere++;
                        }
                    }
                }

                foreach (var gap in gaps)
                {
                    Log.Comment($"Unlocalized: {gap}");
                }

                // PRECONDITION. If the test app were packaged with its languages filtered down to the
                // default, every lookup above would miss and this test would report a localization
                // disaster that is really a packaging artefact. Requiring at least one localized hit
                // separates the two: no hits at all means the payload carries no translations, which is
                // a different bug and is reported as such rather than as 100+ missing strings.
                Verify.IsGreaterThan(resolvedSomewhere, 0,
                    "No TableView resource resolved in any non-default language. The test payload appears to carry no localized resources at all, so per-locale coverage cannot be judged.");

                // KNOWN GAP, ASSERTED AS A CEILING RATHER THAN ZERO.
                // Six automation strings - CellLocalizedControlType, CellTextSeparator, CellNameFormat,
                // GroupHeaderNameFormat, BooleanTrue and BooleanFalse - exist only in
                // Strings\en-US\Resources.resw and in none of the other 85 locale files, so a non-English
                // screen-reader user hears the English fallbacks for all six. That is a real defect, but it
                // cannot be fixed by a test change: the values have to come from the localization pipeline.
                //
                // This assertion locks the gap at its present size so it cannot GROW, and fails the moment
                // a new string ships without localization. Tighten the constants as the gaps close.
                var ceiling = MaxKnownUnlocalizedCount();

                Verify.IsLessThanOrEqual(gaps.Count, ceiling,
                    $"The set of unlocalized TableView strings must not grow beyond the {ceiling} known gaps. Found {gaps.Count}: {string.Join(", ", gaps)}.");

                if (gaps.Count < ceiling)
                {
                    Log.Comment($"Localization improved: {gaps.Count} gaps against a ceiling of {ceiling}. Tighten KnownUnlocalizedKeysPerLocale / KnownEmptyLocales.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies no shipped locale resolves zero TableView resources, which is how an entirely untranslated locale file goes unnoticed.")]
        public void VerifyNoShippedLocaleIsCompletelyUnlocalized()
        {
            RunOnUIThread.Execute(() =>
            {
                var map = RequireTableViewResourceMap();
                var empty = new List<string>();
                var anyResolved = false;

                foreach (var language in RepresentativeLocales)
                {
                    var resolved = AllResourceKeys.Count(key => !string.IsNullOrWhiteSpace(TryResolveForLanguage(map, key, language)));
                    Log.Comment($"{language}: {resolved}/{AllResourceKeys.Length} resources resolved.");

                    anyResolved |= resolved > 0;

                    if (resolved == 0)
                    {
                        empty.Add(language);
                    }
                }

                Verify.IsTrue(anyResolved,
                    "No TableView resource resolved in any non-default language, so an empty locale cannot be distinguished from an unlocalized payload.");

                // A per-key gap list hides a wholly empty locale, because it reads as a handful of
                // individually missing keys. That is not hypothetical: Strings\mi-NZ\Resources.resw was
                // checked in with zero data elements until localization update #12205 filled it. The
                // allowlist exists so a known case cannot fail CI; it is empty now, so the next locale
                // that ships empty fails here.
                var unexpected = empty.Except(KnownEmptyLocales, StringComparer.OrdinalIgnoreCase).ToList();

                Verify.AreEqual(0, unexpected.Count,
                    $"No shipped locale may be entirely unlocalized. Unexpectedly empty: {string.Join(", ", unexpected)}.");

                foreach (var fixedUp in KnownEmptyLocales.Except(empty, StringComparer.OrdinalIgnoreCase))
                {
                    // Not a failure: localizing a locale must never break the build. It does mean the
                    // allowlist is stale and should shrink.
                    Log.Comment($"{fixedUp} is no longer empty; remove it from KnownEmptyLocales.");
                }
            });
        }

        #endregion

        #region 17.3 Format strings survive translation

        [TestMethod]
        [TestProperty("Description", "Verifies every FormatString-style resource keeps the placeholder count the product substitutes into it.")]
        public void VerifyFormatStringPlaceholderArityIsPreserved()
        {
            RunOnUIThread.Execute(() =>
            {
                var map = RequireTableViewResourceMap();

                foreach (var format in FormatResources)
                {
                    var value = TryResolve(map, format.Key);
                    Verify.IsFalse(string.IsNullOrWhiteSpace(value), $"{format.Key} must resolve before its arity can be checked.");

                    var actual = CountPlaceholders(value, format.Style);
                    Log.Comment($"{format.Key} = '{value}' -> {actual} placeholder(s), expected {format.ExpectedPlaceholders}.");

                    // A translator who drops %2!s! does not produce an error: StringUtil::FormatString
                    // simply emits a string missing the value, so a cell announces its header and
                    // nothing else. Arity is the only machine-checkable half of a translated format.
                    Verify.AreEqual(format.ExpectedPlaceholders, actual,
                        $"{format.Key} must keep {format.ExpectedPlaceholders} placeholder(s); resolved value was '{value}'.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies translated format strings keep their placeholder arity in every shipped locale.")]
        public void VerifyFormatStringPlaceholderArityIsPreservedAcrossLocales()
        {
            RunOnUIThread.Execute(() =>
            {
                var map = RequireTableViewResourceMap();

                foreach (var language in RepresentativeLocales)
                {
                    foreach (var format in FormatResources)
                    {
                        var value = TryResolveForLanguage(map, format.Key, language);

                        // An absent translation is section 17.2's subject, not this test's.
                        if (string.IsNullOrWhiteSpace(value))
                        {
                            continue;
                        }

                        var actual = CountPlaceholders(value, format.Style);

                        Verify.AreEqual(format.ExpectedPlaceholders, actual,
                            $"{language}/{format.Key} must keep {format.ExpectedPlaceholders} placeholder(s); resolved value was '{value}'.");
                    }
                }
            });
        }

        #endregion

        #region 17.4 What the control announces comes from the resource, not the fallback

        [TestMethod]
        [TestProperty("Description", "Verifies a cell peer builds its name from the resolved CellNameFormat resource rather than from the hard-coded English fallback.")]
        public void VerifyCellPeerNameIsBuiltFromTheResolvedFormatResource()
        {
            TableView tableView = null;
            List<Person> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeItems();
                tableView = CreateTableView(items, headers: new[] { "Name", "Role" });
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var map = RequireTableViewResourceMap();
                var format = TryResolve(map, "TableViewCellNameFormat");
                Verify.IsFalse(string.IsNullOrWhiteSpace(format), "CellNameFormat must resolve for this test to mean anything.");

                // The cell peer is the surface that composes header text and value text through the
                // format resource (TableViewCellAutomationPeer.cpp:392).
                var peer = GetCellPeer(tableView, items[0], 0);
                Verify.IsNotNull(peer, "The fixture must produce a cell peer for the first cell.");

                var name = peer.GetName();
                Log.Comment($"Cell automation name: '{name}'; format resource: '{format}'.");

                Verify.IsFalse(string.IsNullOrWhiteSpace(name), "A realized cell must expose an automation name.");

                // The separator carried by the FORMAT is the part the fallback would differ in. Asserting
                // the name is consistent with the resolved resource - rather than merely non-empty -
                // is what distinguishes "resource resolved" from "fell back to L\"%1!s!, %2!s!\"".
                var separator = SeparatorOf(format);
                if (!string.IsNullOrEmpty(separator))
                {
                    Verify.IsTrue(name.Contains(separator),
                        $"The cell name '{name}' must be composed with the separator '{separator}' carried by the resolved CellNameFormat.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a group header peer builds its name from the resolved GroupHeaderNameFormat resource rather than from the hard-coded English fallback.")]
        public void VerifyGroupHeaderNameIsBuiltFromTheResolvedFormatResource()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = TableViewGroupingTestHelpers.CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var map = RequireTableViewResourceMap();
                var nameFormat = TryResolve(map, "TableViewGroupHeaderNameFormat");
                var countFormat = TryResolve(map, "TableViewGroupHeaderCountFormat");

                Verify.IsFalse(string.IsNullOrWhiteSpace(nameFormat), "GroupHeaderNameFormat must resolve.");
                Verify.IsFalse(string.IsNullOrWhiteSpace(countFormat), "GroupHeaderCountFormat must resolve.");

                // GroupInfoToName (TableViewAutomationHelpers.h:228) composes the announced name, and the
                // group header peer is where it surfaces.
                var peer = GetGroupHeaderPeer(tableView, 0);
                Verify.IsNotNull(peer, "The grouped fixture must produce a group header peer.");

                var name = peer.GetName();
                Log.Comment($"Group header automation name: '{name}'; name format: '{nameFormat}'; count format: '{countFormat}'.");

                Verify.IsFalse(string.IsNullOrWhiteSpace(name), "A realized group header must expose an automation name.");

                // GroupInfoToName composes KeyText and ItemCountText through the name format, and
                // ItemCountText is itself produced through the count format. The count format's literal
                // framing - the parentheses in en-US - must therefore appear in the announced name.
                var framing = LiteralFramingOf(countFormat);
                if (!string.IsNullOrEmpty(framing))
                {
                    Verify.IsTrue(name.Contains(framing),
                        $"The group header name '{name}' must carry the framing '{framing}' from the resolved GroupHeaderCountFormat.");
                }
            });
        }

        #endregion
    }

    internal static class TableViewLocalizationTestHelpers
    {
        // The 18 keys defined in Strings\en-US\Resources.resw, which are the 18 SR_TableView* constants
        // declared in ResourceAccessor.h. The SR_ macro maps SR_Foo -> L"Foo", so these are the macro
        // names with the SR_ prefix removed. Kept as an explicit list so that adding a string to the
        // product without a matching resource fails section 17.1 rather than going unnoticed.
        internal static readonly string[] AllResourceKeys =
        {
            "TableViewBooleanFalse",
            "TableViewBooleanTrue",
            "TableViewCellLocalizedControlType",
            "TableViewCellNameFormat",
            "TableViewCellTextSeparator",
            "TableViewColumnHeaderHelpTextFormat",
            "TableViewColumnWidthChanged",
            "TableViewGroupHeaderCountFormat",
            "TableViewGroupHeaderFallback",
            "TableViewGroupHeaderNameFormat",
            "TableViewGroupHeaderNull",
            "TableViewSortAscendingHelpText",
            "TableViewSortCleared",
            "TableViewSortClearedAll",
            "TableViewSortDescendingHelpText",
            "TableViewSortNoneHelpText",
            "TableViewSortedAscending",
            "TableViewSortedDescending",
        };

        // A spread over script, direction and plural rules rather than all 85: these are the locales
        // where a format or separator problem actually shows up differently.
        internal static readonly string[] RepresentativeLocales =
        {
            "fr-FR",    // Latin, space-before-punctuation conventions
            "de-DE",    // Latin, long compounds
            "ja-JP",    // CJK, ideographic punctuation in the header format
            "ar-SA",    // RTL
            "he-IL",    // RTL
            "ru-RU",    // Cyrillic, complex plurals
            "zh-CN",    // Simplified Chinese
            "mi-NZ",    // was checked in empty until #12205; kept as a regression guard
        };

        // Six keys are absent from every non-English locale file. See
        // VerifyEveryResourceResolvesInEveryShippedLocale for the full explanation; this is the ceiling
        // that keeps the gap from growing. It should shrink to zero once localization runs.
        internal const int KnownUnlocalizedKeysPerLocale = 6;

        // Locales allowed to resolve no TableView string at all. Empty: mi-NZ was the one case, and
        // #12205 fixed it. Add an entry only for a locale that is KNOWN to ship empty.
        internal static readonly string[] KnownEmptyLocales = { };

        // A wholly empty locale contributes every key, not just the six, so the ceiling cannot be a flat
        // multiple of the locale count.
        internal static int MaxKnownUnlocalizedCount()
        {
            var emptyCount = RepresentativeLocales.Count(l => KnownEmptyLocales.Contains(l, StringComparer.OrdinalIgnoreCase));
            var partialCount = RepresentativeLocales.Length - emptyCount;

            return (partialCount * KnownUnlocalizedKeysPerLocale) + (emptyCount * AllResourceKeys.Length);
        }

        internal enum PlaceholderStyle
        {
            // StringUtil::FormatString's %1!s! / %2!s! form.
            FormatMessage,

            // The {0} form used by the group-header count resource.
            DotNet,
        }

        internal struct FormatResource
        {
            public string Key;
            public int ExpectedPlaceholders;
            public PlaceholderStyle Style;
        }

        // Arity comes from the product's own call sites, not from the English values:
        //   CellNameFormat            TableViewCellAutomationPeer.cpp:392   header + value
        //   GroupHeaderNameFormat     TableViewAutomationHelpers.h:228      keyText + countText
        //   ColumnHeaderHelpTextFormat TableViewColumnHeaderAutomationPeer.cpp:177  tooltip + sort state
        //   GroupHeaderCountFormat    TableViewGroupHeaderAutomationPeer.cpp:58     count
        internal static readonly FormatResource[] FormatResources =
        {
            new FormatResource { Key = "TableViewCellNameFormat", ExpectedPlaceholders = 2, Style = PlaceholderStyle.FormatMessage },
            new FormatResource { Key = "TableViewGroupHeaderNameFormat", ExpectedPlaceholders = 2, Style = PlaceholderStyle.FormatMessage },
            new FormatResource { Key = "TableViewColumnHeaderHelpTextFormat", ExpectedPlaceholders = 2, Style = PlaceholderStyle.FormatMessage },
            new FormatResource { Key = "TableViewGroupHeaderCountFormat", ExpectedPlaceholders = 1, Style = PlaceholderStyle.DotNet },
        };

        private static Microsoft.Windows.ApplicationModel.Resources.ResourceManager s_manager;
        private static Dictionary<string, string> s_resourcePathsByKey;

        private static Microsoft.Windows.ApplicationModel.Resources.ResourceManager Manager
            => s_manager ?? (s_manager = new Microsoft.Windows.ApplicationModel.Resources.ResourceManager());

        // The product picks its subtree with a build macro - MUXTABULARROOT_NAMESPACE_STR when
        // MUXCONTROLS_TABULAR is defined, otherwise Microsoft.UI.Xaml (ResourceAccessor.cpp:11-15).
        // Neither macro is checked in, so rather than hard-code a guess that would rot at the next
        // packaging change, the map is walked once and each key located by its leaf name. That also
        // makes section 17.3's "all in one subtree" assertion possible.
        private static Dictionary<string, string> ResourcePathsByKey()
        {
            if (s_resourcePathsByKey != null)
            {
                return s_resourcePathsByKey;
            }

            var paths = new Dictionary<string, string>();
            var main = Manager.MainResourceMap;
            var wanted = new HashSet<string>(AllResourceKeys, StringComparer.Ordinal);

            for (uint i = 0; i < main.ResourceCount; i++)
            {
                var entry = main.GetValueByIndex(i);
                var path = entry.Key;

                if (string.IsNullOrEmpty(path))
                {
                    continue;
                }

                var leaf = LeafOf(path);

                if (wanted.Contains(leaf) && !paths.ContainsKey(leaf))
                {
                    paths[leaf] = path;
                }
            }

            s_resourcePathsByKey = paths;
            return paths;
        }

        internal static string FindResourcePath(string key)
            => ResourcePathsByKey().TryGetValue(key, out var path) ? path : null;

        // Returned as the resource map rather than a subtree so callers can resolve by full path and
        // stay independent of how the subtree is named.
        internal static Microsoft.Windows.ApplicationModel.Resources.ResourceMap RequireTableViewResourceMap()
        {
            var main = Manager.MainResourceMap;
            var found = ResourcePathsByKey();

            Verify.IsGreaterThan(found.Count, 0,
                $"No TableView resource was found anywhere in the resource map ({main.ResourceCount} resources scanned). " +
                "This is the #12070 failure mode: the control's resource subtree is not resolvable at runtime.");

            return main;
        }

        internal static string TryResolve(Microsoft.Windows.ApplicationModel.Resources.ResourceMap map, string key)
        {
            var path = FindResourcePath(key);

            if (path == null)
            {
                return null;
            }

            try
            {
                return map.GetValue(path).ValueAsString;
            }
            catch (Exception e)
            {
                Log.Comment($"Resolving {key} threw: {e.Message}");
                return null;
            }
        }

        internal static string TryResolveForLanguage(
            Microsoft.Windows.ApplicationModel.Resources.ResourceMap map,
            string key,
            string language)
        {
            var path = FindResourcePath(key);

            if (path == null)
            {
                return null;
            }

            try
            {
                var context = Manager.CreateResourceContext();
                context.QualifierValues["Language"] = language;

                var candidate = map.GetValue(path, context);

                // A candidate that falls back to the default language is not a translation. Treat it as
                // absent so section 17.2 counts it, which is the whole point of that test.
                if (!ResolvedForLanguage(candidate, language))
                {
                    return null;
                }

                return candidate.ValueAsString;
            }
            catch (Exception e)
            {
                Log.Comment($"Resolving {key} for {language} threw: {e.Message}");
                return null;
            }
        }

        private static bool ResolvedForLanguage(
            Microsoft.Windows.ApplicationModel.Resources.ResourceCandidate candidate,
            string language)
        {
            if (candidate == null)
            {
                return false;
            }

            try
            {
                var qualifiers = candidate.QualifierValues;

                if (qualifiers == null || !qualifiers.TryGetValue("Language", out var resolved) || string.IsNullOrEmpty(resolved))
                {
                    // No language qualifier at all means the resource is language-neutral, which for a
                    // translated string means it came from the default file.
                    return false;
                }

                // A request for fr-FR satisfied by "fr" is a translation; one satisfied by "en-US" is not.
                return language.StartsWith(resolved, StringComparison.OrdinalIgnoreCase)
                    || resolved.StartsWith(language, StringComparison.OrdinalIgnoreCase);
            }
            catch
            {
                return false;
            }
        }

        internal static string LeafOf(string resourcePath)
        {
            var slash = resourcePath.LastIndexOf('/');
            return slash < 0 ? resourcePath : resourcePath.Substring(slash + 1);
        }

        internal static string SubtreeOf(string resourcePath)
        {
            if (resourcePath == null)
            {
                return null;
            }

            var slash = resourcePath.LastIndexOf('/');
            return slash < 0 ? string.Empty : resourcePath.Substring(0, slash);
        }

        internal static int CountPlaceholders(string value, PlaceholderStyle style)
        {
            if (string.IsNullOrEmpty(value))
            {
                return 0;
            }

            var seen = new HashSet<int>();

            for (var i = 0; i < value.Length; i++)
            {
                if (style == PlaceholderStyle.FormatMessage)
                {
                    // %<n>!s!
                    if (value[i] != '%' || i + 1 >= value.Length || !char.IsDigit(value[i + 1]))
                    {
                        continue;
                    }

                    seen.Add(value[i + 1] - '0');
                }
                else
                {
                    // {<n>}
                    if (value[i] != '{' || i + 2 >= value.Length || !char.IsDigit(value[i + 1]) || value[i + 2] != '}')
                    {
                        continue;
                    }

                    seen.Add(value[i + 1] - '0');
                }
            }

            return seen.Count;
        }

        // The literal text a two-placeholder format puts BETWEEN its arguments - ", " for the en-US
        // cell name format. This is what differs between a resolved resource and the English fallback
        // in a locale that punctuates differently, so it is the observable that makes section 17.4 a
        // real check rather than a non-empty assertion.
        internal static string SeparatorOf(string format)
        {
            if (string.IsNullOrEmpty(format))
            {
                return null;
            }

            var first = format.IndexOf("%1", StringComparison.Ordinal);
            var second = format.IndexOf("%2", StringComparison.Ordinal);

            if (first < 0 || second < 0 || second <= first)
            {
                return null;
            }

            // Step past "%1!s!".
            var afterFirst = format.IndexOf('!', first);
            afterFirst = afterFirst < 0 ? first + 2 : format.IndexOf('!', afterFirst + 1) + 1;

            return afterFirst <= 0 || afterFirst >= second
                ? null
                : format.Substring(afterFirst, second - afterFirst);
        }

        // The literal text a one-placeholder format wraps around its argument - "(" for en-US's "({0})".
        // Only the leading run is returned; it is enough to tell the resolved resource from the fallback.
        internal static string LiteralFramingOf(string format)
        {
            if (string.IsNullOrEmpty(format))
            {
                return null;
            }

            var brace = format.IndexOf('{');
            return brace <= 0 ? null : format.Substring(0, brace);
        }
    }
}
