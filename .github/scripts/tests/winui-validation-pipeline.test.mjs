// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { test } from 'node:test';
import YAML from 'yaml';

const root = new URL('../../../', import.meta.url);
const read = path => YAML.parse(readFileSync(new URL(path, root), 'utf8'));
const pipeline = read('build/WinUI-GitHub-PR.yml');
const defaults = Object.fromEntries(pipeline.parameters.map(p => [p.name, p.default]));

function evaluate(text, parameters, variables) {
  text = text.trim();
  if (text.startsWith('parameters.')) return parameters[text.slice(11)];
  if (text.startsWith('variables.')) return variables[text.slice(10)];
  const variable = /^variables\['(.+)'\]$/.exec(text);
  if (variable) return variables[variable[1]];
  if (text === 'true') return true;
  if (text === 'false') return false;
  if (/^-?\d+$/.test(text)) return Number(text);
  if (/^'.*'$/.test(text)) return text.slice(1, -1);
  const call = /^(eq|ne|in|and|or|startsWith)\((.*)\)$/.exec(text);
  assert.ok(call, `Unsupported condition ${text}`);
  const args = [];
  let depth = 0;
  let start = 0;
  for (let i = 0; i <= call[2].length; i++) {
    const c = call[2][i];
    if (c === '(') depth++;
    if (c === ')') depth--;
    if ((c === ',' && depth === 0) || i === call[2].length) {
      args.push(evaluate(call[2].slice(start, i), parameters, variables));
      start = i + 1;
    }
  }
  const same = (a, b) => String(a).toLowerCase() === String(b).toLowerCase();
  return { eq: () => same(...args), ne: () => !same(...args),
    and: () => args.every(Boolean), or: () => args.some(Boolean),
    in: () => args.slice(1).some(a => same(args[0], a)),
    startsWith: () => args[0].startsWith(args[1]) }[call[1]]();
}

function expand(value, parameters, variables = {}) {
  if (Array.isArray(value)) return value.flatMap(item => {
    const result = expand(item, parameters, variables);
    return result && Object.keys(result).length ? result : [];
  });
  if (typeof value !== 'object' || value === null) {
    const expression = typeof value === 'string' && /^\$\{\{ (.+) \}\}$/.exec(value);
    return expression ? evaluate(expression[1], parameters, variables) : value;
  }
  let result = {};
  let previous = false;
  for (const [key, item] of Object.entries(value)) {
    const condition = /^\$\{\{ if (.+) \}\}$/.exec(key);
    if (condition || key === '${{ else }}') {
      const include = condition ? evaluate(condition[1], parameters, variables) : !previous;
      previous = include;
      if (include) {
        const expanded = expand(item, parameters, variables);
        result = Array.isArray(expanded) ? expanded : { ...result, ...expanded };
      }
    } else result[key] = expand(item, parameters, variables);
  }
  return result;
}

function stages(branch, override = {}) {
  const parameters = { ...defaults, ...override };
  const variables = { 'Build.SourceBranch': branch };
  const mode = pipeline.variables.find(v => v.name === 'isQuickValidation');
  variables.isQuickValidation = String(expand(mode.value, parameters, variables));
  return expand(pipeline.extends.parameters.stages, parameters, variables);
}

test('ordinary PR and Full CI refs preserve full coverage; Quick refs select reduced runtime coverage', () => {
  for (const branch of ['refs/pull/42/merge', 'refs/heads/main', 'refs/heads/validation/full-pr/42/c100']) {
    const full = stages(branch);
    assert.equal(full.length, 7);
    assert.equal(full[1].parameters.pipelineKind, 'GitHubPR');
    assert.equal(full[1].parameters.buildScenarioApps, true);
    assert.equal(full[2].parameters.stageName, 'Build_MUXFinalRelease');
    assert.ok(full.some(stage => stage.template?.endsWith('WinUI-RunSystemCompositionEngineTests-Stage.yml')));
  }
  for (const quick of [stages('refs/heads/validation/quick-pr/42/c100'),
    stages('refs/pull/42/merge', { validationMode: 'Quick' })]) {
    assert.equal(quick.length, 3);
    assert.equal(quick[1].parameters.pipelineKind, 'QuickPR');
    assert.equal(quick[1].parameters.buildScenarioApps, false);
    assert.equal(quick[1].parameters.buildProductOnly, false);
    assert.equal(quick[2].parameters.pipelineKind, 'QuickPR');
    assert.ok(!quick.some(stage => stage.template?.endsWith('WinUI-RunSystemCompositionEngineTests-Stage.yml')));
  }
  assert.ok(!stages('refs/pull/42/merge', { MUXFinalRelease: true })
    .some(stage => stage.template?.endsWith('WinUI-RunSystemCompositionEngineTests-Stage.yml')));
  assert.equal(stages('refs/heads/validation/quick-pr/42/c100', { runFullValidation: false }).length, 1);
});

test('Quick still builds x86 Debug and runs all twenty 25H2 developer-test shards', () => {
  const build = read('build/AzurePipelinesTemplates/WinUI-BuildWinUI-Stage.yml');
  const quick = expand(build.stages[0].jobs[0].strategy.matrix, { pipelineKind: 'QuickPR' });
  assert.deepEqual(Object.keys(quick), ['Debug_x86']);
  const tests = read('build/AzurePipelinesTemplates/WinUI-RunTests-Stage.yml');
  const jobs = expand(tests.stages[0].jobs, { pipelineKind: 'QuickPR' });
  assert.deepEqual(jobs.map(job => job.parameters.runTestJobName), ['CreateTestPayload', 'RunTests25H2']);
  const runtime = read('build/AzurePipelinesTemplates/WinUI-RunTestPassOnPipeline-Job.yml');
  assert.equal(expand(runtime.jobs[0].strategy, { ...runtime.parameters, testSuite: 'DevTestSuite' }).parallel, 20);
});

test('privileged workflow uses trusted code, serialized publication and checkpoint-before-release', () => {
  const workflow = read('.github/workflows/winui-validation.yml');
  assert.equal(workflow.on.pull_request, undefined);
  assert.equal(workflow.concurrency['cancel-in-progress'], false);
  assert.equal(workflow.concurrency.queue, 'max');
  const steps = workflow.jobs.reconcile.steps;
  const checkout = steps.find(s => s.uses?.startsWith('actions/checkout'));
  assert.equal(checkout.with.ref, '${{ github.event.repository.default_branch }}');
  assert.equal(checkout.with['persist-credentials'], false);
  const release = steps.findIndex(s => s.run?.endsWith('winui-validation.mjs release'));
  const checkpoint = steps.findIndex(s => s.id === 'checkpoint');
  assert.ok(release > checkpoint);
  assert.ok(steps[release].if.includes("steps.checkpoint.outcome == 'success'"));
  assert.ok(steps[release].if.includes("safe_release == 'true'"));
});

test('validation workflows pin every external action to an immutable commit', () => {
  const check = value => {
    if (!value || typeof value !== 'object') return;
    if (value.uses) assert.match(value.uses, /^[\w.-]+\/[\w./-]+@[a-f0-9]{40}$/);
    for (const child of Object.values(value)) check(child);
  };
  check(read('.github/workflows/winui-validation.yml'));
  check(read('.github/workflows/winui-validation-tests.yml'));
});

test('security hold prevents activation through repository variables alone', () => {
  const workflow = read('.github/workflows/winui-validation.yml');
  assert.match(workflow.jobs.reconcile.if.trim(), /^false\s*&&/);
});
