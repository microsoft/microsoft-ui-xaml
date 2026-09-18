@echo off
rem =============================================================================
rem SetHostArch.cmd
rem
rem Detects the native host processor architecture and exposes it as _HostArch,
rem using the naming convention the build uses for toolchain folders:
rem     amd64 | arm64
rem
rem This selects which *native* build toolchain (MSBuild, VC compilers, VsDevCmd
rem host tools) to run. It is independent of the *target* architecture being
rem built (_BuildArch / Platform).
rem
rem We fall back to amd64 for any host that is neither arm64 nor amd64 (e.g. x86,
rem or a future architecture such as RISC-V) so the build keeps working via
rem emulation rather than failing outright.
rem
rem Detection order (most authoritative first):
rem   1. The machine-level PROCESSOR_ARCHITECTURE in the registry. This always
rem      reflects the true OS architecture even when init runs from an emulated
rem      shell (e.g. an x64 process on an arm64 OS), where the process-level
rem      environment variables would otherwise report the emulated architecture.
rem   2. PROCESSOR_ARCHITEW6432 - set only in an emulated process, where it holds
rem      the true host architecture.
rem   3. PROCESSOR_ARCHITECTURE - the process (possibly emulated) architecture.
rem =============================================================================

set _HostArch=amd64
set _detectedHostArch=

for /f "tokens=3" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PROCESSOR_ARCHITECTURE 2^>nul ^| findstr /i "REG_SZ"') do set _detectedHostArch=%%a

if not defined _detectedHostArch set _detectedHostArch=%PROCESSOR_ARCHITEW6432%
if not defined _detectedHostArch set _detectedHostArch=%PROCESSOR_ARCHITECTURE%

if /i "%_detectedHostArch%"=="ARM64" set _HostArch=arm64

set _detectedHostArch=
exit /b 0
