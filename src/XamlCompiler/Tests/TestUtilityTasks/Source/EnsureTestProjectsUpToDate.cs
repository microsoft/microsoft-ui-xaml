// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
// Copyright(c) 2014 Microsoft Corporation
//--------------------------------------------------------------------------------------------

namespace XamlCompilerTestsUtilityTasks
{
	using System;
	using Microsoft.Build.Utilities;
	using XamlCompilerTestsUtilityTasks.TestProjectsUpdators;
	using System.Diagnostics;

	public class EnsureTestProjectsUpToDate : Task
	{
		public override bool Execute()
		{
			var enlistmentDrive = Environment.GetEnvironmentVariable("_NTDRIVE");
			var enlistmentRoot = Environment.GetEnvironmentVariable("_NTROOT");

			if (string.IsNullOrEmpty(enlistmentDrive) || string.IsNullOrEmpty(enlistmentRoot))
			{
				throw new InvalidOperationException("Please run the tool within Razzle Environment");
			}

			var enlistmentRootPath = enlistmentDrive + enlistmentRoot;

			var manifestUpdater = new ManifestFileUpdater(enlistmentRootPath);
			manifestUpdater.EnsureManifestFilesUpToDate();

			var projectFilesUpdator = new ProjectFileUpdater(enlistmentRootPath);
			projectFilesUpdator.EnsureProjectFileUpToDate();
			return true;
		}
	}
}
