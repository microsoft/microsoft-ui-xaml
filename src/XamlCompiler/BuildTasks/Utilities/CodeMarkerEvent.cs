// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.Tracing
{
    public enum CodeMarkerEvent {
      perfXC_StartPass1,
      perfXC_StartPass2,
      perfXC_EndPass1,
      perfXC_EndPass2,
      perfXC_FingerprintCheck,
      perfXC_RestoredTypeInfoBackup,
      perfXC_RestoredGeneratedPass2CodeFileBackup,
      perfXC_CreatingTypeUniverse,
      perfXC_CreatedTypeUniverse,
      perfXC_PageStart,
      perfXC_PageDone,
      perfXC_ReleasingTypeUniverse,
      perfXC_InitializeTypeNameMapStart,
      perfXC_InitializeTypeNameMapEnd,
      perfXC_GenerateTypeInfoStart,
      perfXC_GenerateTypeInfoEnd,
      perfXC_WriteTypeinfoFilesToDiskStart,
      perfXC_WriteTypeinfoFilesToDiskEnd,
      perfXC_PageTypeCollectStart,
      perfXC_PageTypeCollectEnd,
      perfXC_PageHarvestStart,
      perfXC_PageHarvestEnd,
      perfXC_WriteFilesToDiskStart,
      perfXC_WriteFilesToDiskEnd,
      perfXC_PageCodeGenStart,
      perfXC_PageCodeGenEnd,
      perfXC_PageEditStart,
      perfXC_PageEditEnd,
      perfXC_GenerateXBFStart,
      perfXC_GenerateXBFEnd,
      perfXC_GenerateSdkXBFStart,
      perfXC_GenerateSdkXBFEnd,
      perfXC_PageLoadStart,
      perfXC_PageLoadEnd,
      perfXC_PageValidateStart,
      perfXC_PageValidateEnd,
      perfXC_SearchIxmpAndBindableStart,
      perfXC_SearchIxmpAndBindableEnd,
      perfXC_CreatingSchemaContext,
      perfXC_CreatedSchemaContext
    };
}