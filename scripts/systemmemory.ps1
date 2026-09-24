# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

param (
  [switch]$peakCommit,  # Print out the peak commit since system reboot
  [switch]$runForever   # Run forever, printing out the current total commit every 1s
)

Add-Type -TypeDefinition @"
 using System;
 using System.Diagnostics;
 using System.Runtime.InteropServices;

  public static class PsApiWrapper
  {
    [DllImport("psapi.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetPerformanceInfo([Out] out PsApiPerformanceInformation PerformanceInformation, [In] int Size);

    [StructLayout(LayoutKind.Sequential)]
    public struct PsApiPerformanceInformation
    {
      public int Size;
      public IntPtr CommitTotal;
      public IntPtr CommitLimit;
      public IntPtr CommitPeak;
      public IntPtr PhysicalTotal;
      public IntPtr PhysicalAvailable;
      public IntPtr SystemCache;
      public IntPtr KernelTotal;
      public IntPtr KernelPaged;
      public IntPtr KernelNonPaged;
      public IntPtr PageSize;
      public int HandlesCount;
      public int ProcessCount;
      public int ThreadCount;
    }

    public static PsApiPerformanceInformation GetPerformanceInfo()
    {
      PsApiPerformanceInformation perfInfo = new PsApiPerformanceInformation();
      if (GetPerformanceInfo(out perfInfo, Marshal.SizeOf(perfInfo)))
      {
        return perfInfo;
      }
      throw new Exception("GetPerformanceInfo failed.");
    }
  }
"@

if ($runForever.IsPresent) {
  $min = 10000000
  $max = 0

  if (Test-Path "$env:temp\systemmemorystop") {
    del "$env:temp\systemmemorystop"
  }

  while ($true)
  {
      $x = ([PsApiWrapper]::GetPerformanceInfo())
      $commitTotalMb = ($x.CommitTotal.ToInt64() * $x.PageSize.ToInt64() / (1024 * 1024))
      $commitTotalMb = [math]::floor($commitTotalMb)
      if ($commitTotalMb -gt $max) { $max = $commitTotalMb }
      if ($commitTotalMb -lt $min) { $min = $commitTotalMb }

      $t = (get-date -format o)
      Write-Host "$t Commit total (MB): $commitTotalMb min:$min max:$max"

      if (Test-Path "$env:temp\systemmemorystop") {
        break
      }
      sleep 1
  }
}
elseif ($peakCommit.IsPresent) {
  $x = ([PsApiWrapper]::GetPerformanceInfo())
  $commitPeakMb = ($x.CommitPeak.ToInt64() * $x.PageSize.ToInt64() / (1024 * 1024))
  Write-Host "Commit peak (MB): $commitPeakMb"
}
else {
  throw "Please specify a command."
}
