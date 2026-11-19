// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

import assert from 'node:assert/strict';
import { test } from 'node:test';
import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { Gate, GitHub, Azure, UncertainPublication, newState, restore, pushValidationRef } from '../winui-validation.mjs';
import { parseCommand, revision, selection, matchesBuild, coveragePassed } from '../winui-validation-policy.mjs';

const config = { repository: 'example/widgets', pipelineId: 42, pipelineName: 'Widget PR',
  targets: ['main'], approvers: ['maintainer'], defaultBranch: 'main', runId: 500 };
const sha = letter => letter.repeat(40);
const pr = number => ({ number, state: 'open', created_at: '2026-01-01T00:00:00Z',
  updated_at: '2026-01-01T00:00:00Z', head: { sha: sha('a'), ref: 'feature', repo: { full_name: config.repository } },
  base: { sha: sha('b'), ref: 'main' }, merge_commit_sha: sha('c') });

function records(mode) {
  const stages = ['ValidateGitHubPrContext', 'Build', 'RunTests'];
  const jobs = { 'Build.BuildMUX': mode === 'Quick' ? 1 : 5,
    'RunTests.CreateTestPayload': mode === 'Quick' ? 1 : 3, 'RunTests.RunTests25H2': 20 };
  if (mode === 'Full') {
    stages.push('Build_MUXFinalRelease', 'RunStaticTests', 'RunScenarioTests', 'RunSystemCompositionEngineTests');
    Object.assign(jobs, { 'Build_MUXFinalRelease.BuildMUX': 1, 'RunTests.RunTests23H2': 20,
      'RunTests.RunTestsRS5': 20, 'RunScenarioTests.RunAppTestsRS5': 5,
      'RunScenarioTests.RunAppTests25H2': 5, 'RunScenarioTests.RunAppTests23H2': 5,
      'RunSystemCompositionEngineTests.CreateSystemCompositionEngineTestPayload_x86chk': 1,
      'RunSystemCompositionEngineTests.RunSystemCompositionEngineTests_x86chk': 1,
      'RunSystemCompositionEngineTests.RunSystemCompositionEngineBroadWPF_x86chk': 20 });
  }
  const row = (type, identifier) => ({ type, identifier, state: 'completed', result: 'succeeded', attempt: 1 });
  return [...stages.map(s => row('Stage', s)),
    ...Object.entries(jobs).flatMap(([job, count]) =>
      Array.from({ length: count }, (_, i) => row('Job', `${job}.${i}`)))];
}

function harness() {
  const h = { pulls: [pr(1)], comments: [], statuses: [], refs: new Map(), builds: [], events: [],
    permission: 'write', now: Date.now(), state: newState() };
  const github = {
    pulls: async () => structuredClone(h.pulls),
    pull: async number => structuredClone(h.pulls.find(p => p.number === number)),
    commit: async () => ({ parents: [{ sha: h.pulls[0].base.sha }, { sha: h.pulls[0].head.sha }] }),
    comment: async id => h.events.find(e => e.comment.id === id).comment,
    permission: async () => ({ permission: h.permission }),
    acknowledge: async (number, text) => h.comments.push(text),
    ensureRef: async (ref, commit) => {
      if (h.refs.has(ref) && h.refs.get(ref) !== commit) throw new Error('ref changed');
      h.refs.set(ref, commit);
    },
    publish: async (commit, state, message, url) => h.statuses.push({ commit, state, message, url }),
    api: async (path, body, method) => {
      if (path.startsWith('/rules/branches/')) {
        return h.noGate ? [] : [{ type: 'required_status_checks',
          parameters: { required_status_checks: [{ context: 'WinUI Validation Gate', integration_id: 123 }] } }];
      }
      if (method === 'DELETE') {
        h.refs.delete(`refs/${path.slice('/git/refs/'.length)}`);
        return;
      }
      assert.ok(path.startsWith('/git/matching-refs/'));
      const prefix = `refs/${path.slice('/git/matching-refs/'.length)}`;
      return [...h.refs].filter(([ref]) => ref.startsWith(prefix)).map(([ref, commit]) => ({ ref, object: { sha: commit } }));
    },
  };
  const azure = {
    builds: async () => h.builds,
    build: async id => structuredClone(h.builds.find(b => b.id === id)),
    timeline: async id => h.timeline ?? records(h.builds.find(b => b.id === id).sourceBranch.includes('/quick-pr/') ? 'Quick' : 'Full'),
    url: id => `https://example.test/build/${id}`,
  };
  github.list = path => github.api(path);
  h.github = github;
  h.azure = azure;
  h.gate = new Gate(config, github, azure, h.state, () => h.now);
  h.command = (body, login = 'maintainer') => {
    h.now += 1000;
    const event = { action: 'created', issue: { number: 1, pull_request: {} },
      comment: { id: h.events.length + 1, body, user: { login },
        created_at: new Date(h.now).toISOString(), updated_at: new Date(h.now).toISOString() } };
    event.comment.updated_at = event.comment.created_at;
    h.events.push(event);
    return event;
  };
  h.addBuild = (request = h.state.requests[1], id = h.builds.length + 100) => {
    const build = { id, definition: { id: config.pipelineId },
      repository: { type: 'GitHub', name: config.repository }, sourceBranch: request.branch,
      sourceVersion: request.revision.merge, reason: request.source === 'ci' ? 'individualCI' : 'pullRequest',
      queueTime: new Date(h.now + 1).toISOString(), templateParameters: {}, status: 'completed', result: 'succeeded' };
    h.builds.push(build);
    return build;
  };
  h.last = () => h.statuses.at(-1);
  return h;
}

test('default and /azp run select Full without creating extra CI refs', async () => {
  const h = harness();
  await h.gate.reconcile();
  h.addBuild();
  await h.gate.reconcile();
  assert.equal(h.last().state, 'success');
  assert.equal(h.refs.size, 0);
  await h.gate.reconcile(h.command('/winui quick'));
  h.addBuild();
  await h.gate.reconcile();
  assert.equal(h.last().state, 'success');
  await h.gate.reconcile(h.command('/azp run'));
  assert.equal(h.state.requests[1].mode, 'Full');
  assert.equal(h.state.requests[1].source, 'native');
  assert.equal(h.last().state, 'pending');
  assert.equal(h.refs.size, 1);
});

test('a late Full success or failure cannot replace the selected Quick result', async () => {
  const h = harness();
  await h.gate.reconcile();
  const old = h.addBuild();
  old.status = 'inProgress';
  await h.gate.reconcile(h.command('/winui quick aaaaaaa'));
  const quick = h.addBuild();
  quick.status = 'inProgress';
  old.status = 'completed';
  await h.gate.reconcile();
  assert.equal(h.last().state, 'pending');
  quick.status = 'completed';
  await h.gate.reconcile();
  assert.equal(h.last().state, 'success');
  old.result = 'failed';
  await h.gate.reconcile();
  assert.equal(h.last().state, 'success');
  assert.ok(h.last().url.endsWith(String(quick.id)));
});

test('explicit Full replaces Quick, and old Quick cannot unblock it', async () => {
  const h = harness();
  await h.gate.reconcile(h.command('/winui quick'));
  h.addBuild();
  await h.gate.reconcile();
  await h.gate.reconcile(h.command('/winui full'));
  assert.equal(h.last().state, 'pending');
  h.addBuild();
  await h.gate.reconcile();
  assert.equal(h.last().state, 'success');
  assert.match(h.last().message, /Full/);
});

test('a new head or base resets the selected validation to Full', async () => {
  const h = harness();
  await h.gate.reconcile(h.command('/winui quick'));
  h.addBuild();
  await h.gate.reconcile();
  h.pulls[0].head.sha = sha('d');
  h.pulls[0].merge_commit_sha = sha('e');
  await h.gate.reconcile();
  assert.equal(h.state.requests[1].mode, 'Full');
  assert.equal(h.last().state, 'pending');
  h.pulls[0].base.sha = sha('f');
  h.pulls[0].merge_commit_sha = sha('1');
  await h.gate.reconcile();
  assert.equal(h.last().commit, sha('1'));
});

test('authorization, pinned SHAs and edited comments fail closed', async () => {
  for (const variation of ['read', 'not-approved', 'wrong-pin', 'edited', 'fork', 'late']) {
    const h = harness();
    let event = h.command('/winui quick');
    if (variation === 'read') h.permission = 'read';
    if (variation === 'not-approved') event.comment.user.login = 'other-writer';
    if (variation === 'wrong-pin') event.comment.body = '/winui quick fffffff';
    if (variation === 'edited') event.comment.updated_at = '2027-01-01T00:00:00Z';
    if (variation === 'fork') h.pulls[0].head.repo.full_name = 'contributor/widgets';
    if (variation === 'late') h.pulls[0].updated_at = '2027-01-01T00:00:00Z';
    if (variation === 'edited') await h.gate.reconcile(event);
    else await assert.rejects(h.gate.reconcile(event));
    assert.equal(h.refs.size, 0, variation);
    assert.equal(h.state.requests[1].mode, 'Full', variation);
  }
});

test('duplicate commands and replayed old deliveries cannot start or select another build', async () => {
  const h = harness();
  const first = h.command('/winui quick');
  await h.gate.reconcile(first);
  await h.gate.reconcile(h.command('/winui quick'));
  assert.equal(h.refs.size, 1);
  await h.gate.reconcile(h.command('/winui full'));
  await h.gate.reconcile(first);
  assert.equal(h.state.requests[1].mode, 'Full');
  assert.equal(h.refs.size, 2);
});

test('Quick cannot start before the branch requires the controller-owned gate', async () => {
  const h = harness();
  h.noGate = true;
  await assert.rejects(h.gate.reconcile(h.command('/winui quick')), /target must require/);
  assert.equal(h.refs.size, 0);
  assert.equal(h.state.requests[1].mode, 'Full');
});

test('fresh but unrelated builds never satisfy a CI selection', async () => {
  const h = harness();
  await h.gate.reconcile(h.command('/winui quick'));
  const wrong = h.addBuild();
  wrong.sourceBranch += '-different-request';
  await h.gate.reconcile();
  assert.equal(h.last().state, 'pending');
  h.now += 16 * 60_000;
  await h.gate.reconcile();
  assert.equal(h.last().state, 'failure');
});

test('failed, canceled, context-only and incomplete-runtime runs cannot make the gate green', async () => {
  const h = harness();
  await h.gate.reconcile(h.command('/winui quick'));
  const build = h.addBuild();
  for (const result of ['failed', 'canceled', 'partiallySucceeded']) {
    build.result = result;
    await h.gate.reconcile();
    assert.equal(h.last().state, 'failure');
  }
  build.result = 'succeeded';
  h.timeline = records('Quick').filter(r => r.identifier !== 'RunTests.RunTests25H2.19');
  await h.gate.reconcile();
  assert.equal(h.last().state, 'failure');
  h.timeline = records('Quick');
  build.templateParameters.runFullValidation = false;
  await h.gate.reconcile();
  assert.equal(h.last().state, 'failure');
});

test('native Full requires the system-composition stage and every WPF shard', async () => {
  const h = harness();
  await h.gate.reconcile();
  h.addBuild();
  h.timeline = records('Full').filter(r => !r.identifier.startsWith('RunSystemCompositionEngineTests'));
  await h.gate.reconcile();
  assert.equal(h.last().state, 'failure');
  h.timeline = records('Full').filter(r =>
    r.identifier !== 'RunSystemCompositionEngineTests.RunSystemCompositionEngineBroadWPF_x86chk.19');
  await h.gate.reconcile();
  assert.equal(h.last().state, 'failure');
  h.timeline = records('Full');
  await h.gate.reconcile();
  assert.equal(h.last().state, 'success');
});

test('Full CI and fork builds preserve the upstream system-composition skip condition', async () => {
  for (const kind of ['ci', 'fork']) {
    const h = harness();
    if (kind === 'ci') {
      await h.gate.reconcile(h.command('/winui full'));
    } else {
      h.pulls[0].head.repo.full_name = 'contributor/widgets';
      await h.gate.reconcile();
    }
    h.addBuild();
    h.timeline = records('Full').filter(r => !r.identifier.startsWith('RunSystemCompositionEngineTests'));
    h.timeline.push({ type: 'Stage', identifier: 'RunSystemCompositionEngineTests',
      state: 'completed', result: 'skipped' });
    await h.gate.reconcile();
    assert.equal(h.last().state, 'success', kind);
  }
});

test('persisted state retains the exact selected run across workflow invocations', async () => {
  const h = harness();
  await h.gate.reconcile(h.command('/winui quick'));
  const first = h.addBuild();
  first.status = 'inProgress';
  await h.gate.reconcile();
  const saved = JSON.parse(JSON.stringify(h.state));
  const another = h.addBuild();
  another.result = 'succeeded';
  const gate = new Gate(config, h.github, h.azure, saved);
  await gate.reconcile();
  assert.equal(saved.requests[1].buildId, first.id);
  assert.equal(h.last().state, 'pending');
});

test('shared head commits block Quick even if another PR Full validation passes', async () => {
  const h = harness();
  await h.gate.reconcile(h.command('/winui quick'));
  h.addBuild();
  await h.gate.reconcile();
  h.pulls.push(pr(2));
  await h.gate.reconcile();
  h.addBuild(h.state.requests[2]);
  await h.gate.reconcile();
  assert.equal(h.last().state, 'failure');
  assert.match(h.last().message, /Shared/);
});

test('closing a PR cleans only reserved CI refs, not refs used by another open PR', async () => {
  const h = harness();
  await h.gate.reconcile(h.command('/winui quick'));
  const keep = h.state.requests[1].branch;
  h.pulls = [{ ...pr(2), head: { ...pr(2).head, ref: keep.slice('refs/heads/'.length) } }];
  h.refs.set('refs/heads/validation/full-pr/1/c5', sha('c'));
  await h.gate.reconcile();
  assert.equal(h.refs.has(keep), true);
  assert.equal(h.refs.has('refs/heads/validation/full-pr/1/c5'), false);
});

test('GitHub publication errors are distinguished from retryable read errors', async t => {
  t.mock.method(globalThis, 'fetch', async () => { throw new Error('timeout'); });
  await assert.rejects(new GitHub(config, 'fake-token').publish(sha('c'), 'success', 'test', 'https://example.test'),
    UncertainPublication);
});

test('native git publication fetches but never executes PR code or force-updates a ref', () => {
  const commands = [];
  const execute = (exe, args, options) => {
    commands.push(args);
    assert.equal(options.env.GIT_TERMINAL_PROMPT, '0');
    if (args[0] === 'remote') return `https://github.com/${config.repository}.git\n`;
    if (args[0] === 'rev-parse') return `${sha('c')}\n`;
    return '';
  };
  pushValidationRef(config.repository, 'fake-token', 'refs/heads/validation/quick-pr/1/c12', sha('c'), execute);
  assert.deepEqual(commands.at(-1), ['push', 'origin', `${sha('c')}:refs/heads/validation/quick-pr/1/c12`]);
  assert.equal(commands.some(args => args.includes('checkout') || args.includes('--force')), false);
  assert.throws(() => pushValidationRef(config.repository, '', 'refs/heads/main', sha('c'), execute));
});

test('only exact commands and successful required job attempts are accepted', () => {
  for (const text of ['> /winui quick', 'text /winui quick', '/winui quick\n/winui full', '/azp run Other']) {
    assert.equal(parseCommand(text, config.pipelineName), null);
  }
  assert.equal(coveragePassed(records('Quick'), 'Full'), false);
  assert.equal(coveragePassed(records('Quick'), 'Quick'), true);
  const timeline = records('Quick');
  timeline.push({ ...timeline.at(-1), attempt: 2, result: 'failed' });
  assert.equal(coveragePassed(timeline, 'Quick'), false);
  const request = selection(pr(1));
  assert.equal(matchesBuild({ definition: { id: 99 } }, request, config), false);
});

test('state restore ignores artifacts from untrusted workflow revisions', async t => {
  const directory = mkdtempSync(join(tmpdir(), 'winui-validation-restore-'));
  t.after(() => rmSync(directory, { recursive: true }));
  const github = {
    api: async path => {
      if (path.startsWith('/actions/artifacts')) {
        return { artifacts: [3, 2, 1].map(id => ({ id, expired: false, workflow_run: { id } })) };
      }
      if (path.includes('/workflows/')) return { id: 10 };
      if (path.includes('/runs/')) {
        const id = Number(path.split('/').at(-1));
        return { id, workflow_id: id === 3 ? 11 : 10, event: 'workflow_dispatch', head_sha: String(id) };
      }
      return { status: path.includes('/2...') ? 'diverged' : 'ahead' };
    },
  };
  const state = await restore(github, config, directory, (id, repo, dir) => {
    assert.equal(id, 1);
    writeFileSync(join(dir, 'state.json'), JSON.stringify({ ...newState(), commands: { 1: 25 } }));
  });
  assert.equal(state.commands[1], 25);
});

test('Azure metadata uses federated bearer authentication and never queues a build', async t => {
  const paths = [];
  t.mock.method(globalThis, 'fetch', async (url, options) => {
    paths.push(url);
    assert.equal(options.headers.authorization, 'Bearer fake-access-token');
    assert.equal(options.method, undefined);
    return Response.json(url.includes('/pipelines/') ? { templateParameters: {} } : { id: 100 });
  });
  await new Azure({ ...config, organization: 'example', project: 'Widgets' }, 'fake-access-token').build(100);
  assert.equal(paths.length, 2);
});
