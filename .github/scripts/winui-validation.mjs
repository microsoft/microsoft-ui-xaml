// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { join } from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
import { createHash } from 'node:crypto';
import { artifactName, workflowPath, gateContext, parseCommand, revision, sameRevision,
  eligible, selection, matchesBuild, coveragePassed } from './winui-validation-policy.mjs';

export class UncertainPublication extends Error {}

const trustedEvents = ['issue_comment', 'schedule', 'workflow_dispatch'];

export function assertTrustedExecution(env, config) {
  const ref = `refs/heads/${config.defaultBranch}`;
  if (!trustedEvents.includes(env.GITHUB_EVENT_NAME) || env.GITHUB_REF !== ref
    || env.GITHUB_WORKFLOW_REF !== `${config.repository}/${workflowPath}@${ref}`
    || !/^[a-f0-9]{40}$/.test(config.automationSha ?? '') || env.GITHUB_SHA !== config.automationSha) {
    throw new Error('Validation must run the pinned default-branch workflow on a trusted event');
  }
}

export function pushValidationRef(repository, token, ref, sha, execute = execFileSync) {
  const match = /^refs\/heads\/validation\/(?:quick|full)-pr\/([1-9][0-9]*)\/c[0-9]+$/.exec(ref);
  if (!match || !/^[a-f0-9]{40}$/.test(sha)) throw new Error('Invalid validation ref');
  const env = { ...process.env, GIT_TERMINAL_PROMPT: '0', GIT_CONFIG_COUNT: '1',
    GIT_CONFIG_KEY_0: 'http.https://github.com/.extraheader',
    GIT_CONFIG_VALUE_0: `AUTHORIZATION: basic ${Buffer.from(`x-access-token:${token}`).toString('base64')}` };
  const git = args => execute('git', args, { env, encoding: 'utf8', stdio: 'pipe', timeout: 120_000 }).trim();
  if (git(['remote', 'get-url', 'origin']).replace(/\.git$/, '') !== `https://github.com/${repository}`) {
    throw new Error('Checkout origin is not the configured GitHub repository');
  }
  // Only fetch objects; never check out or execute the PR. Both parents already
  // exist upstream because this path is restricted to in-repository PRs.
  git(['fetch', '--no-tags', '--depth=2', 'origin', `refs/pull/${match[1]}/merge`]);
  if (git(['rev-parse', 'FETCH_HEAD']) !== sha) throw new Error('PR merge ref changed before publication');
  git(['push', 'origin', `${sha}:${ref}`]);
}

export class GitHub {
  constructor(config, token) {
    this.config = config;
    this.token = token;
    this.root = `https://api.github.com/repos/${config.repository}`;
  }
  async api(path, body, method = body === undefined ? 'GET' : 'POST') {
    const response = await fetch(`${this.root}${path}`, {
      method, redirect: 'error', signal: AbortSignal.timeout(30_000),
      headers: { authorization: `Bearer ${this.token}`, accept: 'application/vnd.github+json',
        'content-type': 'application/json', 'user-agent': 'winui-validation' },
      body: body === undefined ? undefined : JSON.stringify(body),
    });
    if (!response.ok) {
      const error = new Error(`GitHub API failed (${response.status})`);
      error.status = response.status;
      throw error;
    }
    return response.status === 204 ? null : response.json();
  }
  async list(path, key) {
    const all = [];
    for (let page = 1; page <= 100; page++) {
      const response = await this.api(`${path}${path.includes('?') ? '&' : '?'}per_page=100&page=${page}`);
      const batch = key ? response[key] : response;
      all.push(...batch);
      if (batch.length < 100) return all;
    }
    throw new Error('GitHub pagination limit reached; refusing incomplete results');
  }
  pulls() { return this.list('/pulls?state=open'); }
  pull(number) { return this.api(`/pulls/${number}`); }
  commit(sha) { return this.api(`/git/commits/${sha}`); }
  comment(id) { return this.api(`/issues/comments/${id}`); }
  permission(login) { return this.api(`/collaborators/${encodeURIComponent(login)}/permission`); }
  acknowledge(number, text) { return this.api(`/issues/${number}/comments`, { body: text }); }
  async ensureRef(ref, sha) {
    const path = `/git/ref/${ref.replace(/^refs\//, '')}`;
    try {
      const existing = await this.api(path);
      if (existing.object.sha !== sha) throw new Error('Validation ref changed unexpectedly');
      return;
    } catch (error) {
      if (error.status !== 404) throw error;
    }
    try {
      if (ref.startsWith('refs/heads/validation/')) {
        pushValidationRef(this.config.repository, this.token, ref, sha);
      } else {
        await this.api('/git/refs', { ref, sha });
      }
    } catch (error) {
      // Ref creation is idempotent. Never force-update a request's revision.
      const existing = await this.api(path);
      if (existing.object.sha !== sha) throw error;
    }
  }
}

export class Publisher extends GitHub {
  constructor(config, token) {
    super(config, token);
    if (!token || !Number.isSafeInteger(config.publisherAppId) || config.publisherAppId <= 0) {
      throw new Error('A dedicated publisher App ID and token are required');
    }
  }
  verify(check, sha) {
    if (check.app?.id !== this.config.publisherAppId || check.head_sha !== sha || check.name !== gateContext
      || !Number.isSafeInteger(check.id) || check.id <= 0) {
      throw new Error('Gate check does not belong to the configured publisher and revision');
    }
  }
  async assertCheckpoint(state, pulls) {
    const revisions = new Set(pulls.filter(pr => eligible(pr, this.config.targets)).map(pr => pr.merge_commit_sha));
    for (const sha of revisions) {
      const checks = await this.list(`/commits/${sha}/check-runs?check_name=${encodeURIComponent(gateContext)}&filter=latest&app_id=${this.config.publisherAppId}`, 'check_runs');
      const current = checks.filter(check => check.app?.id === this.config.publisherAppId)
        .sort((a, b) => b.id - a.id)[0];
      const saved = state.published[sha];
      if (!current && !saved) continue;
      if (!current || !saved) throw new Error('Gate checkpoint is missing; operator recovery required');
      this.verify(current, sha);
      if (current.id !== state.checks[sha] || current.external_id !== saved.request
        || current.details_url !== saved.url || current.output?.summary !== saved.message
        || current.status !== (saved.state === 'pending' ? 'in_progress' : 'completed')
        || (saved.state !== 'pending' && current.conclusion !== (saved.state === 'success' ? 'success' : 'failure'))) {
        throw new Error('Gate checkpoint is stale; refusing to replace the selected run');
      }
    }
  }
  async publish(sha, state, description, url, id, request) {
    const body = { name: gateContext, details_url: url, output: { title: gateContext, summary: description },
      external_id: request,
      status: state === 'pending' ? 'in_progress' : 'completed',
      ...(state === 'pending' ? {} : { conclusion: state === 'success' ? 'success' : 'failure' }) };
    if (id) {
      const current = await this.api(`/check-runs/${id}`);
      this.verify(current, sha);
      if (current.status === 'completed' && state === 'pending') id = undefined;
    }
    try {
      const check = id ? await this.api(`/check-runs/${id}`, body, 'PATCH')
        : await this.api('/check-runs', { ...body, head_sha: sha });
      this.verify(check, sha);
      return check.id;
    } catch (error) {
      // Keep the transaction ref after an uncertain write; the next workflow must
      // not overtake a request that GitHub could still be processing.
      throw new UncertainPublication(error.message);
    }
  }
}

export class Azure {
  constructor(config, token) {
    this.config = config;
    this.token = token;
    this.root = `https://dev.azure.com/${encodeURIComponent(config.organization)}/${encodeURIComponent(config.project)}`;
  }
  async api(path) {
    const response = await fetch(`${this.root}/_apis/${path}${path.includes('?') ? '&' : '?'}api-version=7.1`, {
      redirect: 'error', signal: AbortSignal.timeout(30_000),
      headers: { authorization: `Bearer ${this.token}` },
    });
    if (!response.ok) throw new Error(`Azure build metadata request failed (${response.status})`);
    return { data: await response.json(), next: response.headers.get('x-ms-continuationtoken') };
  }
  async builds() {
    const all = [];
    let continuation = '';
    for (let page = 0; page < 100; page++) {
      const { data, next } = await this.api(
        `build/builds?definitions=${this.config.pipelineId}&queryOrder=queueTimeDescending&$top=100${continuation}`);
      all.push(...data.value);
      if (!next || (data.value.length && Date.parse(data.value.at(-1).queueTime) < Date.now() - 2 * 86400_000)) return all;
      continuation = `&continuationToken=${encodeURIComponent(next)}`;
    }
    throw new Error('Azure build listing exceeded its safety bound');
  }
  async build(id) {
    const { data } = await this.api(`build/builds/${id}`);
    const run = await this.api(`pipelines/${this.config.pipelineId}/runs/${id}`);
    return { ...data, templateParameters: run.data.templateParameters ?? data.templateParameters ?? {} };
  }
  async timeline(id) { return (await this.api(`build/builds/${id}/timeline`)).data.records; }
  url(id) { return `${this.root}/_build/results?buildId=${id}`; }
}

export function newState() {
  return { version: 2, requests: {}, commands: {}, published: {}, checks: {} };
}

export class Gate {
  constructor(config, github, azure, state, publisher, now = () => Date.now()) {
    Object.assign(this, { config, github, azure, state, publisher, now });
  }
  url(number) { return `https://github.com/${this.config.repository}/pull/${number}`; }
  async publish(request, result) {
    const sha = request.revision.merge;
    const selections = Object.values(this.state.requests).filter(item => item.revision.merge === sha)
      .sort((a, b) => a.revision.number - b.revision.number);
    const value = { ...result, request: createHash('sha256').update(JSON.stringify(selections)).digest('hex'),
      url: request.buildId ? this.azure.url(request.buildId) : this.url(request.revision.number) };
    if (JSON.stringify(this.state.published[sha]) === JSON.stringify(value)) return;
    this.state.checks[sha] = await this.publisher.publish(sha, result.state, result.message, value.url,
      this.state.checks[sha], value.request);
    this.state.published[sha] = value;
  }
  async accept(event) {
    if (!event.issue?.pull_request || event.action !== 'created') return;
    const command = parseCommand(event.comment.body, this.config.pipelineName);
    if (!command) return;
    const number = event.issue.number;
    if (event.comment.id <= (this.state.commands[number] ?? 0)) return;
    const current = await this.github.comment(event.comment.id);
    if (current.body !== event.comment.body || current.created_at !== current.updated_at) return;
    const permission = await this.github.permission(current.user.login);
    if (!['admin', 'maintain', 'write'].includes(permission.permission)) throw new Error('Commenter needs repository write access');
    if (command.mode === 'Quick' && this.config.approvers.length
      && !this.config.approvers.includes(current.user.login.toLowerCase())) throw new Error('Commenter is not a Quick approver');
    const pr = await this.github.pull(number);
    if (!eligible(pr, this.config.targets) || (command.pin && !pr.head.sha.startsWith(command.pin))) {
      throw new Error('PR is closed, has no valid merge revision, or the pinned head changed');
    }
    if (command.source === 'ci' && pr.head.repo?.full_name?.toLowerCase() !== this.config.repository.toLowerCase()) {
      throw new Error('Custom validation refs are limited to in-repository PRs; use the separate reviewed fork-validation flow');
    }
    if (command.mode === 'Quick' && Date.parse(pr.updated_at) > Date.parse(current.created_at)) {
      throw new Error('PR changed after the Quick command was written; review and issue a fresh command');
    }
    if (command.mode === 'Quick' && (await this.github.pulls()).some(other =>
      other.number !== number && other.head.sha === pr.head.sha)) throw new Error('Shared head commits require Full validation');
    if (command.mode === 'Quick') {
      const rules = await this.github.list(`/rules/branches/${encodeURIComponent(pr.base.ref)}`);
      const guarded = rules.some(rule => rule.type === 'required_status_checks'
        && rule.parameters.required_status_checks.some(check =>
          check.context === gateContext && check.integration_id === this.config.publisherAppId));
      if (!guarded) throw new Error('The target must require WinUI Validation Gate from its expected App before enabling Quick');
    }
    const merge = await this.github.commit(pr.merge_commit_sha);
    if (merge.parents?.length !== 2 || merge.parents[0].sha !== pr.base.sha || merge.parents[1].sha !== pr.head.sha) {
      throw new Error('GitHub merge revision is not current');
    }
    const previous = this.state.requests[number];
    if (command.source === 'ci' && previous?.source === 'ci' && previous.mode === command.mode
      && sameRevision(previous.revision, revision(pr)) && previous.lastResult === 'pending') {
      this.state.commands[number] = event.comment.id;
      await this.github.acknowledge(number, `${command.mode} validation is already pending for this revision.`);
      return;
    }
    const request = selection(pr, event.comment.id, command);
    request.notBefore = current.created_at;
    request.requester = current.user.login;
    this.state.requests[number] = request;
    this.state.commands[number] = event.comment.id;
    await this.publish(request, { state: 'pending', message: `${request.mode} selected; waiting for validation` });
    if (request.source === 'ci') await this.github.ensureRef(request.branch, request.revision.merge);
    await this.github.acknowledge(number,
      `${request.mode} validation selected for head \`${pr.head.sha}\`, merge \`${pr.merge_commit_sha}\`. `
      + (command.mode === 'Quick' ? 'Coverage: x86 Debug developer tests on Windows 11 25H2. ' : '')
      + 'The WinUI Validation Gate check tracks only the selected attempt; all other merge requirements remain.');
  }
  async evaluate(request, builds) {
    if (request.source === 'ci' && !request.buildId) await this.github.ensureRef(request.branch, request.revision.merge);
    if (!request.buildId) {
      const candidates = builds.filter(b => b.sourceBranch === request.branch && b.sourceVersion === request.revision.merge);
      for (const candidate of candidates) {
        if (matchesBuild(await this.azure.build(candidate.id), request, this.config)) {
          request.buildId = candidate.id;
          break;
        }
      }
    }
    if (!request.buildId) {
      const expired = request.source === 'ci' && this.now() - Date.parse(request.selectedAt) > 15 * 60_000;
      return { state: expired ? 'failure' : 'pending',
        message: expired ? 'No CI build found for the selected ref; check the pipeline UI trigger'
          : `${request.mode}: waiting for the selected build` };
    }
    const build = await this.azure.build(request.buildId);
    if (!matchesBuild(build, request, this.config)) return { state: 'failure', message: 'Selected build identity or coverage mismatch' };
    if (build.status !== 'completed') return { state: 'pending', message: `${request.mode} validation is running` };
    if (build.result !== 'succeeded') {
      return { state: 'failure', message: `${request.mode} build and required runtime shards did not all pass` };
    }
    const current = await this.github.pull(request.revision.number);
    const merge = await this.github.commit(request.revision.merge);
    if (current.state !== 'open' || !sameRevision(request.revision, revision(current))
      || merge.parents?.length !== 2
      || merge.parents[0].sha !== request.revision.base || merge.parents[1].sha !== request.revision.head) {
      return { state: 'failure', message: 'PR revision changed; fresh Full validation required' };
    }
    // Match the upstream stage's PR-only, non-fork condition. Full CI validation
    // refs legitimately skip that stage; Quick never includes it.
    const isFork = current.head.repo && current.head.repo.full_name.toLowerCase() !== this.config.repository.toLowerCase();
    const systemCompositionRequired = build.reason === 'pullRequest' && !isFork;
    if (!coveragePassed(await this.azure.timeline(build.id), request.mode, { systemCompositionRequired })) {
      return { state: 'failure', message: `${request.mode} build and required runtime shards did not all pass` };
    }
    return { state: 'success', message: `${request.mode} validation passed for the selected revision` };
  }
  async reconcile(event) {
    let pulls = await this.github.pulls();
    for (const number of Object.keys(this.state.requests)) {
      if (pulls.some(pr => pr.number === Number(number))) continue;
      for (const mode of ['quick', 'full']) {
        const prefix = `heads/validation/${mode}-pr/${number}/`;
        for (const ref of await this.github.api(`/git/matching-refs/${prefix}`)) {
          if (!new RegExp(`^refs/${prefix}c[0-9]+$`).test(ref.ref)) throw new Error('Unexpected validation ref; refusing cleanup');
          if (pulls.some(pr => `refs/heads/${pr.head.ref}` === ref.ref)) continue;
          await this.github.api(`/git/refs/${ref.ref.replace(/^refs\//, '')}`, undefined, 'DELETE');
        }
      }
      delete this.state.requests[number];
    }
    for (const pr of pulls.filter(p => eligible(p, this.config.targets))) {
      if (!sameRevision(this.state.requests[pr.number]?.revision, revision(pr))) {
        const request = selection(pr);
        this.state.requests[pr.number] = request;
        await this.publish(request, { state: 'pending', message: 'Full validation required for this revision' });
      }
    }
    if (event?.issue?.pull_request) await this.accept(event);
    pulls = await this.github.pulls();
    const builds = await this.azure.builds();
    const groups = new Map();
    for (const pr of pulls.filter(p => eligible(p, this.config.targets))) {
      const request = this.state.requests[pr.number];
      if (!sameRevision(request?.revision, revision(pr))) continue;
      let result;
      if (request.mode === 'Quick' && pulls.some(p => p.number !== pr.number && p.head.sha === pr.head.sha)) {
        result = { state: 'failure', message: 'Shared head commits require Full validation' };
      } else {
        try { result = await this.evaluate(request, builds); }
        catch (error) {
          if (error instanceof UncertainPublication) throw error;
          result = { state: 'error', message: 'Unable to verify the selected run; retrying on the next reconciliation' };
        }
      }
      request.lastResult = result.state;
      const group = groups.get(request.revision.merge) ?? [];
      group.push({ request, result });
      groups.set(request.revision.merge, group);
    }
    for (const group of groups.values()) {
      const { request, result } = group.find(item => item.result.state !== 'success') ?? group[0];
      await this.publish(request, result);
    }
  }
}

export function checkpoint(state, config) {
  return { automationSha: config.automationSha, runId: config.runId, attempt: config.attempt, state };
}

export async function restore(github, config, directory, download = (run, repo, destination) =>
  execFileSync('gh', ['run', 'download', String(run), '--repo', repo,
    '--name', artifactName, '--dir', destination], { stdio: 'pipe' })) {
  const workflow = await github.api(`/actions/workflows/${workflowPath.split('/').at(-1)}`);
  for (let page = 1; page <= 100; page++) {
    const { artifacts } = await github.api(`/actions/artifacts?name=${artifactName}&per_page=100&page=${page}`);
    for (const artifact of artifacts.sort((a, b) => b.id - a.id)) {
      const run = await github.api(`/actions/runs/${artifact.workflow_run.id}`);
      if (run.workflow_id !== workflow.id || !trustedEvents.includes(run.event)
        || run.path !== workflowPath || run.head_branch !== config.defaultBranch) continue;
      // These events run and check out the same immutable default-branch revision.
      // A PR/status event's head_sha is not evidence of which automation executed.
      if (artifact.expired) throw new Error('Latest trusted checkpoint expired; operator recovery required');
      if (!/^[a-f0-9]{40}$/.test(run.head_sha ?? '')) throw new Error('Invalid workflow revision');
      const ancestry = await github.api(`/compare/${run.head_sha}...${encodeURIComponent(config.defaultBranch)}`);
      if (!['ahead', 'identical'].includes(ancestry.status)) {
        throw new Error('Trusted automation history changed; refusing to restore older state');
      }
      download(run.id, config.repository, directory);
      const saved = JSON.parse(readFileSync(join(directory, 'state.json'), 'utf8'));
      const { state } = saved;
      if (saved.automationSha !== run.head_sha || saved.runId !== run.id
        || !Number.isSafeInteger(run.run_attempt) || !Number.isSafeInteger(saved.attempt)
        || saved.attempt < 1 || saved.attempt > run.run_attempt
        || state?.version !== 2 || !state.requests || !state.commands || !state.published || !state.checks) {
        throw new Error('Invalid saved gate state or checkpoint provenance; refusing older state');
      }
      return state;
    }
    if (artifacts.length < 100) return newState();
  }
  throw new Error('No trustworthy state found within the artifact search bound');
}

function configuration(env) {
  const required = name => {
    if (!env[name]) throw new Error(`Missing ${name}`);
    return env[name];
  };
  const config = {
    repository: required('GITHUB_REPOSITORY'), organization: required('WINUI_ADO_ORGANIZATION'),
    project: required('WINUI_ADO_PROJECT'), pipelineId: Number(required('WINUI_ADO_PIPELINE_ID')),
    pipelineName: required('WINUI_ADO_PIPELINE_NAME'), defaultBranch: required('WINUI_DEFAULT_BRANCH'),
    publisherAppId: Number(required('WINUI_PUBLISHER_APP_ID')), automationSha: required('GITHUB_WORKFLOW_SHA'),
    targets: required('WINUI_VALIDATION_TARGETS').split(',').map(s => s.trim()),
    approvers: (env.WINUI_QUICK_APPROVERS ?? '').split(',').map(s => s.trim().toLowerCase()).filter(Boolean),
    runId: Number(required('GITHUB_RUN_ID')), attempt: Number(required('GITHUB_RUN_ATTEMPT')),
  };
  if (!Number.isSafeInteger(config.pipelineId) || config.pipelineId <= 0) throw new Error('Invalid pipeline ID');
  assertTrustedExecution(env, config);
  return config;
}

async function main() {
  const config = configuration(process.env);
  const github = new GitHub(config, process.env.GH_TOKEN);
  const directory = join(process.env.RUNNER_TEMP, 'winui-validation');
  const lock = `refs/heads/winui-gate/transactions/${config.runId}-${config.attempt}`;
  if (process.argv[2] === 'release') {
    await github.api(`/git/refs/${lock.replace(/^refs\//, '')}`, undefined, 'DELETE');
    return;
  }
  const locks = await github.api('/git/matching-refs/heads/winui-gate/transactions/');
  if (locks.length) throw new Error('An interrupted gate transaction needs operator recovery. See WINUI-VALIDATION.md.');
  mkdirSync(directory, { recursive: true });
  const state = await restore(github, config, directory);
  const publisher = new Publisher(config, process.env.WINUI_PUBLISHER_TOKEN);
  // Artifact deletion must not roll back a selection already published by the App.
  await publisher.assertCheckpoint(state, await github.pulls());
  const before = JSON.stringify(state);
  const base = await github.api(`/git/ref/heads/${encodeURIComponent(config.defaultBranch)}`);
  await github.ensureRef(lock, base.object.sha);
  let safe = true;
  try {
    const event = JSON.parse(readFileSync(process.env.GITHUB_EVENT_PATH, 'utf8'));
    await new Gate(config, github, new Azure(config, process.env.AZURE_DEVOPS_TOKEN), state, publisher).reconcile(event);
  } catch (error) {
    safe = !(error instanceof UncertainPublication);
    throw error;
  } finally {
    writeFileSync(join(directory, 'state.json'), JSON.stringify(checkpoint(state, config)));
    if (existsSync(join(directory, 'state.json'))) {
      const unchanged = before === JSON.stringify(state);
      writeFileSync(process.env.GITHUB_OUTPUT,
        `state_available=${!unchanged}\nunchanged=${unchanged}\nsafe_release=${safe}\n`, { flag: 'a' });
    }
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) await main();
