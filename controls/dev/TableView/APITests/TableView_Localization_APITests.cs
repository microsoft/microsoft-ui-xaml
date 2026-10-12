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
    // LocalizedOrFallbackForTableViewAutomation / FormatLocalizedOrFallback fall back to hard-coded
    // English, so asserting on the automation peer alone cannot detect a failed lookup (#12070).
    // These tests therefore read the resource map directly.
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

                // A payload with no localized resources at all is reported as such, not as N missing strings.
                Verify.IsGreaterThan(resolvedSomewhere, 0,
                    "No TableView resource resolved in any non-default language. The test payload appears to carry no localized resources at all, so per-locale coverage cannot be judged.");

                // Known gap: six automation strings exist only in en-US. This is a ceiling that must not
                // grow; tighten KnownUnlocalizedKeysPerLocale as localization lands.
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

                var unexpected = empty.Except(KnownEmptyLocales, StringComparer.OrdinalIgnoreCase).ToList();

                Verify.AreEqual(0, unexpected.Count,
                    $"No shipped locale may be entirely unlocalized. Unexpectedly empty: {string.Join(", ", unexpected)}.");

                foreach (var fixedUp in KnownEmptyLocales.Except(empty, StringComparer.OrdinalIgnoreCase))
                {
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

                var peer = GetCellPeer(tableView, items[0], 0);
                Verify.IsNotNull(peer, "The fixture must produce a cell peer for the first cell.");

                var name = peer.GetName();
                Log.Comment($"Cell automation name: '{name}'; format resource: '{format}'.");

                Verify.IsFalse(string.IsNullOrWhiteSpace(name), "A realized cell must expose an automation name.");

                // Checking the resolved separator, not just a non-empty name, is what tells the resource from the fallback.
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

                var peer = GetGroupHeaderPeer(tableView, 0);
                Verify.IsNotNull(peer, "The grouped fixture must produce a group header peer.");

                var name = peer.GetName();
                Log.Comment($"Group header automation name: '{name}'; name format: '{nameFormat}'; count format: '{countFormat}'.");

                Verify.IsFalse(string.IsNullOrWhiteSpace(name), "A realized group header must expose an automation name.");

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

        internal static readonly string[] RepresentativeLocales =
        {
            "fr-FR",
            "de-DE",
            "ja-JP",
            "ar-SA",
            "he-IL",
            "ru-RU",
            "zh-CN",
            "mi-NZ",
        };

        internal const int KnownUnlocalizedKeysPerLocale = 6;

        // Locales allowed to resolve no TableView string at all.
        internal static readonly string[] KnownEmptyLocales = { };

        internal static int MaxKnownUnlocalizedCount()
        {
            var emptyCount = RepresentativeLocales.Count(l => KnownEmptyLocales.Contains(l, StringComparer.OrdinalIgnoreCase));
            var partialCount = RepresentativeLocales.Length - emptyCount;

            return (partialCount * KnownUnlocalizedKeysPerLocale) + (emptyCount * AllResourceKeys.Length);
        }

        internal enum PlaceholderStyle
        {
            FormatMessage,

            DotNet,
        }

        internal struct FormatResource
        {
            public string Key;
            public int ExpectedPlaceholders;
            public PlaceholderStyle Style;
        }

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

        // Paths are discovered by scanning because the product's resource subtree is chosen by a build macro.
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
                    // A language-neutral candidate is the default-language fallback; treat it as untranslated.
                    return false;
                }

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
                    if (value[i] != '%' || i + 1 >= value.Length || !char.IsDigit(value[i + 1]))
                    {
                        continue;
                    }

                    seen.Add(value[i + 1] - '0');
                }
                else
                {
                    if (value[i] != '{' || i + 2 >= value.Length || !char.IsDigit(value[i + 1]) || value[i + 2] != '}')
                    {
                        continue;
                    }

                    seen.Add(value[i + 1] - '0');
                }
            }

            return seen.Count;
        }

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

            var afterFirst = format.IndexOf('!', first);
            afterFirst = afterFirst < 0 ? first + 2 : format.IndexOf('!', afterFirst + 1) + 1;

            return afterFirst <= 0 || afterFirst >= second
                ? null
                : format.Substring(afterFirst, second - afterFirst);
        }

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
