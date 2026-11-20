// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Linq.Expressions;
using System.Numerics;
using System.Reflection;
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using System.Runtime.CompilerServices;
using System.Text.RegularExpressions;
using Private.Infrastructure;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Framework.ApiSurface
{
    // Sweeps the generated WinRT API surface of Microsoft.UI.Xaml.dll (winrtgeneratedclasses, core metadata,
    // enum tables and simple properties) through the public projection: IInspectable identity, activation
    // factories, dependency-property identifiers, attached and instance properties, events, enum text conversion
    // and parser object creation. Each method checks one contract across every applicable type and reports
    // per-category pass/expected/failure counts before failing on any unexpected result.
    //
    // Scope is the shipping public API: types and members must exist in the public projection that
    // test\CreateTestPayload.ps1 deploys to Test\PrivateAPITests\Microsoft.WinUI.dll, and members of
    // feature-gated ("...Feature_...") projection interfaces are excluded because they are only implemented when
    // their feature is enabled.
    [TestClass]
    public class GeneratedApiSurfaceSweepTests : XamlTestsBase
    {
        private const string PresentationXmlns = "http://schemas.microsoft.com/winfx/2006/xaml/presentation";
        private const string XamlXmlns = "http://schemas.microsoft.com/winfx/2006/xaml";

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Hosting:Mode", "WPF")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        #region Test methods

        [TestMethod]
        [Description("Every default-activatable public Microsoft.UI.Xaml.dll class reports its runtime class name, returns non-empty IIDs that include the WinRT IID of its default interface, and rejects an unknown IID.")]
        public void VerifyInstanceInspectableSurface()
        {
            var result = new SweepResult(SweepCategory.InstanceInspectable);
            int activated = 0;

            foreach (Type type in Catalog.ActivatableClasses)
            {
                UIExecutor.Execute(() =>
                {
                    object instance = Activate(type, result);
                    if (instance == null)
                    {
                        return;
                    }
                    activated++;
                    VerifyInspectable(instance, type.FullName, Catalog.GetDefaultInterface(type), result, type.FullName, verifyIidsQueryable: false);
                });
            }

            // XamlRenderingBackgroundTask is composable-only (protected constructor), so it is reached through a
            // derived class; the inner object must still answer for its own default interface and reject others.
            UIExecutor.Execute(() =>
            {
                var task = new SweepRenderingBackgroundTask();
                IntPtr inner = ((WinRT.IWinRTObject)task).NativeObject.ThisPtr;
                Type defaultInterface = Catalog.GetDefaultInterface(typeof(XamlRenderingBackgroundTask));
                result.Check(defaultInterface != null, "XamlRenderingBackgroundTask: default interface not found in projection metadata");
                if (defaultInterface != null)
                {
                    int hr = QueryAndRelease(inner, Catalog.GetWinRTIid(defaultInterface), result, "XamlRenderingBackgroundTask (composed)");
                    result.Check(hr == 0, $"XamlRenderingBackgroundTask (composed): QI(default interface) returned 0x{hr:X8}");
                }
                int negativeHr = QueryAndRelease(inner, InspectableAbiHelper.IID_NotImplemented, result, "XamlRenderingBackgroundTask (composed)");
                result.Check(negativeHr == InspectableAbiHelper.E_NOINTERFACE, $"XamlRenderingBackgroundTask (composed): QI(unknown IID) returned 0x{negativeHr:X8}, expected E_NOINTERFACE");
                GC.KeepAlive(task);
            });

            Log.Comment($"Activated {activated} of {Catalog.ActivatableClasses.Count} default-activatable classes.");
            result.Report(minimumPassed: 1000);
            Verify.IsTrue(activated >= 200, $"Expected at least 200 default-activatable Microsoft.UI.Xaml.dll classes, activated {activated}.");
        }

        [TestMethod]
        [Description("Every public runtime class served by Microsoft.UI.Xaml.dll!DllGetActivationFactory returns a factory whose runtime class name is the inherited ComBase null HSTRING, which returns non-empty IIDs and answers QueryInterface for IActivationFactory and every projected statics/factory interface; unknown classes fail with CLASS_E_CLASSNOTAVAILABLE; every class Microsoft.UI.Xaml.dll owns (registered to it in the deployed AppXManifest.xml or recorded in KnownUnregisteredMuxClasses) is served, and no other class is.")]
        public void VerifyActivationFactoryInspectableSurface()
        {
            var result = new SweepResult(SweepCategory.ActivationFactories);
            int staticsInterfacesMatched = 0;

            foreach (Type type in Catalog.MuxRuntimeClasses)
            {
                UIExecutor.Execute(() =>
                {
                    int hr = InspectableAbiHelper.GetMuxActivationFactory(type.FullName, out IntPtr factory);
                    if (!result.Check(hr == 0 && factory != IntPtr.Zero, $"{type.FullName}: DllGetActivationFactory returned 0x{hr:X8}"))
                    {
                        return;
                    }
                    try
                    {
                        // Factories do not declare INSPECTABLE_CLASS, so they inherit ctl::ComBase::GetRuntimeClassNameImpl
                        // (components\com\inc\ComBase.h), which succeeds and returns a null HSTRING (empty name).
                        int nameHr = InspectableAbiHelper.GetRuntimeClassName(factory, out string factoryClassName, out bool isNullHString);
                        result.Check(nameHr == 0 && isNullHString && factoryClassName.Length == 0,
                            $"{type.FullName} factory: GetRuntimeClassName returned 0x{nameHr:X8} \"{factoryClassName}\" (null HSTRING: {isNullHString}), expected S_OK with a null HSTRING");

                        int iidsHr = InspectableAbiHelper.GetIids(factory, out Guid[] iids);
                        if (!result.Check(iidsHr == 0 && iids.Length > 0, $"{type.FullName} factory: GetIids returned 0x{iidsHr:X8} with {iids.Length} IIDs"))
                        {
                            return;
                        }
                        // DllGetActivationFactory hands out IActivationFactory, so every factory must answer for it.
                        result.Check(IsQueryable(factory, InspectableAbiHelper.IID_IActivationFactory, result, type.FullName + " factory"),
                            $"{type.FullName} factory: QI(IActivationFactory) failed");

                        foreach (Type staticsInterface in Catalog.GetStaticsAndFactoryInterfaces(type))
                        {
                            staticsInterfacesMatched++;
                            Guid staticsIid = Catalog.GetWinRTIid(staticsInterface);
                            if (ApiSurfaceAllowList.TryGetKnownIidDefect(type.FullName + " factory", staticsIid, IidDefectKind.ReportedButNotQueryable, out KnownIidDefect knownDefect))
                            {
                                int knownHr = QueryAndRelease(factory, staticsIid, result, type.FullName + " factory");
                                result.Check(knownHr == knownDefect.QueryHResult,
                                    knownHr == 0
                                        ? $"{knownDefect.Key}: known defect no longer reproduces (QueryInterface now succeeds) - remove the KnownProductDefects entry"
                                        : $"{knownDefect.Key}: known defect expected QueryInterface 0x{knownDefect.QueryHResult:X8} but got 0x{knownHr:X8}");
                                continue;
                            }
                            result.Check(IsQueryable(factory, staticsIid, result, type.FullName + " factory"),
                                $"{type.FullName} factory: QI({staticsInterface.Name}) failed");
                        }

                        int negativeHr = QueryAndRelease(factory, InspectableAbiHelper.IID_NotImplemented, result, type.FullName + " factory");
                        result.Check(negativeHr == InspectableAbiHelper.E_NOINTERFACE,
                            $"{type.FullName} factory: QI(unknown IID) returned 0x{negativeHr:X8}, expected E_NOINTERFACE");
                    }
                    finally
                    {
                        InspectableAbiHelper.Release(factory);
                    }
                });
            }

            UIExecutor.Execute(() =>
            {
                int hr = InspectableAbiHelper.GetMuxActivationFactory("Microsoft.UI.Xaml.NoSuchGeneratedApiSurfaceSweepClass", out IntPtr factory);
                InspectableAbiHelper.Release(factory);
                result.Check(hr == InspectableAbiHelper.CLASS_E_CLASSNOTAVAILABLE && factory == IntPtr.Zero,
                    $"Unknown runtime class: DllGetActivationFactory returned 0x{hr:X8}, expected CLASS_E_CLASSNOTAVAILABLE and no factory");
            });

            Log.Comment($"Swept {Catalog.MuxRuntimeClasses.Count} factories; matched {staticsInterfacesMatched} projected statics/factory interfaces.");
            // Public classes the DLL does not serve must be reported as "class not available", not with another failure.
            foreach (KeyValuePair<string, int> unserved in Catalog.UnservedClasses)
            {
                result.Check(unserved.Value == InspectableAbiHelper.CLASS_E_CLASSNOTAVAILABLE,
                    $"{unserved.Key}: DllGetActivationFactory returned 0x{unserved.Value:X8}, expected CLASS_E_CLASSNOTAVAILABLE");
            }
            Log.Comment($"Public classes not served by Microsoft.UI.Xaml.dll: {Catalog.UnservedClasses.Count}.");

            // Ownership is decided independently of the DLL under test (deployed registration manifest plus the recorded
            // unregistered Microsoft.UI.Xaml.dll classes), so an owned class that stops being served cannot silently drop out
            // of the sweeps, and a class that starts being served without being recorded as owned is reported.
            foreach (string failure in Catalog.OwnershipFailures)
            {
                result.Fail(failure);
            }
            foreach (string pinned in ApiSurfaceAllowList.KnownUnregisteredMuxClasses)
            {
                if (!Catalog.ProbedClasses.Contains(pinned))
                {
                    result.Fail($"{pinned}: listed in KnownUnregisteredMuxClasses but is no longer a public class - remove the entry");
                }
                else if (Catalog.ClassRegistrations.TryGetValue(pinned, out string server))
                {
                    result.Fail($"{pinned}: listed in KnownUnregisteredMuxClasses but is now registered to {server} - remove the entry");
                }
                else
                {
                    result.Pass();
                }
            }
            Log.Comment($"Ownership: {Catalog.ClassRegistrations.Count} manifest registrations, {ApiSurfaceAllowList.KnownUnregisteredMuxClasses.Count} recorded unregistered Microsoft.UI.Xaml.dll classes, {Catalog.OwnershipFailures.Count} ownership failures.");
            result.Report(minimumPassed: 2000);
            Verify.IsTrue(Catalog.MuxRuntimeClasses.Count >= 400, $"Expected at least 400 public Microsoft.UI.Xaml.dll runtime classes, found {Catalog.MuxRuntimeClasses.Count}.");
            Verify.IsTrue(staticsInterfacesMatched >= 300, $"Expected at least 300 projected statics/factory interfaces, matched {staticsInterfacesMatched}.");
        }

        [TestMethod]
        [Description("IInspectable::GetIids is consistent with QueryInterface: every IID an instance or activation factory reports is queryable, and every factory reports each projected statics/factory interface it implements; recorded product defects (ApiSurfaceAllowList.KnownProductDefects) must still reproduce exactly.")]
        public void VerifyGetIidsEntriesAreQueryable()
        {
            var result = new SweepResult(SweepCategory.GetIidsConsistency);
            var evaluated = new HashSet<KnownIidDefect>();

            foreach (Type type in Catalog.ActivatableClasses)
            {
                UIExecutor.Execute(() =>
                {
                    object instance = Activate(type, result);
                    if (instance == null)
                    {
                        return;
                    }
                    IntPtr inspectable = InspectableAbiHelper.GetInspectable(instance);
                    try
                    {
                        int iidsHr = InspectableAbiHelper.GetIids(inspectable, out Guid[] iids);
                        if (result.Check(iidsHr == 0, $"{type.FullName}: GetIids returned 0x{iidsHr:X8}"))
                        {
                            VerifyEachIidIsQueryable(inspectable, iids, result, type.FullName, evaluated);
                        }
                    }
                    finally
                    {
                        InspectableAbiHelper.Release(inspectable);
                        GC.KeepAlive(instance);
                    }
                });
            }

            foreach (Type type in Catalog.MuxRuntimeClasses)
            {
                UIExecutor.Execute(() =>
                {
                    int hr = InspectableAbiHelper.GetMuxActivationFactory(type.FullName, out IntPtr factory);
                    if (!result.Check(hr == 0 && factory != IntPtr.Zero, $"{type.FullName}: DllGetActivationFactory returned 0x{hr:X8}"))
                    {
                        return;
                    }
                    try
                    {
                        int iidsHr = InspectableAbiHelper.GetIids(factory, out Guid[] iids);
                        if (!result.Check(iidsHr == 0, $"{type.FullName} factory: GetIids returned 0x{iidsHr:X8}"))
                        {
                            return;
                        }
                        string subject = type.FullName + " factory";
                        VerifyEachIidIsQueryable(factory, iids, result, subject, evaluated);
                        foreach (Type staticsInterface in Catalog.GetStaticsAndFactoryInterfaces(type))
                        {
                            Guid iid = Catalog.GetWinRTIid(staticsInterface);
                            bool queryable = IsQueryable(factory, iid, result, subject);
                            if (ApiSurfaceAllowList.TryGetKnownIidDefect(subject, iid, IidDefectKind.QueryableButNotReported, out KnownIidDefect defect))
                            {
                                evaluated.Add(defect);
                                result.Check(queryable && !iids.Contains(iid),
                                    $"{defect.Key}: known defect no longer reproduces as recorded (queryable={queryable}, reported={iids.Contains(iid)}) - update or remove the KnownProductDefects entry");
                            }
                            else if (queryable)
                            {
                                result.Check(iids.Contains(iid), $"{subject}: implements {staticsInterface.Name} {{{iid}}} but GetIids does not report it");
                            }
                        }
                    }
                    finally
                    {
                        InspectableAbiHelper.Release(factory);
                    }
                });
            }

            // Every recorded defect must have been observed; a stale entry (type removed or renamed) fails.
            foreach (KnownIidDefect defect in ApiSurfaceAllowList.KnownProductDefects.Where(d => !evaluated.Contains(d)))
            {
                result.Fail($"{defect.Key}: known defect entry was not exercised by the sweep - update or remove it");
            }
            Log.Comment($"Known GetIids/QueryInterface product defects confirmed: {evaluated.Count} of {ApiSurfaceAllowList.KnownProductDefects.Count}.");
            result.Report(minimumPassed: 3000);
        }

        [TestMethod]
        [Description("Every public static DependencyProperty identifier is non-null, returns the same object on every read and is distinct from the other identifiers of its owner type.")]
        public void VerifyStaticDependencyPropertyIdentifiers()
        {
            var result = new SweepResult(SweepCategory.StaticDependencyProperties);
            int identifiers = 0;

            foreach (Type type in Catalog.MuxRuntimeClasses)
            {
                PropertyInfo[] dpProperties = type.GetProperties(BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly)
                    .Where(p => p.PropertyType == typeof(DependencyProperty) && Catalog.IsSweepableMember(type, p.Name))
                    .ToArray();
                if (dpProperties.Length == 0)
                {
                    continue;
                }

                UIExecutor.Execute(() =>
                {
                    var owners = new Dictionary<IntPtr, string>();
                    foreach (PropertyInfo property in dpProperties)
                    {
                        string key = $"{type.FullName}.{property.Name}";
                        object first = null;
                        object second = null;
                        if (!result.Invoke(key, SweepOperation.Get, () => { first = property.GetValue(null); second = property.GetValue(null); }))
                        {
                            continue;
                        }
                        identifiers++;
                        if (!result.Check(first != null, $"{key} is null"))
                        {
                            continue;
                        }
                        IntPtr identity = InspectableAbiHelper.GetIdentity(first);
                        result.Check(identity != IntPtr.Zero && identity == InspectableAbiHelper.GetIdentity(second), $"{key} returned different objects on consecutive reads");
                        if (owners.TryGetValue(identity, out string other))
                        {
                            result.Fail($"{key} is the same identifier object as {other}");
                        }
                        else
                        {
                            owners.Add(identity, key);
                            result.Pass();
                        }
                    }
                });
            }

            Log.Comment($"Read {identifiers} DependencyProperty identifiers.");
            result.Report(minimumPassed: 800);
            Verify.IsTrue(identifiers >= 400, $"Expected at least 400 DependencyProperty identifiers, read {identifiers}.");
        }

        [TestMethod]
        [Description("Every attached property Get/Set accessor pair agrees with DependencyObject.GetValue, round-trips its current and a distinct sample value, and ClearValue restores the original value.")]
        public void VerifyAttachedPropertyAccessorsRoundTrip()
        {
            var result = new SweepResult(SweepCategory.AttachedProperties);
            int pairs = 0;

            using (var pool = new InstancePool(result))
            {
                foreach (Type type in Catalog.MuxRuntimeClasses)
                {
                    // Static DependencyProperty identifiers are thread-affine, so enumerate them on the UI thread.
                    List<AttachedProperty> attachedProperties = null;
                    UIExecutor.Execute(() => attachedProperties = Catalog.GetAttachedProperties(type).ToList());
                    foreach (AttachedProperty attached in attachedProperties)
                    {
                        UIExecutor.Execute(() =>
                        {
                            DependencyObject target = pool.GetInstanceAssignableTo(attached.TargetType) as DependencyObject;
                            if (!result.Check(target != null, $"{attached.Key}: no activatable target of type {attached.TargetType.FullName}"))
                            {
                                return;
                            }
                            pairs++;
                            Log.Comment($"[Attached property accessors] {attached.Key} target: {target.GetType().FullName}");
                            VerifyAttachedProperty(attached, target, result);
                        });
                    }
                }
            }

            Log.Comment($"Swept {pairs} attached properties.");
            result.Report(minimumPassed: 300);
            Verify.IsTrue(pairs >= 100, $"Expected at least 100 attached properties, swept {pairs}.");
        }

        [TestMethod]
        [Description("Every readable instance property returns the same value as GetValue on its DependencyProperty; writable properties keep their value when set to it and round-trip a distinct boolean/enum sample; the generated accessors reject calls from a non-UI thread.")]
        public void VerifyInstancePropertyGetSetRoundTrip()
        {
            var result = new SweepResult(SweepCategory.InstanceProperties);
            int gets = 0, dpChecks = 0, writes = 0, samples = 0;
            var divergencesExercised = new HashSet<string>(StringComparer.Ordinal);

            // The comparison oracle itself: null and "" are equivalent, but a string never equals a non-string value.
            result.Check(AreSameValue(null, string.Empty) && AreSameValue(string.Empty, null) && !AreSameValue(string.Empty, 42) &&
                         !AreSameValue(false, string.Empty) && !AreSameValue("a", "b"), "AreSameValue string normalization is not limited to string/null operands");

            using (var pool = new InstancePool(result))
            {
                foreach (SweepTarget target in pool.SweepTargets)
                {
                    if (ApiSurfaceAllowList.ProductConcernTypes.TryGetValue(target.DeclaringType.FullName, out string concern))
                    {
                        result.Exclude(target.DeclaringType.FullName, concern);
                        continue;
                    }
                    UIExecutor.Execute(() =>
                    {
                        foreach (PropertyInfo property in Catalog.GetInstanceProperties(target.DeclaringType))
                        {
                            string key = $"{target.DeclaringType.FullName}.{property.Name}";
                            object value = null;
                            if (!result.Invoke(key, SweepOperation.Get, () => value = property.GetValue(target.Instance)))
                            {
                                continue;
                            }
                            gets++;
                            result.Pass();

                            DependencyProperty dp = Catalog.FindDependencyProperty(target.DeclaringType, property.Name);
                            object storedBefore = null;
                            if (dp != null && target.Instance is DependencyObject dependencyObject)
                            {
                                object stored = null;
                                if (result.Invoke(key, SweepOperation.GetValue, () => stored = dependencyObject.GetValue(dp)))
                                {
                                    dpChecks++;
                                    storedBefore = stored;
                                    if (ApiSurfaceAllowList.KnownPropertyStoreSplits.TryGetValue(key, out KnownPropertyStoreSplit split))
                                    {
                                        // The getter reads its private field (initialized to GetterValue), not the DependencyProperty store.
                                        object getterValue = Enum.Parse(property.PropertyType, split.GetterValue);
                                        result.Check(AreSameValue(value, getterValue),
                                            $"{key} [{split.ConcernId}]: known defect expected the getter to return its field value {Describe(getterValue)}, read {Describe(value)} (GetValue: {Describe(stored)})");
                                    }
                                    else
                                    {
                                        CrossCheckGetValue(key, value, stored, result, divergencesExercised);
                                    }
                                }
                            }

                            if (property.SetMethod == null || !property.SetMethod.IsPublic)
                            {
                                continue;
                            }
                            if (ApiSurfaceAllowList.PropertyWriteExclusions.TryGetValue(key, out string reason))
                            {
                                result.Exclude(key, reason);
                                continue;
                            }

                            if (!result.Invoke(key, SweepOperation.Set, () => property.SetValue(target.Instance, value)))
                            {
                                continue;
                            }
                            writes++;
                            object afterSet = property.GetValue(target.Instance);
                            result.Check(AreSameValue(afterSet, value), $"{key}: setting the current value {Describe(value)} changed it to {Describe(afterSet)}");

                            ApiSurfaceAllowList.KnownPropertyStoreSplits.TryGetValue(key, out KnownPropertyStoreSplit sampleSplit);
                            object sample = dp != null ? DistinctSample(property.PropertyType, value, includeNumericAndString: false) : null;
                            if (sampleSplit != null && property.PropertyType.IsEnum)
                            {
                                // The sample must differ from both stores so a setter that reached the DependencyProperty is observable.
                                sample = Enum.GetValues(property.PropertyType).Cast<object>().FirstOrDefault(v => !AreSameValue(v, value) && !AreSameValue(v, storedBefore));
                            }
                            if (sample != null && result.Invoke(key, SweepOperation.SetSample, () => property.SetValue(target.Instance, sample)))
                            {
                                samples++;
                                object afterSample = property.GetValue(target.Instance);
                                object storedSample = ((DependencyObject)target.Instance).GetValue(dp);
                                if (ApiSurfaceAllowList.KnownSampleOutcomes.TryGetValue(key, out KnownSampleOutcome outcome))
                                {
                                    // Asserts both the getter and GetValue against the recorded outcome.
                                    VerifyKnownSampleOutcome(key, property.PropertyType, outcome, sample, afterSample, storedSample, result);
                                }
                                else if (sampleSplit != null)
                                {
                                    result.Check(AreSameValue(afterSample, sample), $"{key}: set {Describe(sample)} but read {Describe(afterSample)}");
                                    // The setter writes only the private field, so the DependencyProperty store keeps its prior value.
                                    result.Check(AreSameValue(storedSample, storedBefore),
                                        AreSameValue(storedSample, sample)
                                            ? $"{key} [{sampleSplit.ConcernId}]: known defect no longer reproduces (the setter now updates GetValue) - remove the KnownPropertyStoreSplits entry"
                                            : $"{key} [{sampleSplit.ConcernId}]: known defect expected GetValue to keep {Describe(storedBefore)} after setting {Describe(sample)}, read {Describe(storedSample)}");
                                }
                                else
                                {
                                    result.Check(AreSameValue(afterSample, sample), $"{key}: set {Describe(sample)} but read {Describe(afterSample)}");
                                    result.Check(AreSameValue(storedSample, sample), $"{key}: set {Describe(sample)} but GetValue read {Describe(storedSample)}");
                                }
                                if (result.Invoke(key, SweepOperation.Restore, () => property.SetValue(target.Instance, value)))
                                {
                                    object afterRestore = property.GetValue(target.Instance);
                                    result.Check(AreSameValue(afterRestore, value), $"{key}: restoring {Describe(value)} read {Describe(afterRestore)}");
                                }
                            }
                        }
                    });
                }
            }

            foreach (string divergenceKey in ApiSurfaceAllowList.KnownGetterStoreDivergences.Keys.Where(k => !divergencesExercised.Contains(k)))
            {
                result.Fail($"{divergenceKey}: KnownGetterStoreDivergences entry was not exercised - update or remove the entry");
            }

            // Generated accessors call CheckThread first, so a call from the (non-UI) test thread must fail with
            // RPC_E_WRONG_THREAD instead of reading the value.
            VisualStateGroup group = null;
            UIExecutor.Execute(() => group = new VisualStateGroup());
            Exception wrongThread = CatchException(() => { string unused = group.Name; });
            result.Check(wrongThread != null && wrongThread.HResult == InspectableAbiHelper.RPC_E_WRONG_THREAD,
                $"VisualStateGroup.Name from a non-UI thread: expected RPC_E_WRONG_THREAD, got {DescribeException(wrongThread)}");

            Log.Comment($"Property gets={gets}, GetValue cross-checks={dpChecks}, same-value writes={writes}, distinct-sample writes={samples}.");
            result.Report(minimumPassed: 1200);
            Verify.IsTrue(gets >= 600, $"Expected at least 600 property reads, performed {gets}.");
            Verify.IsTrue(dpChecks >= 300, $"Expected at least 300 GetValue cross-checks, performed {dpChecks}.");
            Verify.IsTrue(writes >= 250, $"Expected at least 250 property writes, performed {writes}.");
        }

        [TestMethod]
        [Description("Every public instance event and every UIElement routed event accepts and releases a handler (null routed handlers fail with E_INVALIDARG), representative events stop being delivered after removal, and VisualStateManager raise methods reject null arguments with E_INVALIDARG.")]
        public void VerifyInstanceEventAddRemove()
        {
            var result = new SweepResult(SweepCategory.Events);
            int events = 0, routedEvents = 0;

            using (var pool = new InstancePool(result))
            {
                foreach (SweepTarget target in pool.SweepTargets)
                {
                    UIExecutor.Execute(() =>
                    {
                        foreach (EventInfo eventInfo in target.DeclaringType.GetEvents(BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly).Where(e => Catalog.IsSweepableMember(target.DeclaringType, e.Name)))
                        {
                            string key = $"{target.DeclaringType.FullName}.{eventInfo.Name}";
                            var counter = new StrongBox<int>();
                            Delegate handler = CreateCountingHandler(eventInfo.EventHandlerType, counter);
                            // Add/remove twice: the second cycle re-creates the event source released by the first.
                            if (result.Invoke(key, SweepOperation.AddRemove, () =>
                                {
                                    eventInfo.AddEventHandler(target.Instance, handler);
                                    eventInfo.RemoveEventHandler(target.Instance, handler);
                                    eventInfo.AddEventHandler(target.Instance, handler);
                                    eventInfo.RemoveEventHandler(target.Instance, handler);
                                }))
                            {
                                events++;
                                result.Pass();
                            }
                        }
                    });
                }

                foreach (Type type in Catalog.MuxRuntimeClasses)
                {
                    foreach (PropertyInfo routedEventProperty in type.GetProperties(BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly).Where(p => p.PropertyType == typeof(RoutedEvent) && Catalog.IsSweepableMember(type, p.Name)))
                    {
                        UIExecutor.Execute(() =>
                        {
                            string key = $"{type.FullName}.{routedEventProperty.Name}";
                            string clrEventName = routedEventProperty.Name.EndsWith("Event", StringComparison.Ordinal)
                                ? routedEventProperty.Name.Substring(0, routedEventProperty.Name.Length - "Event".Length)
                                : routedEventProperty.Name;
                            EventInfo clrEvent = type.GetEvent(clrEventName, BindingFlags.Public | BindingFlags.Instance);
                            UIElement element = pool.GetInstanceAssignableTo(type) as UIElement;
                            if (!result.Check(clrEvent != null && element != null, $"{key}: no matching CLR event or UIElement target"))
                            {
                                return;
                            }
                            var routedEvent = (RoutedEvent)routedEventProperty.GetValue(null);
                            Delegate handler = CreateCountingHandler(clrEvent.EventHandlerType, new StrongBox<int>());
                            if (result.Invoke(key, SweepOperation.AddRemove, () =>
                                {
                                    element.AddHandler(routedEvent, handler, false);
                                    element.RemoveHandler(routedEvent, handler);
                                    element.AddHandler(routedEvent, handler, true);
                                    element.RemoveHandler(routedEvent, handler);
                                }))
                            {
                                routedEvents++;
                                result.Pass();
                            }
                            Exception nullHandler = CatchException(() => element.AddHandler(routedEvent, null, false));
                            result.Check(nullHandler != null && nullHandler.HResult == InspectableAbiHelper.E_INVALIDARG,
                                $"{key}: AddHandler(null) expected E_INVALIDARG, got {DescribeException(nullHandler)}");
                        });
                    }
                }
            }

            VerifyRangeBaseValueChangedDeliveryStopsAfterRemoval(result);
            VerifyToggleButtonCheckedDeliveryStopsAfterRemoval(result);
            VerifyVisualStateGroupEventsDeliveryStopsAfterRemoval(result);

            Log.Comment($"Instance events add/removed={events}, routed events add/removed={routedEvents}.");
            result.Report(minimumPassed: 150);
            Verify.IsTrue(events >= 100, $"Expected at least 100 instance events, swept {events}.");
            Verify.IsTrue(routedEvents >= 30, $"Expected at least 30 routed events, swept {routedEvents}.");
        }

        [TestMethod]
        [Description("Every enum value name of each enum-typed dependency property parses from XAML attribute text to that value (flags combine), invalid names fail with E_XAMLPARSEFAILED, and Microsoft.UI.Xaml enums parse as keyed object elements.")]
        public void VerifyEnumStringConversionThroughXaml()
        {
            var result = new SweepResult(SweepCategory.EnumTextConversion);
            int attributeValues = 0, objectElementValues = 0, enumTypes = 0;

            using (var pool = new InstancePool(result))
            {
                // Static DependencyProperty identifiers are thread-affine, so discover hosts on the UI thread.
                IEnumerable<EnumHost> hosts = null;
                UIExecutor.Execute(() => hosts = GetEnumHosts(pool));
                foreach (EnumHost host in hosts)
                {
                    enumTypes++;
                    UIExecutor.Execute(() =>
                    {
                        Type enumType = host.Property.PropertyType;
                        foreach (string name in Enum.GetNames(enumType))
                        {
                            attributeValues++;
                            VerifyEnumAttribute(host, name, Enum.Parse(enumType, name), result);
                        }

                        if (enumType.IsDefined(typeof(FlagsAttribute), false))
                        {
                            ulong[] singleBits = Enum.GetValues(enumType).Cast<object>().Select(v => Convert.ToUInt64(v))
                                .Where(v => v != 0 && (v & (v - 1)) == 0).Distinct().Take(2).ToArray();
                            if (singleBits.Length == 2)
                            {
                                string text = $"{Enum.ToObject(enumType, singleBits[0])},{Enum.ToObject(enumType, singleBits[1])}";
                                VerifyEnumAttribute(host, text, Enum.ToObject(enumType, singleBits[0] | singleBits[1]), result);
                            }
                        }

                        Exception invalid = CatchException(() => XamlReader.Load(EnumAttributeXaml(host, "NotAGeneratedEnumValue")));
                        result.Check(invalid != null && invalid.HResult == InspectableAbiHelper.E_XAMLPARSEFAILED,
                            $"{host.Key}=\"NotAGeneratedEnumValue\": expected E_XAMLPARSEFAILED, got {DescribeException(invalid)}");

                        if (enumType.Namespace.StartsWith("Microsoft.UI.Xaml", StringComparison.Ordinal))
                        {
                            objectElementValues += VerifyEnumObjectElements(enumType, result);
                        }
                    });
                }

                foreach (string splitKey in ApiSurfaceAllowList.KnownPropertyStoreSplits.Keys.Where(k => !hosts.Any(h => h.Key == k)))
                {
                    result.Fail($"{splitKey}: KnownPropertyStoreSplits entry was not exercised (property is no longer an enum host) - update or remove the entry");
                }
            }

            Log.Comment($"Enum types={enumTypes}, attribute values={attributeValues}, object-element values={objectElementValues}.");
            result.Report(minimumPassed: 500);
            Verify.IsTrue(enumTypes >= 60, $"Expected at least 60 enum-typed dependency properties, found {enumTypes}.");
            Verify.IsTrue(objectElementValues >= 200, $"Expected at least 200 enum object-element values, parsed {objectElementValues}.");
        }

        [TestMethod]
        [Description("XAML object elements create every parser-creatable class; simple properties parse from XAML text and malformed text fails; strict-only simple properties block non-strict APIs only while set to a non-default value; automation peers created for elements are cached and expose their own IInspectable surface.")]
        public void VerifyParserObjectElementsAndSimpleProperties()
        {
            var parser = new SweepResult(SweepCategory.ParserObjectElements);
            var simple = new SweepResult(SweepCategory.SimpleProperties);
            var strictness = new SweepResult(SweepCategory.StrictOnlySimpleProperties);
            var peers = new SweepResult(SweepCategory.AutomationPeers);

            VerifyParserObjectElements(parser);
            VerifySimplePropertiesFromXaml(simple);
            VerifyStrictOnlySimpleProperties(strictness);
            int peerCount = VerifyAutomationPeersForElements(peers);

            parser.Report(minimumPassed: 150, verify: false);
            simple.Report(minimumPassed: 15, verify: false);
            // UIElement always contributes 11 checks (Translation coexistence + 2 per strict-only property x 5);
            // LinearGradientBrush adds 2 per property only when its feature-gated simple properties are public.
            strictness.Report(minimumPassed: 11, verify: false);
            peers.Report(minimumPassed: 200, verify: false);
            SweepResult.VerifyAll(parser, simple, strictness, peers);
            Verify.IsTrue(peerCount >= 50, $"Expected at least 50 automation peers, created {peerCount}.");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product defect PC-4: PressedKeys on a default-constructed AccessKeyDisplayRequestedEventArgs access-violates in CAccessKeyDisplayRequestedEventArgs::get_PressedKeys (CEventArgs core peer static_cast; VM dump TE.ProcessHost.exe.5456.dmp)
        [Description("A default-constructed AccessKeyDisplayRequestedEventArgs (public parameterless constructor) reports empty PressedKeys.")]
        public void VerifyDefaultConstructedAccessKeyDisplayRequestedEventArgs()
        {
            // Separate from the property sweep: on the current build this read dereferences an invalid core peer
            // (product concern PC-4), so it is isolated to keep the generic sweep's results independent.
            string pressedKeys = null;
            UIExecutor.Execute(() => pressedKeys = new Microsoft.UI.Xaml.Input.AccessKeyDisplayRequestedEventArgs().PressedKeys);
            Verify.AreEqual(string.Empty, pressedKeys ?? string.Empty, "PressedKeys of a default-constructed AccessKeyDisplayRequestedEventArgs");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product defect PC-4: Handled on a default-constructed AccessKeyInvokedEventArgs static_casts a plain CEventArgs core peer to CAccessKeyInvokedEventArgs (out-of-bounds read/write; FrameworkEventArgs.cpp CreateCorePeer)
        [Description("A default-constructed AccessKeyInvokedEventArgs (public parameterless constructor) starts unhandled and round-trips Handled.")]
        public void VerifyDefaultConstructedAccessKeyInvokedEventArgs()
        {
            // Separate from the property sweep for the same core-peer type mismatch as PC-4.
            bool initial = true, afterSetTrue = false, afterSetFalse = true;
            UIExecutor.Execute(() =>
            {
                var args = new Microsoft.UI.Xaml.Input.AccessKeyInvokedEventArgs();
                initial = args.Handled;
                args.Handled = true;
                afterSetTrue = args.Handled;
                args.Handled = false;
                afterSetFalse = args.Handled;
            });
            Verify.IsFalse(initial, "Handled of a default-constructed AccessKeyInvokedEventArgs");
            Verify.IsTrue(afterSetTrue, "Handled after setting true");
            Verify.IsFalse(afterSetFalse, "Handled after setting false");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product defects PC-16/PC-20: AutoPlay set on a detached MediaPlayerElement is not read back (getter reads the MediaPlayer; MediaPlayerElement_partial.cpp) and releasing the created MediaPlayer during a render tick trips CReentrancyGuard (VM dump TE.ProcessHost.exe.8752.dmp)
        [Description("AutoPlay set to true on a detached MediaPlayerElement reads back true through both the property and GetValue.")]
        public void VerifyMediaPlayerElementAutoPlayOnDetachedElement()
        {
            bool fromGetter = false;
            object fromStore = null;
            UIExecutor.Execute(() =>
            {
                var element = new MediaPlayerElement();
                element.AutoPlay = true;
                fromGetter = element.AutoPlay;
                fromStore = element.GetValue(MediaPlayerElement.AutoPlayProperty);
            });
            Verify.IsTrue(fromGetter, "MediaPlayerElement.AutoPlay after setting true on a detached element");
            Verify.AreEqual(true, fromStore, "GetValue(AutoPlayProperty) after setting true on a detached element");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product defect PC-15: PasswordBox::put_InputScopeImpl dereferences a null InputScope argument (PasswordBox_Partial.cpp:1012; VM dump TE.ProcessHost.exe.10024.dmp)
        [Description("Assigning null to PasswordBox.InputScope (its default value; the generated setter takes an optional value) succeeds and InputScope reads back null.")]
        public void VerifyPasswordBoxInputScopeNullAssignment()
        {
            Exception error = null;
            Microsoft.UI.Xaml.Input.InputScope afterAssignment = null;
            UIExecutor.Execute(() =>
            {
                var passwordBox = new PasswordBox();
                error = CatchException(() => passwordBox.InputScope = null);
                afterAssignment = passwordBox.InputScope;
            });
            Verify.IsTrue(error == null, $"PasswordBox.InputScope = null: expected success, got {DescribeException(error)}");
            Verify.IsNull(afterAssignment, "PasswordBox.InputScope after assigning null");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product defect PC-14: UIElement.CanDrag = true on a detached element fail-fasts in CInputServices::GetIslandInputSiteRegistrationForUIElement (InputServices.cpp FAIL_FAST_IF; VM dump TE.ProcessHost.exe.4640.dmp)
        [Description("Enabling CanDrag on an element that is not in the live tree succeeds and the value reads back; clearing it restores false.")]
        public void VerifyDetachedElementCanDragAssignment()
        {
            bool afterEnable = false, afterDisable = true;
            UIExecutor.Execute(() =>
            {
                var border = new Border();
                border.CanDrag = true;
                afterEnable = border.CanDrag;
                border.CanDrag = false;
                afterDisable = border.CanDrag;
            });
            Verify.IsTrue(afterEnable, "CanDrag after setting true on a detached Border");
            Verify.IsFalse(afterDisable, "CanDrag after setting false on a detached Border");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product defect PC-13: CRenderTargetBitmap::PixelWidth/PixelHeight dereference a null m_impl before RenderAsync (RenderTargetBitmap.cpp:59/78; VM dump TE.ProcessHost.exe.9464.dmp)
        [Description("A RenderTargetBitmap that has not rendered yet reports a pixel size of 0 x 0.")]
        public void VerifyDefaultConstructedRenderTargetBitmapPixelSize()
        {
            int pixelWidth = -1, pixelHeight = -1;
            UIExecutor.Execute(() =>
            {
                var bitmap = new RenderTargetBitmap();
                pixelWidth = bitmap.PixelWidth;
                pixelHeight = bitmap.PixelHeight;
            });
            Verify.AreEqual(0, pixelWidth, "PixelWidth of a RenderTargetBitmap before RenderAsync");
            Verify.AreEqual(0, pixelHeight, "PixelHeight of a RenderTargetBitmap before RenderAsync");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product defect PC-12: EventTrigger::put_RoutedEventImpl dereferences a null RoutedEvent argument (EventTrigger_Partial.cpp:38; VM dump TE.ProcessHost.exe.6196.dmp)
        [Description("Assigning null to EventTrigger.RoutedEvent (its default value; the generated setter takes an optional value) succeeds and RoutedEvent reads back null.")]
        public void VerifyEventTriggerRoutedEventNullAssignment()
        {
            Exception error = null;
            RoutedEvent afterAssignment = null;
            UIExecutor.Execute(() =>
            {
                var trigger = new EventTrigger();
                error = CatchException(() => trigger.RoutedEvent = null);
                afterAssignment = trigger.RoutedEvent;
            });
            Verify.IsTrue(error == null, $"EventTrigger.RoutedEvent = null: expected success, got {DescribeException(error)}");
            Verify.IsNull(afterAssignment, "EventTrigger.RoutedEvent after assigning null");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product defect PC-7: DeclaringType/Name on a default-constructed ProvideValueTargetProperty dereference a null m_xamlProperty (ProvideValueTargetProperty_Partial.cpp; VM dump TE.ProcessHost.exe.10472.dmp)
        [Description("A default-constructed ProvideValueTargetProperty (public parameterless constructor) reports no declaring type and an empty name.")]
        public void VerifyDefaultConstructedProvideValueTargetProperty()
        {
            Type declaringType = typeof(object);
            string name = null;
            UIExecutor.Execute(() =>
            {
                var property = new ProvideValueTargetProperty();
                declaringType = property.DeclaringType;
                name = property.Name;
            });
            Verify.IsNull(declaringType, "DeclaringType of a default-constructed ProvideValueTargetProperty");
            Verify.AreEqual(string.Empty, name ?? string.Empty, "Name of a default-constructed ProvideValueTargetProperty");
        }

        #endregion

        #region Inspectable helpers

        private static void VerifyInspectable(object instance, string expectedClassName, Type defaultInterface, SweepResult result, string label, bool verifyIidsQueryable, bool failedQueryClearsOutPointer = false)
        {
            IntPtr inspectable = InspectableAbiHelper.GetInspectable(instance);
            try
            {
                int nameHr = InspectableAbiHelper.GetRuntimeClassName(inspectable, out string className);
                if (expectedClassName != null)
                {
                    result.Check(nameHr == 0 && className == expectedClassName, $"{label}: GetRuntimeClassName returned 0x{nameHr:X8} \"{className}\"");
                }
                else
                {
                    result.Check(nameHr == 0 && !string.IsNullOrEmpty(className), $"{label}: GetRuntimeClassName returned 0x{nameHr:X8} \"{className}\"");
                }

                int iidsHr = InspectableAbiHelper.GetIids(inspectable, out Guid[] iids);
                if (!result.Check(iidsHr == 0 && iids.Length > 0, $"{label}: GetIids returned 0x{iidsHr:X8} with {iids.Length} IIDs"))
                {
                    return;
                }
                if (defaultInterface != null)
                {
                    // Generic default interfaces (IList`1 -> IVector<T>) need the WinRT parameterized IID, not Type.GUID.
                    Guid defaultIid = Guid.Empty;
                    Exception iidError = CatchException(() => defaultIid = Catalog.GetWinRTIid(defaultInterface));
                    if (iidError != null)
                    {
                        result.Fail($"{label}: cannot compute the WinRT IID of default interface {defaultInterface}: {DescribeException(iidError)}");
                    }
                    else
                    {
                        result.Check(iids.Contains(defaultIid), $"{label}: GetIids does not report default interface {defaultInterface} {{{defaultIid}}}");
                    }
                }
                else if (expectedClassName != null)
                {
                    result.Fail($"{label}: default interface not found in projection metadata");
                }
                if (verifyIidsQueryable)
                {
                    VerifyEachIidIsQueryable(inspectable, iids, result, label, failedQueryClearsOutPointer: failedQueryClearsOutPointer);
                }

                int negativeHr = QueryAndRelease(inspectable, InspectableAbiHelper.IID_NotImplemented, result, label, failedQueryClearsOutPointer);
                result.Check(negativeHr == InspectableAbiHelper.E_NOINTERFACE, $"{label}: QI(unknown IID) returned 0x{negativeHr:X8}, expected E_NOINTERFACE");
            }
            finally
            {
                InspectableAbiHelper.Release(inspectable);
                GC.KeepAlive(instance);
            }
        }

        private static void VerifyEachIidIsQueryable(IntPtr comPointer, Guid[] iids, SweepResult result, string label, ISet<KnownIidDefect> evaluated = null, bool failedQueryClearsOutPointer = false)
        {
            foreach (Guid iid in iids)
            {
                int hr = QueryAndRelease(comPointer, iid, result, label, failedQueryClearsOutPointer);
                if (ApiSurfaceAllowList.TryGetKnownIidDefect(label, iid, IidDefectKind.ReportedButNotQueryable, out KnownIidDefect defect))
                {
                    evaluated?.Add(defect);
                    result.Check(hr == defect.QueryHResult,
                        hr == 0
                            ? $"{defect.Key}: known defect no longer reproduces (QueryInterface now succeeds) - remove the KnownProductDefects entry"
                            : $"{defect.Key}: known defect expected QueryInterface 0x{defect.QueryHResult:X8} but got 0x{hr:X8}");
                }
                else
                {
                    result.Check(hr == 0, $"{label}: GetIids reported {{{iid}}} but QueryInterface for it returned 0x{hr:X8}");
                }
            }
            foreach (KnownIidDefect defect in ApiSurfaceAllowList.KnownProductDefects.Where(d => d.Subject == label && d.Kind == IidDefectKind.ReportedButNotQueryable && !iids.Contains(d.Iid)))
            {
                evaluated?.Add(defect);
                result.Fail($"{defect.Key}: known defect no longer reproduces (GetIids no longer reports the IID) - remove the KnownProductDefects entry");
            }
        }

        // QueryInterface that also asserts the COM out-pointer contract: *ppv starts as a non-null sentinel and a successful
        // call must write a valid pointer. A failed call must set *ppv to null (failedQueryClearsOutPointer: automation
        // peers do); the ComBase-based objects and activation factories of Microsoft.UI.Xaml.dll currently leave it unchanged
        // (ApiSurfaceAllowList.FailedQueryInterfaceLeavesOutPointerUnchanged), which is asserted exactly. Either way a
        // failed call that writes any other value fails. A failed call's out pointer is never released (not owned).
        private static int QueryAndRelease(IntPtr comPointer, Guid iid, SweepResult result, string label, bool failedQueryClearsOutPointer = false)
        {
            int hr = InspectableAbiHelper.QueryInterface(comPointer, iid, out IntPtr queried, out IntPtr outPointerOnFailure);
            bool unwritten = outPointerOnFailure == InspectableAbiHelper.QueryInterfaceOutSentinel;
            if (hr < 0)
            {
                if (failedQueryClearsOutPointer)
                {
                    result.Check(outPointerOnFailure == IntPtr.Zero, unwritten
                        ? $"{label}: QueryInterface({{{iid}}}) failed with 0x{hr:X8} without clearing its out pointer (COM requires null on failure)"
                        : $"{label}: QueryInterface({{{iid}}}) failed with 0x{hr:X8} but wrote a non-null out pointer (COM requires null on failure)");
                }
                else if (outPointerOnFailure == IntPtr.Zero)
                {
                    result.Fail($"{label}: QueryInterface({{{iid}}}) failed with 0x{hr:X8} and cleared its out pointer - known defect no longer reproduces, " +
                                $"remove ApiSurfaceAllowList.FailedQueryInterfaceLeavesOutPointerUnchanged ({ApiSurfaceAllowList.FailedQueryInterfaceLeavesOutPointerUnchanged})");
                }
                else
                {
                    result.Check(unwritten,
                        $"{label}: QueryInterface({{{iid}}}) failed with 0x{hr:X8} but wrote a non-null out pointer (COM requires null on failure)");
                }
            }
            else if (!result.Check(queried != IntPtr.Zero, unwritten
                ? $"{label}: QueryInterface({{{iid}}}) returned 0x{hr:X8} without writing its out pointer"
                : $"{label}: QueryInterface({{{iid}}}) returned 0x{hr:X8} with a null pointer"))
            {
                return InspectableAbiHelper.E_POINTER;
            }
            InspectableAbiHelper.Release(queried);
            return hr;
        }

        private static bool IsQueryable(IntPtr comPointer, Guid iid, SweepResult result, string label)
        {
            return QueryAndRelease(comPointer, iid, result, label) == 0;
        }

        private static object Activate(Type type, SweepResult result)
        {
            object instance = null;
            if (result.Invoke(type.FullName, SweepOperation.Activate, () => instance = Activator.CreateInstance(type)))
            {
                return instance;
            }
            return null;
        }

        #endregion

        #region Attached and instance property helpers

        private static void VerifyAttachedProperty(AttachedProperty attached, DependencyObject target, SweepResult result)
        {
            Func<object> get = () => attached.Getter.Invoke(null, new object[] { target });
            object original = null;
            if (!result.Invoke(attached.Key, SweepOperation.Get, () => original = get()))
            {
                return;
            }
            object stored = null;
            if (result.Invoke(attached.Key, SweepOperation.GetValue, () => stored = target.GetValue(attached.Property)))
            {
                result.Check(AreSameValue(original, stored), $"{attached.Key}: Get returned {Describe(original)} but GetValue returned {Describe(stored)}");
            }

            if (attached.Setter == null)
            {
                return;
            }
            if (ApiSurfaceAllowList.PropertyWriteExclusions.TryGetValue(attached.Key, out string reason))
            {
                result.Exclude(attached.Key, reason);
                return;
            }
            if (!result.Invoke(attached.Key, SweepOperation.Set, () => attached.Setter.Invoke(null, new object[] { target, original })))
            {
                return;
            }
            object afterSet = get();
            result.Check(AreSameValue(afterSet, original), $"{attached.Key}: setting the current value {Describe(original)} changed it to {Describe(afterSet)}");

            object sample = DistinctSample(attached.ValueType, original, includeNumericAndString: true);
            if (sample == null || !result.Invoke(attached.Key, SweepOperation.SetSample, () => attached.Setter.Invoke(null, new object[] { target, sample })))
            {
                return;
            }
            object afterSample = get();
            object storedSample = target.GetValue(attached.Property);
            result.Check(AreSameValue(afterSample, sample), $"{attached.Key}: Set {Describe(sample)} but Get returned {Describe(afterSample)}");
            result.Check(AreSameValue(storedSample, sample), $"{attached.Key}: Set {Describe(sample)} but GetValue returned {Describe(storedSample)}");

            if (result.Invoke(attached.Key, SweepOperation.ClearValue, () => target.ClearValue(attached.Property)))
            {
                object afterClear = get();
                result.Check(AreSameValue(afterClear, original), $"{attached.Key}: ClearValue read {Describe(afterClear)}, expected the original {Describe(original)}");
            }
        }
        // Returns a value of 'type' different from 'current', or null when the type has no safe distinct sample.
        private static object DistinctSample(Type type, object current, bool includeNumericAndString)
        {
            Type underlying = Nullable.GetUnderlyingType(type) ?? type;
            if (underlying == typeof(bool))
            {
                return !(current is bool b && b);
            }
            if (underlying.IsEnum)
            {
                long currentValue = current == null ? long.MinValue : Convert.ToInt64(current);
                foreach (object candidate in Enum.GetValues(underlying))
                {
                    if (Convert.ToInt64(candidate) != currentValue)
                    {
                        return candidate;
                    }
                }
                return null;
            }
            if (!includeNumericAndString)
            {
                return null;
            }
            if (underlying == typeof(int))
            {
                return (current is int i ? i : 0) + 1;
            }
            if (underlying == typeof(double))
            {
                double d = current is double value ? value : double.NaN;
                return double.IsFinite(d) ? d + 1 : 1.0;
            }
            if (underlying == typeof(string))
            {
                return "WinUISweep";
            }
            return null;
        }

        // Getter vs GetValue for a DependencyProperty-backed property: strict AreSameValue, except DateTimeOffset values
        // within 1 minute (DatePicker MinYear/MaxYear defaults are computed from the current time on every read) and the
        // exact divergences recorded in ApiSurfaceAllowList.KnownGetterStoreDivergences.
        private static void CrossCheckGetValue(string key, object value, object stored, SweepResult result, ISet<string> divergencesExercised)
        {
            if (ApiSurfaceAllowList.KnownGetterStoreDivergences.TryGetValue(key, out KnownGetterStoreDivergence divergence))
            {
                divergencesExercised.Add(key);
                VerifyKnownGetterStoreDivergence(key, divergence, value, stored, result);
                return;
            }
            if (value is DateTimeOffset fromGetter && stored is DateTimeOffset fromStore)
            {
                result.Check((fromGetter - fromStore).Duration() < TimeSpan.FromMinutes(1),
                    $"{key}: property returned {Describe(value)} but GetValue returned {Describe(stored)}");
                return;
            }
            result.Check(AreSameValue(value, stored), $"{key}: property returned {Describe(value)} but GetValue returned {Describe(stored)}");
        }

        private static void VerifyKnownGetterStoreDivergence(string key, KnownGetterStoreDivergence divergence, object value, object stored, SweepResult result)
        {
            if (AreSameValue(value, stored))
            {
                result.Fail($"{key}: known getter/GetValue divergence no longer reproduces (both return {Describe(value)}) - remove the KnownGetterStoreDivergences entry");
                return;
            }
            string getterType = value?.GetType().ToString() ?? "null";
            result.Check(getterType == divergence.GetterType, $"{key}: known divergence expected the getter to return {divergence.GetterType}, got {getterType}");
            if (divergence.Kind == GetterStoreDivergenceKind.EmptyStore)
            {
                result.Check(stored == null, $"{key}: known divergence expected GetValue to return null, got {Describe(stored)}");
            }
            else
            {
                string storedType = stored?.GetType().ToString() ?? "null";
                result.Check(storedType == divergence.GetterType,
                    $"{key}: known divergence expected GetValue to return a different {divergence.GetterType} instance, got {Describe(stored)}");
            }
        }

        private static void VerifyKnownSampleOutcome(string key, Type propertyType, KnownSampleOutcome outcome, object sample, object afterSample, object storedSample, SweepResult result)
        {
            object recordedSample = ParseSampleValue(propertyType, outcome.Sample);
            if (!AreSameValue(sample, recordedSample))
            {
                result.Fail($"{key}: KnownSampleOutcomes entry expects sample {outcome.Sample} but the sweep wrote {Describe(sample)} - update the entry");
                return;
            }
            object expectedGetter = ParseSampleValue(propertyType, outcome.GetterValue);
            object expectedStored = ParseSampleValue(propertyType, outcome.StoredValue);
            if (AreSameValue(afterSample, sample) && AreSameValue(storedSample, sample))
            {
                result.Fail($"{key}: known outcome no longer reproduces (setting {Describe(sample)} is now observed unchanged) - remove the KnownSampleOutcomes entry ({outcome.Reason})");
                return;
            }
            result.Check(AreSameValue(afterSample, expectedGetter), $"{key}: after setting {Describe(sample)} expected the getter to read {outcome.GetterValue}, read {Describe(afterSample)} ({outcome.Reason})");
            result.Check(AreSameValue(storedSample, expectedStored), $"{key}: after setting {Describe(sample)} expected GetValue to read {outcome.StoredValue}, read {Describe(storedSample)} ({outcome.Reason})");
        }

        private static object ParseSampleValue(Type propertyType, string text)
        {
            return propertyType.IsEnum ? Enum.Parse(propertyType, text) : Convert.ChangeType(text, propertyType, global::System.Globalization.CultureInfo.InvariantCulture);
        }

        // Value equality for projected values: native identity for WinRT objects, numeric equality across
        // enum/integer boxing differences, null and empty string equivalent (HSTRING has no null). The string
        // normalization applies only when both operands are a string or null; a string never equals a non-string value.
        private static bool AreSameValue(object a, object b)
        {
            if (a is string || b is string)
            {
                bool aIsStringOrNull = a == null || a is string;
                bool bIsStringOrNull = b == null || b is string;
                return aIsStringOrNull && bIsStringOrNull &&
                       string.Equals((string)a ?? string.Empty, (string)b ?? string.Empty, StringComparison.Ordinal);
            }
            if (a == null || b == null)
            {
                return a == null && b == null;
            }
            if (a.GetType().IsEnum || b.GetType().IsEnum)
            {
                return IsIntegral(a) && IsIntegral(b) && Convert.ToInt64(a) == Convert.ToInt64(b);
            }
            if (a.GetType().IsValueType || b.GetType().IsValueType)
            {
                if (a.GetType() != b.GetType() && IsNumeric(a) && IsNumeric(b))
                {
                    return Convert.ToDouble(a).Equals(Convert.ToDouble(b));
                }
                return a.Equals(b);
            }
            IntPtr identityA = InspectableAbiHelper.GetIdentity(a);
            IntPtr identityB = InspectableAbiHelper.GetIdentity(b);
            if (identityA != IntPtr.Zero || identityB != IntPtr.Zero)
            {
                return identityA == identityB;
            }
            return ReferenceEquals(a, b) || a.Equals(b);
        }

        private static bool IsIntegral(object value)
        {
            return value is Enum || value is int || value is uint || value is long || value is ulong || value is short || value is ushort || value is byte || value is sbyte;
        }

        private static bool IsNumeric(object value)
        {
            return IsIntegral(value) || value is double || value is float;
        }

        #endregion

        #region Event helpers

        private static Delegate CreateCountingHandler(Type delegateType, StrongBox<int> counter)
        {
            MethodInfo invoke = delegateType.GetMethod("Invoke");
            ParameterExpression[] parameters = invoke.GetParameters().Select(p => Expression.Parameter(p.ParameterType, p.Name)).ToArray();
            Expression increment = Expression.PreIncrementAssign(Expression.Field(Expression.Constant(counter), nameof(StrongBox<int>.Value)));
            Expression body = invoke.ReturnType == typeof(void)
                ? Expression.Block(typeof(void), increment)
                : Expression.Block(increment, Expression.Default(invoke.ReturnType));
            return Expression.Lambda(delegateType, body, parameters).Compile();
        }

        private static void VerifyRangeBaseValueChangedDeliveryStopsAfterRemoval(SweepResult result)
        {
            UIExecutor.Execute(() =>
            {
                var slider = new Slider { Minimum = 0, Maximum = 100, Value = 1 };
                int raised = 0;
                double lastNewValue = double.NaN;
                RangeBaseValueChangedEventHandler handler = (s, e) => { raised++; lastNewValue = e.NewValue; };

                slider.ValueChanged += handler;
                slider.Value = 5;
                result.Check(raised == 1 && lastNewValue == 5, $"Slider.ValueChanged: expected one event with NewValue 5, got {raised} with {lastNewValue}");

                slider.ValueChanged -= handler;
                slider.Value = 6;
                result.Check(raised == 1, $"Slider.ValueChanged: handler invoked after removal ({raised} events)");
            });
        }

        private static void VerifyToggleButtonCheckedDeliveryStopsAfterRemoval(SweepResult result)
        {
            UIExecutor.Execute(() =>
            {
                var checkBox = new CheckBox();
                int checkedCount = 0, uncheckedCount = 0;
                RoutedEventHandler onChecked = (s, e) => checkedCount++;
                RoutedEventHandler onUnchecked = (s, e) => uncheckedCount++;

                checkBox.Checked += onChecked;
                checkBox.Unchecked += onUnchecked;
                checkBox.IsChecked = true;
                checkBox.IsChecked = false;
                result.Check(checkedCount == 1 && uncheckedCount == 1, $"CheckBox: expected one Checked and one Unchecked, got {checkedCount}/{uncheckedCount}");

                checkBox.Checked -= onChecked;
                checkBox.Unchecked -= onUnchecked;
                checkBox.IsChecked = true;
                checkBox.IsChecked = false;
                result.Check(checkedCount == 1 && uncheckedCount == 1, $"CheckBox: handlers invoked after removal ({checkedCount}/{uncheckedCount})");
            });
        }

        private static void VerifyVisualStateGroupEventsDeliveryStopsAfterRemoval(SweepResult result)
        {
            Button templatedButton = null;
            UIExecutor.Execute(() =>
            {
                templatedButton = new Button { Content = "Sweep" };
                TestServices.WindowHelper.WindowContent = templatedButton;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var stateManager = new RaisingVisualStateManager();
                var group = new VisualStateGroup();
                var oldState = new VisualState();
                var newState = new VisualState();
                int changing = 0, changed = 0;
                VisualStateChangedEventArgs changingArgs = null;
                VisualStateChangedEventHandler onChanging = (s, e) => { changing++; changingArgs = e; };
                VisualStateChangedEventHandler onChanged = (s, e) => changed++;

                // Boundary: a control without an applied template is ignored and raises nothing.
                group.CurrentStateChanging += onChanging;
                group.CurrentStateChanged += onChanged;
                stateManager.RaiseChanging(group, oldState, newState, new Button());
                result.Check(changing == 0, $"VisualStateGroup.CurrentStateChanging raised for an untemplated control ({changing})");

                stateManager.RaiseChanging(group, oldState, newState, templatedButton);
                stateManager.RaiseChanged(group, oldState, newState, templatedButton);
                result.Check(changing == 1 && changed == 1, $"VisualStateGroup: expected one Changing and one Changed, got {changing}/{changed}");
                result.Check(changingArgs != null && AreSameValue(changingArgs.NewState, newState) && AreSameValue(changingArgs.Control, templatedButton),
                    "VisualStateGroup.CurrentStateChanging: event args do not carry the raised NewState/Control");

                // VisualStateManager.g.cpp validates every argument with ARG_NOTNULL (E_INVALIDARG).
                Exception missingNewState = CatchException(() => stateManager.RaiseChanging(group, oldState, null, templatedButton));
                result.Check(missingNewState != null && missingNewState.HResult == InspectableAbiHelper.E_INVALIDARG,
                    $"RaiseCurrentStateChanging(newState: null): expected E_INVALIDARG, got {DescribeException(missingNewState)}");
                Exception missingOldState = CatchException(() => stateManager.RaiseChanged(group, null, newState, templatedButton));
                result.Check(missingOldState != null && missingOldState.HResult == InspectableAbiHelper.E_INVALIDARG,
                    $"RaiseCurrentStateChanged(oldState: null): expected E_INVALIDARG, got {DescribeException(missingOldState)}");

                group.CurrentStateChanging -= onChanging;
                group.CurrentStateChanged -= onChanged;
                stateManager.RaiseChanging(group, oldState, newState, templatedButton);
                stateManager.RaiseChanged(group, oldState, newState, templatedButton);
                result.Check(changing == 1 && changed == 1, $"VisualStateGroup: handlers invoked after removal ({changing}/{changed})");
            });
        }

        #endregion

        #region Enum and parser helpers

        private static IEnumerable<EnumHost> GetEnumHosts(InstancePool pool)
        {
            var hosts = new Dictionary<Type, EnumHost>();
            foreach (SweepTarget target in pool.SweepTargets)
            {
                Type elementType = target.Instance.GetType();
                if (!Catalog.IsXamlElementNameUnique(elementType))
                {
                    continue;
                }
                foreach (PropertyInfo property in Catalog.GetInstanceProperties(target.DeclaringType))
                {
                    Type enumType = property.PropertyType;
                    string key = $"{target.DeclaringType.FullName}.{property.Name}";
                    if (!enumType.IsEnum || hosts.ContainsKey(enumType) || property.SetMethod == null || !property.SetMethod.IsPublic ||
                        Catalog.FindDependencyProperty(target.DeclaringType, property.Name) == null ||
                        ApiSurfaceAllowList.PropertyWriteExclusions.ContainsKey(key) || ApiSurfaceAllowList.HasExpectedFailure(key, SweepOperation.Get) || ApiSurfaceAllowList.HasExpectedFailure(key, SweepOperation.Set))
                    {
                        continue;
                    }
                    hosts.Add(enumType, new EnumHost(elementType, property, key));
                }
            }
            return hosts.Values.OrderBy(h => h.Property.PropertyType.FullName, StringComparer.Ordinal).ToList();
        }

        private static string EnumAttributeXaml(EnumHost host, string text)
        {
            return $"<{host.ElementType.Name} xmlns='{PresentationXmlns}' {host.Property.Name}='{text}'/>";
        }

        private static void VerifyEnumAttribute(EnumHost host, string text, object expected, SweepResult result)
        {
            object parsed = null;
            if (!result.Invoke($"{host.Key}={text}", SweepOperation.Parse, () => parsed = XamlReader.Load(EnumAttributeXaml(host, text))))
            {
                return;
            }
            if (!result.Check(parsed != null && parsed.GetType() == host.ElementType, $"{host.Key}=\"{text}\": parsed {parsed?.GetType().FullName ?? "null"}"))
            {
                return;
            }
            object actual = host.Property.GetValue(parsed);
            if (ApiSurfaceAllowList.KnownPropertyStoreSplits.TryGetValue(host.Key, out KnownPropertyStoreSplit split))
            {
                VerifyKnownPropertyStoreSplit(host, split, (DependencyObject)parsed, text, expected, actual, result);
                return;
            }
            result.Check(AreSameValue(actual, expected), $"{host.Key}=\"{text}\": expected {Describe(expected)}, read {Describe(actual)}");
        }

        // The XAML value must land in the DependencyProperty store while the getter keeps returning its private field.
        private static void VerifyKnownPropertyStoreSplit(EnumHost host, KnownPropertyStoreSplit split, DependencyObject parsed, string text, object expected, object actual, SweepResult result)
        {
            string label = $"{host.Key}=\"{text}\" [{split.ConcernId}]";
            DependencyProperty dependencyProperty = Catalog.FindDependencyProperty(host.ElementType, host.Property.Name);
            if (!result.Check(dependencyProperty != null, $"{label}: {host.Property.Name}Property identifier not found"))
            {
                return;
            }
            object stored = parsed.GetValue(dependencyProperty);
            result.Check(AreSameValue(stored, expected), $"{label}: GetValue({host.Property.Name}Property) expected {Describe(expected)}, read {Describe(stored)}");

            object getterValue = Enum.Parse(host.Property.PropertyType, split.GetterValue);
            if (AreSameValue(expected, getterValue))
            {
                result.Check(AreSameValue(actual, expected), $"{label}: expected {Describe(expected)}, read {Describe(actual)}");
            }
            else if (AreSameValue(actual, expected))
            {
                result.Fail($"{label}: known defect no longer reproduces (getter returns the XAML value) - remove the KnownPropertyStoreSplits entry");
            }
            else
            {
                result.Check(AreSameValue(actual, getterValue), $"{label}: known defect expected the getter to keep returning {Describe(getterValue)}, read {Describe(actual)}");
            }
        }

        private static int VerifyEnumObjectElements(Type enumType, SweepResult result)
        {
            string[] names = Enum.GetNames(enumType);
            string entries = string.Concat(names.Select((name, i) => $"<{enumType.Name} x:Key='k{i}'>{name}</{enumType.Name}>"));
            string xaml = $"<ResourceDictionary xmlns='{PresentationXmlns}' xmlns:x='{XamlXmlns}'>{entries}</ResourceDictionary>";
            ResourceDictionary dictionary = null;
            if (!result.Invoke(enumType.FullName + " object elements", SweepOperation.Parse, () => dictionary = (ResourceDictionary)XamlReader.Load(xaml)))
            {
                return 0;
            }
            for (int i = 0; i < names.Length; i++)
            {
                object expected = Enum.Parse(enumType, names[i]);
                object actual = dictionary.TryGetValue($"k{i}", out object value) ? value : null;
                result.Check(AreSameValue(actual, expected), $"<{enumType.Name}>{names[i]}</{enumType.Name}>: expected {Describe(expected)}, read {Describe(actual)}");
            }
            return names.Length;
        }

        private static void VerifyParserObjectElements(SweepResult result)
        {
            foreach (Type type in Catalog.ActivatableClasses)
            {
                if (!typeof(DependencyObject).IsAssignableFrom(type))
                {
                    result.Exclude(type.FullName, "Not a DependencyObject, so not a XAML object element.");
                    continue;
                }
                if (ApiSurfaceAllowList.InstanceSweepExclusions.TryGetValue(type.FullName, out string reason))
                {
                    result.Exclude(type.FullName, reason);
                    continue;
                }
                if (!Catalog.IsXamlElementNameUnique(type))
                {
                    result.Exclude(type.FullName, "Short name is shared by another Microsoft.UI.Xaml type in the default XAML namespace.");
                    continue;
                }
                UIExecutor.Execute(() =>
                {
                    object parsed = null;
                    if (result.Invoke(type.FullName + " object element", SweepOperation.Parse, () => parsed = XamlReader.Load($"<{type.Name} xmlns='{PresentationXmlns}'/>")))
                    {
                        result.Check(parsed != null && parsed.GetType() == type, $"<{type.Name}/>: parsed {parsed?.GetType().FullName ?? "null"}, expected {type.FullName}");
                    }
                });
            }
        }

        private static void VerifySimplePropertiesFromXaml(SweepResult result)
        {
            UIExecutor.Execute(() =>
            {
                VerifySimpleAttribute<Grid>("Translation='1,2,3'", g => g.Translation, new Vector3(1, 2, 3), result);
                VerifySimpleAttribute<Grid>("Rotation='45'", g => g.Rotation, 45f, result);
                VerifySimpleAttribute<Grid>("Scale='2,3,4'", g => g.Scale, new Vector3(2, 3, 4), result);
                VerifySimpleAttribute<Grid>("CenterPoint='5,6,7'", g => g.CenterPoint, new Vector3(5, 6, 7), result);
                VerifySimpleAttribute<Grid>("RotationAxis='0,1,0'", g => g.RotationAxis, new Vector3(0, 1, 0), result);
                VerifySimpleAttribute<Grid>("TransformMatrix='1,0,0,0,0,1,0,0,0,0,1,0,10,20,30,1'", g => g.TransformMatrix,
                    new Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 20, 30, 1), result);
                VerifySimpleAttribute<Grid>("RasterizationScale='2'", g => g.RasterizationScale, 2.0, result);

                if (IsPublicSimpleProperty<LinearGradientBrush>("CenterPoint", result))
                {
                    VerifySimpleAttribute<LinearGradientBrush>("CenterPoint='0.5,0.25'", b => b.CenterPoint, new Vector2(0.5f, 0.25f), result);
                }
                if (IsPublicSimpleProperty<LinearGradientBrush>("Rotation", result))
                {
                    VerifySimpleAttribute<LinearGradientBrush>("Rotation='30'", b => b.Rotation, 30.0, result);
                }
                if (IsPublicSimpleProperty<LinearGradientBrush>("Scale", result))
                {
                    VerifySimpleAttribute<LinearGradientBrush>("Scale='2,3'", b => b.Scale, new Vector2(2, 3), result);
                }
                if (IsPublicSimpleProperty<LinearGradientBrush>("Translation", result))
                {
                    VerifySimpleAttribute<LinearGradientBrush>("Translation='4,5'", b => b.Translation, new Vector2(4, 5), result);
                }
                if (IsPublicSimpleProperty<LinearGradientBrush>("TransformMatrix", result))
                {
                    VerifySimpleAttribute<LinearGradientBrush>("TransformMatrix='1,0,0,1,6,7'", b => b.TransformMatrix, new Matrix3x2(1, 0, 0, 1, 6, 7), result);
                }

                VerifySimpleElement<Grid>("<Grid.OpacityTransition><ScalarTransition Duration='0:0:2'/></Grid.OpacityTransition>",
                    g => g.OpacityTransition?.Duration, TimeSpan.FromSeconds(2), result);
                VerifySimpleElement<Grid>("<Grid.BackgroundTransition><BrushTransition Duration='0:0:4'/></Grid.BackgroundTransition>",
                    g => g.BackgroundTransition?.Duration, TimeSpan.FromSeconds(4), result);
                VerifySimpleElement<Grid>("<Grid.TranslationTransition><Vector3Transition Duration='0:0:3' Components='X,Z'/></Grid.TranslationTransition>",
                    g => g.TranslationTransition?.Duration, TimeSpan.FromSeconds(3), result);
                VerifySimpleElement<Grid>("<Grid.TranslationTransition><Vector3Transition Components='X,Z'/></Grid.TranslationTransition>",
                    g => g.TranslationTransition?.Components, Vector3TransitionComponents.X | Vector3TransitionComponents.Z, result);

                // A Vector3 simple property given two components is malformed and must fail the parse.
                Exception malformed = CatchException(() => XamlReader.Load($"<Grid xmlns='{PresentationXmlns}' Translation='1,2'/>"));
                result.Check(malformed != null && malformed.HResult == InspectableAbiHelper.E_XAMLPARSEFAILED,
                    $"<Grid Translation='1,2'/>: expected E_XAMLPARSEFAILED, got {DescribeException(malformed)}");
            });
        }

        private static bool IsPublicSimpleProperty<T>(string propertyName, SweepResult result)
        {
            if (Catalog.IsSweepableMember(typeof(T), propertyName))
            {
                return true;
            }
            result.Exclude($"{typeof(T).FullName}.{propertyName}", "Not part of the public projection (feature-gated API).");
            return false;
        }

        private static void VerifySimpleAttribute<T>(string attribute, Func<T, object> read, object expected, SweepResult result)
        {
            VerifySimpleXaml($"<{typeof(T).Name} xmlns='{PresentationXmlns}' {attribute}/>", read, expected, result);
        }

        private static void VerifySimpleElement<T>(string content, Func<T, object> read, object expected, SweepResult result)
        {
            VerifySimpleXaml($"<{typeof(T).Name} xmlns='{PresentationXmlns}'>{content}</{typeof(T).Name}>", read, expected, result);
        }

        private static void VerifySimpleXaml<T>(string xaml, Func<T, object> read, object expected, SweepResult result)
        {
            object parsed = null;
            if (!result.Invoke(xaml, SweepOperation.Parse, () => parsed = XamlReader.Load(xaml)))
            {
                return;
            }
            if (result.Check(parsed is T, $"{xaml}: parsed {parsed?.GetType().FullName ?? "null"}"))
            {
                object actual = read((T)parsed);
                result.Check(AreSameValue(actual, expected), $"{xaml}: expected {Describe(expected)}, read {Describe(actual)}");
            }
        }

        // Strict-only simple properties lock an object into strict mode. A later non-strict API is allowed only when
        // every strict-only simple property is back at its default value, otherwise it fails with E_ACCESSDENIED.
        private static void VerifyStrictOnlySimpleProperties(SweepResult result)
        {
            UIExecutor.Execute(() =>
            {
                var defaults = new Grid();
                // UIElement.Translation is not strict-only (UIElementGenerated::put_Translation performs no StrictOnlyApiCheck),
                // so a non-default Translation must not block the non-strict RenderTransform.
                var translated = new Grid { Translation = new Vector3(1, 0, 0) };
                var renderTransform = new TranslateTransform();
                Exception coexist = CatchException(() => translated.RenderTransform = renderTransform);
                result.Check(coexist == null && AreSameValue(translated.RenderTransform, renderTransform) && translated.Translation == new Vector3(1, 0, 0),
                    $"UIElement.Translation non-default then RenderTransform: expected both to apply, got {DescribeException(coexist)}");
                VerifyStrictness<Grid>("UIElement.Rotation", g => g.Rotation = 30, g => g.Rotation = defaults.Rotation, g => g.RenderTransform = new TranslateTransform(), result);
                VerifyStrictness<Grid>("UIElement.Scale", g => g.Scale = new Vector3(2, 2, 2), g => g.Scale = defaults.Scale, g => g.RenderTransform = new TranslateTransform(), result);
                VerifyStrictness<Grid>("UIElement.CenterPoint", g => g.CenterPoint = new Vector3(3, 3, 0), g => g.CenterPoint = defaults.CenterPoint, g => g.RenderTransform = new TranslateTransform(), result);
                VerifyStrictness<Grid>("UIElement.RotationAxis", g => g.RotationAxis = new Vector3(1, 0, 0), g => g.RotationAxis = defaults.RotationAxis, g => g.RenderTransform = new TranslateTransform(), result);
                VerifyStrictness<Grid>("UIElement.TransformMatrix", g => g.TransformMatrix = Matrix4x4.CreateTranslation(5, 0, 0), g => g.TransformMatrix = defaults.TransformMatrix, g => g.RenderTransform = new TranslateTransform(), result);

                // LinearGradientBrush simple properties are feature-gated (Feature_XamlMotionSystemHoldbacks) and only
                // exercised when they are part of the public projection.
                var brushDefaults = new LinearGradientBrush();
                if (IsPublicSimpleProperty<LinearGradientBrush>("Rotation", result))
                {
                    VerifyStrictness<LinearGradientBrush>("LinearGradientBrush.Rotation", b => b.Rotation = 30, b => b.Rotation = brushDefaults.Rotation, b => b.Transform = new RotateTransform(), result);
                }
                if (IsPublicSimpleProperty<LinearGradientBrush>("CenterPoint", result))
                {
                    VerifyStrictness<LinearGradientBrush>("LinearGradientBrush.CenterPoint", b => b.CenterPoint = new Vector2(2, 2), b => b.CenterPoint = brushDefaults.CenterPoint, b => b.Transform = new RotateTransform(), result);
                }
                if (IsPublicSimpleProperty<LinearGradientBrush>("Scale", result))
                {
                    VerifyStrictness<LinearGradientBrush>("LinearGradientBrush.Scale", b => b.Scale = new Vector2(3, 3), b => b.Scale = brushDefaults.Scale, b => b.Transform = new RotateTransform(), result);
                }
                if (IsPublicSimpleProperty<LinearGradientBrush>("Translation", result))
                {
                    VerifyStrictness<LinearGradientBrush>("LinearGradientBrush.Translation", b => b.Translation = new Vector2(4, 4), b => b.Translation = brushDefaults.Translation, b => b.Transform = new RotateTransform(), result);
                }
                if (IsPublicSimpleProperty<LinearGradientBrush>("TransformMatrix", result))
                {
                    VerifyStrictness<LinearGradientBrush>("LinearGradientBrush.TransformMatrix", b => b.TransformMatrix = Matrix3x2.CreateTranslation(5, 5), b => b.TransformMatrix = brushDefaults.TransformMatrix, b => b.Transform = new RotateTransform(), result);
                }
            });
        }

        private static void VerifyStrictness<T>(string label, Action<T> setNonDefault, Action<T> restoreDefault, Action<T> useNonStrictApi, SweepResult result) where T : new()
        {
            var inUse = new T();
            setNonDefault(inUse);
            Exception denied = CatchException(() => useNonStrictApi(inUse));
            result.Check(denied != null && denied.HResult == InspectableAbiHelper.E_ACCESSDENIED,
                $"{label} non-default then non-strict API: expected E_ACCESSDENIED, got {DescribeException(denied)}");

            var restored = new T();
            setNonDefault(restored);
            restoreDefault(restored);
            Exception allowed = CatchException(() => useNonStrictApi(restored));
            result.Check(allowed == null, $"{label} restored to default then non-strict API: expected success, got {DescribeException(allowed)}");
        }

        private static int VerifyAutomationPeersForElements(SweepResult result)
        {
            int created = 0;
            foreach (Type type in Catalog.ActivatableClasses.Where(t => typeof(FrameworkElement).IsAssignableFrom(t)))
            {
                if (ApiSurfaceAllowList.InstanceSweepExclusions.ContainsKey(type.FullName))
                {
                    continue;
                }
                UIExecutor.Execute(() =>
                {
                    var element = Activate(type, result) as FrameworkElement;
                    if (element == null)
                    {
                        return;
                    }
                    AutomationPeer peer = null;
                    if (!result.Invoke(type.FullName, SweepOperation.CreatePeer, () => peer = FrameworkElementAutomationPeer.CreatePeerForElement(element)))
                    {
                        return;
                    }
                    if (peer == null)
                    {
                        result.Exclude(type.FullName, "Element does not create an automation peer.");
                        return;
                    }
                    created++;
                    result.Check(AreSameValue(FrameworkElementAutomationPeer.FromElement(element), peer), $"{type.FullName}: FromElement did not return the created peer");
                    VerifyInspectable(peer, null, null, result, type.FullName + " peer", verifyIidsQueryable: true, failedQueryClearsOutPointer: true);

                    IntPtr inspectable = InspectableAbiHelper.GetInspectable(peer);
                    InspectableAbiHelper.GetRuntimeClassName(inspectable, out string peerClassName);
                    InspectableAbiHelper.Release(inspectable);
                    result.Check(peerClassName != null && peerClassName.StartsWith("Microsoft.UI.Xaml.Automation.Peers.", StringComparison.Ordinal) &&
                                 peerClassName.EndsWith("AutomationPeer", StringComparison.Ordinal),
                        $"{type.FullName}: peer runtime class name \"{peerClassName}\" is not a Microsoft.UI.Xaml automation peer");
                });
            }
            return created;
        }

        #endregion

        #region Shared utilities

        private static Exception CatchException(Action action)
        {
            try
            {
                action();
                return null;
            }
            catch (TargetInvocationException e) when (e.InnerException != null)
            {
                return e.InnerException;
            }
            catch (Exception e)
            {
                return e;
            }
        }

        private static string DescribeException(Exception e)
        {
            return e == null ? "no exception" : $"{e.GetType().Name} 0x{e.HResult:X8} ({e.Message})";
        }

        private static string Describe(object value)
        {
            return value == null ? "null" : $"{value} ({value.GetType().Name})";
        }

        // Counts per-category outcomes. Expected failures must match the allow-listed exception and HRESULT exactly; any
        // other exception, an allow-listed member that unexpectedly succeeds, and an allow-listed entry owned by this
        // sweep that the sweep never reaches are recorded as failures.
        private sealed class SweepResult
        {
            private readonly string _category;
            private readonly List<string> _failures = new List<string>();
            private readonly HashSet<string> _visitedExpectations = new HashSet<string>(StringComparer.Ordinal);
            private int _passed;
            private int _expectedFailures;
            private int _excluded;
            private int _minimumPassed;

            public SweepResult(string category)
            {
                _category = category;
            }

            public void Pass() => _passed++;

            public void Fail(string message) => _failures.Add(message);

            public void Exclude(string member, string reason)
            {
                _excluded++;
                Log.Comment($"[{_category}] not exercised: {member} - {reason}");
            }

            public bool Check(bool condition, string failureMessage)
            {
                if (condition)
                {
                    _passed++;
                }
                else
                {
                    _failures.Add(failureMessage);
                }
                return condition;
            }

            // Runs one operation on one member. The allow-list is consulted with the operation-qualified key
            // "member:operation", so each operation (get, set, set-sample, ...) has its own expectation.
            public bool Invoke(string memberKey, string operation, Action action)
            {
                string operationKey = ApiSurfaceAllowList.OperationKey(memberKey, operation);
                Exception error = CatchException(action);
                bool allowListed = ApiSurfaceAllowList.TryGetExpectedFailure(memberKey, operation, out ExpectedFailure expected);
                if (allowListed)
                {
                    _visitedExpectations.Add(operationKey);
                }
                if (error == null)
                {
                    if (allowListed)
                    {
                        _failures.Add($"{operationKey}: allow-listed failure {expected} no longer occurs");
                        return false;
                    }
                    return true;
                }
                if (allowListed && expected.Matches(error))
                {
                    _expectedFailures++;
                    return false;
                }
                _failures.Add(allowListed
                    ? $"{operationKey}: expected allow-listed {expected}, got {DescribeException(error)}"
                    : $"{operationKey}: unexpected {DescribeException(error)}");
                return false;
            }

            public void Report(int minimumPassed, bool verify = true)
            {
                _minimumPassed = minimumPassed;
                // An expectation that this sweep owns but never reached (renamed/removed member, skipped operation or
                // changed key) is stale and must not silently remain in the allow-list.
                foreach (KeyValuePair<string, ExpectedFailure> entry in ApiSurfaceAllowList.ExpectedFailures)
                {
                    if (entry.Value.Sweep == _category && !_visitedExpectations.Contains(entry.Key))
                    {
                        _failures.Add($"{entry.Key}: allow-listed failure {entry.Value} was never exercised by this sweep - remove or update the ExpectedFailures entry");
                    }
                }
                Log.Comment($"[{_category}] passed={_passed} expectedFailures={_expectedFailures} notExercised={_excluded} failures={_failures.Count}");
                foreach (string failure in _failures)
                {
                    Log.Comment($"[{_category}] FAILURE: {failure}");
                }
                if (verify)
                {
                    VerifyAll(this);
                }
            }

            public static void VerifyAll(params SweepResult[] results)
            {
                foreach (SweepResult result in results)
                {
                    Verify.AreEqual(0, result._failures.Count, $"[{result._category}] unexpected failures (see FAILURE comments above).");
                    Verify.IsTrue(result._passed >= result._minimumPassed, $"[{result._category}] expected at least {result._minimumPassed} passing checks, got {result._passed}.");
                }
            }
        }

        private sealed class SweepTarget
        {
            public SweepTarget(Type declaringType, object instance)
            {
                DeclaringType = declaringType;
                Instance = instance;
            }

            public Type DeclaringType { get; }
            public object Instance { get; }
        }

        private sealed class EnumHost
        {
            public EnumHost(Type elementType, PropertyInfo property, string key)
            {
                ElementType = elementType;
                Property = property;
                Key = key;
            }

            public Type ElementType { get; }
            public PropertyInfo Property { get; }
            public string Key { get; }
        }

        private sealed class AttachedProperty
        {
            public string Key { get; set; }
            public MethodInfo Getter { get; set; }
            public MethodInfo Setter { get; set; }
            public DependencyProperty Property { get; set; }
            public Type TargetType { get; set; }
            public Type ValueType { get; set; }
        }

        // One fresh, detached instance of every default-activatable class, plus the mapping from each projected
        // class in their hierarchies to the shallowest instance that can exercise its declared members.
        private sealed class InstancePool : IDisposable
        {
            private readonly List<object> _instances = new List<object>();

            public InstancePool(SweepResult result)
            {
                var visited = new HashSet<Type>();
                var targets = new List<SweepTarget>();
                foreach (Type type in Catalog.ActivatableClasses.OrderBy(Catalog.Depth).ThenBy(t => t.FullName, StringComparer.Ordinal))
                {
                    if (ApiSurfaceAllowList.InstanceSweepExclusions.TryGetValue(type.FullName, out string reason))
                    {
                        result.Exclude(type.FullName, reason);
                        continue;
                    }
                    UIExecutor.Execute(() =>
                    {
                        object instance = Activate(type, result);
                        if (instance == null)
                        {
                            return;
                        }
                        _instances.Add(instance);
                        for (Type current = type; current != null && Catalog.IsProjectedClass(current); current = current.BaseType)
                        {
                            if (visited.Add(current))
                            {
                                targets.Add(new SweepTarget(current, instance));
                            }
                        }
                    });
                }
                SweepTargets = targets;
            }

            public IReadOnlyList<SweepTarget> SweepTargets { get; }

            public object GetInstanceAssignableTo(Type type)
            {
                return _instances.FirstOrDefault(type.IsInstanceOfType);
            }

            public void Dispose()
            {
                UIExecutor.Execute(() => _instances.Clear());
            }
        }

        // Reflection catalog over the projection assembly, filtered to classes Microsoft.UI.Xaml.dll serves.
        private static class Catalog
        {
            private const string PublicProjectionRelativePath = @"PrivateAPITests\Microsoft.WinUI.dll";
            private const string ClassRegistrationManifestFileName = "AppXManifest.xml";

            private static readonly Lazy<Type[]> s_projectedTypes = new Lazy<Type[]>(LoadProjectedTypes);
            private static readonly Lazy<Dictionary<string, HashSet<string>>> s_publicSurface = new Lazy<Dictionary<string, HashSet<string>>>(LoadPublicSurface);
            private static readonly Lazy<Dictionary<string, string>> s_classRegistrations = new Lazy<Dictionary<string, string>>(LoadClassRegistrations);
            private static readonly Dictionary<Type, HashSet<string>> s_featureGatedMembers = new Dictionary<Type, HashSet<string>>();
            private static readonly Lazy<IReadOnlyList<Type>> s_muxRuntimeClasses = new Lazy<IReadOnlyList<Type>>(LoadMuxRuntimeClasses);
            private static readonly List<KeyValuePair<string, int>> s_unservedClasses = new List<KeyValuePair<string, int>>();
            private static readonly HashSet<string> s_probedClasses = new HashSet<string>(StringComparer.Ordinal);
            private static readonly List<string> s_ownershipFailures = new List<string>();

            // Public projected classes for which Microsoft.UI.Xaml.dll!DllGetActivationFactory failed, with its HRESULT.
            public static IReadOnlyList<KeyValuePair<string, int>> UnservedClasses
            {
                get
                {
                    IReadOnlyList<Type> unused = s_muxRuntimeClasses.Value;
                    return s_unservedClasses;
                }
            }

            // Public projected classes passed to DllGetActivationFactory (every public class except FactoryProbeExclusions).
            public static IReadOnlyCollection<string> ProbedClasses
            {
                get
                {
                    IReadOnlyList<Type> unused = s_muxRuntimeClasses.Value;
                    return s_probedClasses;
                }
            }

            // Disagreements between what Microsoft.UI.Xaml.dll serves and which classes it owns (see IsOwnedByMux).
            public static IReadOnlyList<string> OwnershipFailures
            {
                get
                {
                    IReadOnlyList<Type> unused = s_muxRuntimeClasses.Value;
                    return s_ownershipFailures;
                }
            }

            // Activatable class id -> InProcessServer DLL file name, from the deployed class registration manifest.
            public static IReadOnlyDictionary<string, string> ClassRegistrations => s_classRegistrations.Value;
            private static readonly Lazy<HashSet<string>> s_ambiguousNames = new Lazy<HashSet<string>>(() =>
                new HashSet<string>(s_projectedTypes.Value.Where(IsProjectedClass).GroupBy(t => t.Name).Where(g => g.Count() > 1).Select(g => g.Key)));

            public static IReadOnlyList<Type> MuxRuntimeClasses => s_muxRuntimeClasses.Value;

            public static IReadOnlyList<Type> ActivatableClasses => MuxRuntimeClasses
                .Where(t => !t.IsAbstract && t.GetConstructor(Type.EmptyTypes) != null && !ApiSurfaceAllowList.ActivationExclusions.ContainsKey(t.FullName))
                .ToList();

            public static bool IsProjectedClass(Type type)
            {
                return type.IsClass && type.IsPublic && !type.IsGenericType && !typeof(Delegate).IsAssignableFrom(type) &&
                       type.Namespace != null && type.Namespace.StartsWith("Microsoft.UI.Xaml", StringComparison.Ordinal) &&
                       type.CustomAttributes.Any(a => a.AttributeType.FullName == "WinRT.WindowsRuntimeTypeAttribute") &&
                       s_publicSurface.Value.ContainsKey(type.FullName);
            }

            // True when the member is in the public projection and not declared by a feature-gated interface.
            public static bool IsSweepableMember(Type type, string memberName)
            {
                return s_publicSurface.Value.TryGetValue(type.FullName, out HashSet<string> members) && members.Contains(memberName) &&
                       !GetFeatureGatedMembers(type).Contains(memberName);
            }

            // WinRT IID, including parameterized IIDs for generic interfaces such as IList`1 (IVector<T>).
            public static Guid GetWinRTIid(Type interfaceType) => interfaceType.IsGenericType
                ? global::WinRT.GuidGenerator.CreateIID(interfaceType)
                : global::WinRT.GuidGenerator.GetIID(interfaceType);

            private static HashSet<string> GetFeatureGatedMembers(Type type)
            {
                lock (s_featureGatedMembers)
                {
                    if (!s_featureGatedMembers.TryGetValue(type, out HashSet<string> members))
                    {
                        var pattern = new Regex("^I" + Regex.Escape(type.Name) + "(Statics|Factory|Overrides|Protected)?[0-9]*Feature_");
                        members = new HashSet<string>(StringComparer.Ordinal);
                        foreach (Type featureInterface in s_projectedTypes.Value.Where(t => t.IsInterface && t.Namespace == type.Namespace && pattern.IsMatch(t.Name)))
                        {
                            foreach (MemberInfo member in featureInterface.GetMembers(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly))
                            {
                                members.Add(member.Name);
                            }
                        }
                        s_featureGatedMembers.Add(type, members);
                    }
                    return members;
                }
            }

            public static int Depth(Type type)
            {
                int depth = 0;
                for (Type current = type.BaseType; current != null; current = current.BaseType)
                {
                    depth++;
                }
                return depth;
            }

            public static bool IsXamlElementNameUnique(Type type) => !s_ambiguousNames.Value.Contains(type.Name);

            public static Type GetDefaultInterface(Type type)
            {
                CustomAttributeData attribute = type.CustomAttributes.FirstOrDefault(a => a.AttributeType.FullName == "WinRT.ProjectedRuntimeClassAttribute");
                return attribute?.ConstructorArguments.Select(a => a.Value).OfType<Type>().FirstOrDefault(t => t.IsInterface);
            }

            public static IEnumerable<Type> GetStaticsAndFactoryInterfaces(Type type)
            {
                var pattern = new Regex("^I" + Regex.Escape(type.Name) + "(Statics|Factory)[0-9]*$");
                return s_projectedTypes.Value.Where(t => t.IsInterface && t.Namespace == type.Namespace && pattern.IsMatch(t.Name) &&
                                                         t.IsDefined(typeof(global::System.Runtime.InteropServices.GuidAttribute), false));
            }

            public static IEnumerable<PropertyInfo> GetInstanceProperties(Type declaringType)
            {
                return declaringType.GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly)
                    .Where(p => p.CanRead && p.GetMethod.IsPublic && p.GetIndexParameters().Length == 0 && IsSweepableMember(declaringType, p.Name))
                    .OrderBy(p => p.Name, StringComparer.Ordinal);
            }

            public static DependencyProperty FindDependencyProperty(Type declaringType, string propertyName)
            {
                for (Type current = declaringType; current != null && current != typeof(object); current = current.BaseType)
                {
                    PropertyInfo identifier = current.GetProperty(propertyName + "Property", BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly);
                    if (identifier != null && identifier.PropertyType == typeof(DependencyProperty) && IsSweepableMember(current, identifier.Name))
                    {
                        return (DependencyProperty)identifier.GetValue(null);
                    }
                }
                return null;
            }

            public static IEnumerable<AttachedProperty> GetAttachedProperties(Type type)
            {
                MethodInfo[] methods = type.GetMethods(BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly);
                foreach (MethodInfo getter in methods.Where(m => m.Name.StartsWith("Get", StringComparison.Ordinal) && m.ReturnType != typeof(void)).OrderBy(m => m.Name, StringComparer.Ordinal))
                {
                    ParameterInfo[] parameters = getter.GetParameters();
                    if (parameters.Length != 1 || !typeof(DependencyObject).IsAssignableFrom(parameters[0].ParameterType))
                    {
                        continue;
                    }
                    string name = getter.Name.Substring(3);
                    PropertyInfo identifier = type.GetProperty(name + "Property", BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly);
                    if (identifier == null || identifier.PropertyType != typeof(DependencyProperty) ||
                        !IsSweepableMember(type, getter.Name) || !IsSweepableMember(type, identifier.Name))
                    {
                        continue;
                    }
                    MethodInfo setter = methods.FirstOrDefault(m => m.Name == "Set" + name && IsSweepableMember(type, m.Name) && m.GetParameters().Length == 2 &&
                                                                    m.GetParameters()[0].ParameterType == parameters[0].ParameterType &&
                                                                    m.GetParameters()[1].ParameterType == getter.ReturnType);
                    yield return new AttachedProperty
                    {
                        Key = $"{type.FullName}.{name}",
                        Getter = getter,
                        Setter = setter,
                        Property = (DependencyProperty)identifier.GetValue(null),
                        TargetType = parameters[0].ParameterType,
                        ValueType = getter.ReturnType,
                    };
                }
            }

            // Type full name -> public member names, read from the public projection's metadata without loading it.
            private static Dictionary<string, HashSet<string>> LoadPublicSurface()
            {
                string directory = Path.GetDirectoryName(typeof(GeneratedApiSurfaceSweepTests).Assembly.Location);
                string path = Path.Combine(directory, PublicProjectionRelativePath);
                if (!File.Exists(path))
                {
                    throw new FileNotFoundException("Public WinUI projection (deployed by test\\CreateTestPayload.ps1) was not found.", path);
                }

                var surface = new Dictionary<string, HashSet<string>>(StringComparer.Ordinal);
                using (FileStream stream = File.OpenRead(path))
                using (var peReader = new PEReader(stream))
                {
                    MetadataReader reader = peReader.GetMetadataReader();
                    foreach (TypeDefinitionHandle handle in reader.TypeDefinitions)
                    {
                        TypeDefinition definition = reader.GetTypeDefinition(handle);
                        if ((definition.Attributes & TypeAttributes.VisibilityMask) != TypeAttributes.Public)
                        {
                            continue;
                        }
                        var members = new HashSet<string>(StringComparer.Ordinal);
                        foreach (PropertyDefinitionHandle property in definition.GetProperties())
                        {
                            members.Add(reader.GetString(reader.GetPropertyDefinition(property).Name));
                        }
                        foreach (EventDefinitionHandle eventHandle in definition.GetEvents())
                        {
                            members.Add(reader.GetString(reader.GetEventDefinition(eventHandle).Name));
                        }
                        foreach (MethodDefinitionHandle method in definition.GetMethods())
                        {
                            MethodDefinition methodDefinition = reader.GetMethodDefinition(method);
                            if ((methodDefinition.Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Public)
                            {
                                members.Add(reader.GetString(methodDefinition.Name));
                            }
                        }
                        surface[reader.GetString(definition.Namespace) + "." + reader.GetString(definition.Name)] = members;
                    }
                }
                Log.Comment($"[Catalog] public projection {path}: {surface.Count} public types.");
                return surface;
            }

            private static Type[] LoadProjectedTypes()
            {
                try
                {
                    return typeof(UIElement).Assembly.GetTypes();
                }
                catch (ReflectionTypeLoadException e)
                {
                    return e.Types.Where(t => t != null).ToArray();
                }
            }

            private static IReadOnlyList<Type> LoadMuxRuntimeClasses()
            {
                var classes = new List<Type>();
                UIExecutor.Execute(() =>
                {
                    foreach (Type type in s_projectedTypes.Value.Where(IsProjectedClass).OrderBy(t => t.FullName, StringComparer.Ordinal))
                    {
                        if (ApiSurfaceAllowList.FactoryProbeExclusions.TryGetValue(type.FullName, out string reason))
                        {
                            Log.Comment($"[Catalog] factory not probed: {type.FullName} - {reason}");
                            continue;
                        }
                        s_probedClasses.Add(type.FullName);
                        bool owned = IsOwnedByMux(type.FullName, out string ownership);
                        int hr = InspectableAbiHelper.GetMuxActivationFactory(type.FullName, out IntPtr factory);
                        if (hr == 0)
                        {
                            classes.Add(type);
                            if (!owned)
                            {
                                s_ownershipFailures.Add($"{type.FullName}: served by Microsoft.UI.Xaml.dll but {ownership}; if Microsoft.UI.Xaml.dll owns this " +
                                                        "non-activatable class, add it to ApiSurfaceAllowList.KnownUnregisteredMuxClasses");
                            }
                        }
                        else
                        {
                            Log.Comment($"[Catalog] not served by Microsoft.UI.Xaml.dll: {type.FullName} - DllGetActivationFactory 0x{hr:X8} ({ownership})");
                            s_unservedClasses.Add(new KeyValuePair<string, int>(type.FullName, hr));
                            if (owned)
                            {
                                s_ownershipFailures.Add($"{type.FullName}: {ownership} but Microsoft.UI.Xaml.dll!DllGetActivationFactory returned 0x{hr:X8}");
                            }
                        }
                        InspectableAbiHelper.Release(factory);
                    }
                });
                return classes;
            }

            // A public class is owned by Microsoft.UI.Xaml.dll when the deployed manifest registers it to that DLL, or when it
            // has no registration and is listed in ApiSurfaceAllowList.KnownUnregisteredMuxClasses. Classes registered to other
            // DLLs, and unregistered classes that are not listed, belong to other DLLs (Controls, Phone, ...).
            private static bool IsOwnedByMux(string className, out string ownership)
            {
                if (s_classRegistrations.Value.TryGetValue(className, out string server))
                {
                    ownership = $"registered to {server} in {ClassRegistrationManifestFileName}";
                    return string.Equals(server, InspectableAbiHelper.MuxModuleName, StringComparison.OrdinalIgnoreCase);
                }
                if (ApiSurfaceAllowList.KnownUnregisteredMuxClasses.Contains(className))
                {
                    ownership = $"not registered in {ClassRegistrationManifestFileName} and listed in KnownUnregisteredMuxClasses";
                    return true;
                }
                ownership = $"not registered in {ClassRegistrationManifestFileName} and not listed in KnownUnregisteredMuxClasses";
                return false;
            }

            // Reads every <InProcessServer> of the class registration manifest that the test payload deploys next to the
            // test binary (test\CreateTestPayload.ps1). It is generated by the build, independently of the runtime under test.
            private static Dictionary<string, string> LoadClassRegistrations()
            {
                string directory = Path.GetDirectoryName(typeof(GeneratedApiSurfaceSweepTests).Assembly.Location);
                string path = Path.Combine(directory, ClassRegistrationManifestFileName);
                if (!File.Exists(path))
                {
                    throw new FileNotFoundException("Class registration manifest (deployed by test\\CreateTestPayload.ps1) was not found.", path);
                }

                var registrations = new Dictionary<string, string>(StringComparer.Ordinal);
                global::System.Xml.Linq.XDocument manifest = global::System.Xml.Linq.XDocument.Load(path);
                foreach (global::System.Xml.Linq.XElement server in manifest.Descendants().Where(e => e.Name.LocalName == "InProcessServer"))
                {
                    string serverPath = server.Elements().FirstOrDefault(e => e.Name.LocalName == "Path")?.Value;
                    if (string.IsNullOrEmpty(serverPath))
                    {
                        continue;
                    }
                    foreach (global::System.Xml.Linq.XElement activatableClass in server.Elements().Where(e => e.Name.LocalName == "ActivatableClass"))
                    {
                        string classId = (string)activatableClass.Attribute("ActivatableClassId");
                        if (!string.IsNullOrEmpty(classId))
                        {
                            registrations[classId] = Path.GetFileName(serverPath);
                        }
                    }
                }
                int muxRegistrations = registrations.Values.Count(v => string.Equals(v, InspectableAbiHelper.MuxModuleName, StringComparison.OrdinalIgnoreCase));
                Log.Comment($"[Catalog] class registrations {path}: {registrations.Count} classes, {muxRegistrations} registered to {InspectableAbiHelper.MuxModuleName}.");
                if (muxRegistrations == 0)
                {
                    throw new InvalidOperationException($"{path} registers no classes to {InspectableAbiHelper.MuxModuleName}.");
                }
                return registrations;
            }
        }

        #endregion
    }

    // Composes XamlRenderingBackgroundTask, whose constructor is protected.
    internal sealed partial class SweepRenderingBackgroundTask : XamlRenderingBackgroundTask
    {
    }

    // Exposes the protected VisualStateManager raise methods that drive VisualStateGroup's state events.
    internal sealed partial class RaisingVisualStateManager : VisualStateManager
    {
        public void RaiseChanging(VisualStateGroup group, VisualState oldState, VisualState newState, Control control)
        {
            RaiseCurrentStateChanging(group, oldState, newState, control);
        }

        public void RaiseChanged(VisualStateGroup group, VisualState oldState, VisualState newState, Control control)
        {
            RaiseCurrentStateChanged(group, oldState, newState, control);
        }
    }
}
