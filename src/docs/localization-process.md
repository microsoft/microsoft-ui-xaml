# Localization Process

## Table of Contents

- [Overview](#overview)
- [Supported Languages](#supported-languages)
- [Directions](#directions)
- [More Info](#more-info)
- [Responsibility & Requirements](#responsibility--requirements)

## Overview

Types of files being localized:
+ .mui files (built from checked-in .rc files)
+ .resw files

Nuget package where localized resources are being stored:
+ WinUILocalizationResources.Major.Minor.Patch-Tag.YYMMDD.R.nupkg
    See $(VersionFinal) in build\AzurePipelinesTemplates\WinUI-BuildVariables.yml for how the format is implemented

## Supported Languages

We currently support 85 languages indicated below which matches Windows OS.

af-ZA,am-ET,ar-SA,as-IN,az-Latn-AZ,bg-BG,bn-IN,bs-Latn-BA,ca-ES,ca-Es-VALENCIA,cs-CZ,cy-GB,da-DK,
de-DE,el-GR,en-GB,es-ES,es-MX,et-EE,eu-ES,fa-IR,fi-FI,fil-PH,fr-CA,fr-FR,ga-IE,gd-gb,gl-ES,gu-IN,
he-IL,hi-IN,hr-HR,hu-HU,hy-AM,id-ID,is-IS,it-IT,ja-JP,ka-GE,kk-KZ,km-KH,kn-IN,kok-IN,ko-KR,lb-LU,
lo-LA,lt-LT,lv-LV,mi-NZ,mk-MK,ml-IN,mr-IN,ms-MY,mt-MT,nb-NO,ne-NP,nl-NL,nn-NO,or-IN,pa-IN,pl-PL,
pt-BR,pt-PT,quz-PE,ro-RO,ru-RU,sk-SK,sl-SI,sq-AL,sr-Cyrl-BA,sr-Cyrl-RS,sr-Latn-RS,sv-SE,ta-IN,
te-IN,th-TH,tr-TR,tt-RU,ug-CN,uk-UA,ur-PK,uz-Latn-UZ,vi-VN,zh-CN,zh-TW

An up to date version of the language list and other details can be found in the localization pipeline configuration.

## Directions

If you want to manually schedule a pipeline run, use the following steps:

1. Check-in updated strings
2. Queue up the Localization Pipeline
    1. Click on RunPipeline
    2. Select your branch with newly added resource files
    3. If your updated strings are **not** in the .rc files, then under advanced options, Stages to run, just select localization. 
       No need to run build here.
    + Touchdown task takes .mui & .resw files and uploads to Touchdown service
    + Touchdown task retrieves currently localized files being stored in Touchdown
3. Currently available localized resource files get placed into a nuget package and uploaded to the internal dependency feed.
    1. The place holder files without localized data get immediately created with successful run. However, it takes couple of days 
       for Touchdown to localize new strings. Once files are generated, the next localization pipeline run will pick up the changes.
4. The pipeline will automatically detect when there are changes to localization files and create a Pull Request titled 'Localization Update YYYYMMDD'. 
   This is done by the script `build\PipelineScripts\ApplyLocalizationUpdates.ps1`.

## More Info

* During the build, localized .mui files get copied over from the nuget package version specified. The version number 
is automatically updated in `packages.config` for the `WinUILocalizationResources` 
package.
* Updates to .resw files are checked-in as part of the PR the pipeline opens

## Responsibility & Requirements

1. When adding a new control, make sure the english source file is under '../Strings/en-us'. 
   This is CASE SENSITIVE because currently the Touchdown localization service is unable to ignore character case. 
   It will treat the same source file as two different files if the character case in the file path is changed. 
   To keep consistent with the other english source files, we should keep the 'en-us' folder name lower case.
2. For new strings, give a time window of more than 3-4 days so that Touchdown has time for their 
   localization process. 
3. The pipeline should be run (Run all stages) at least once after we make changes to the Major and/or Minor version 
   numbers of Microsoft.UI.Xaml.dll. This is because the MUI system uses this info as part of a checksum to verify its 
   loading the correct resource files. The Touchdown service re-generates the .mui files before returning them to us 
   to make sure that the version is in sync.