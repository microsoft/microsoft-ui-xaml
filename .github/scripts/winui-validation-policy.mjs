// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

export const gateContext = 'WinUI Validation Gate';
export const workflowPath = '.github/workflows/winui-validation.yml';
export const artifactName = 'winui-validation-state';

export function parseCommand(body, pipelineName) {
  const command = /^\/winui (quick|full)(?: ([a-f0-9]{7,40}))?$/i.exec(body.trim());
  if (command) return { mode: command[1].toLowerCase() === 'quick' ? 'Quick' : 'Full',
    source: 'ci', pin: command[2]?.toLowerCase() };
  if (['/azp run', '/AzurePipelines run', `/azp run ${pipelineName}`,
    `/AzurePipelines run ${pipelineName}`].includes(body.trim())) {
    return { mode: 'Full', source: 'native' };
  }
  return null;
}

export function revision(pr) {
  return { number: pr.number, head: pr.head.sha, base: pr.base.sha,
    merge: pr.merge_commit_sha, target: pr.base.ref };
}

export function sameRevision(a, b) {
  return a && b && ['number', 'head', 'base', 'merge', 'target'].every(key => a[key] === b[key]);
}

export function eligible(pr, targets) {
  return pr.state === 'open' && targets.includes(pr.base.ref)
    && [pr.head.sha, pr.base.sha, pr.merge_commit_sha].every(sha => /^[a-f0-9]{40}$/.test(sha ?? ''));
}

export function selection(pr, commandId = 0, command = { mode: 'Full', source: 'native' }) {
  return {
    revision: revision(pr), mode: command.mode, source: command.source, commandId,
    notBefore: pr.created_at, selectedAt: new Date().toISOString(), buildId: null,
    branch: command.source === 'ci'
      ? `refs/heads/validation/${command.mode.toLowerCase()}-pr/${pr.number}/c${commandId}`
      : `refs/pull/${pr.number}/merge`,
  };
}

export function matchesBuild(build, request, config) {
  const parameters = build.templateParameters ?? {};
  const quick = parameters.validationMode === 'Quick'
    || build.sourceBranch?.startsWith('refs/heads/validation/quick-pr/');
  return build.definition?.id === config.pipelineId
    && build.repository?.type === 'GitHub'
    && build.repository?.name?.toLowerCase() === config.repository.toLowerCase()
    && build.sourceVersion === request.revision.merge && build.sourceBranch === request.branch
    && (quick ? 'Quick' : 'Full') === request.mode
    && String(parameters.runFullValidation ?? true).toLowerCase() === 'true'
    && String(parameters.MUXFinalRelease ?? false).toLowerCase() === 'false'
    && Date.parse(build.queueTime) >= Date.parse(request.notBefore)
    && (request.source === 'native' ? build.reason === 'pullRequest'
      : ['individualCI', 'batchedCI'].includes(build.reason));
}

export function coveragePassed(records, mode) {
  const latest = new Map();
  for (const record of records) {
    if (!['Stage', 'Job'].includes(record.type)) continue;
    const key = `${record.type}:${record.identifier}`;
    if (!latest.has(key) || (record.attempt ?? 1) > (latest.get(key).attempt ?? 1)) latest.set(key, record);
  }
  const current = [...latest.values()];
  const success = record => record.state === 'completed' && record.result === 'succeeded';
  const stages = ['ValidateGitHubPrContext', 'Build', 'RunTests'];
  const jobs = { 'Build.BuildMUX': mode === 'Quick' ? 1 : 5,
    'RunTests.CreateTestPayload': mode === 'Quick' ? 1 : 3, 'RunTests.RunTests25H2': 20 };
  if (mode === 'Full') {
    stages.push('Build_MUXFinalRelease', 'RunStaticTests', 'RunScenarioTests');
    Object.assign(jobs, { 'Build_MUXFinalRelease.BuildMUX': 1, 'RunTests.RunTests23H2': 20,
      'RunTests.RunTestsRS5': 20, 'RunScenarioTests.RunAppTestsRS5': 5,
      'RunScenarioTests.RunAppTests25H2': 5, 'RunScenarioTests.RunAppTests23H2': 5 });
  }
  return stages.every(stage => current.some(r => r.type === 'Stage' && r.identifier === stage && success(r)))
    && Object.entries(jobs).every(([prefix, count]) => {
      const found = current.filter(r => r.type === 'Job'
        && (r.identifier === prefix || r.identifier?.startsWith(`${prefix}.`)));
      return found.length === count && found.every(success);
    });
}
