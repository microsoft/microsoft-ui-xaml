// Requests optional reviewers on a PR: every area in .github/area-owners.yml whose
// paths match a changed file contributes its reviewers. Nothing here blocks a merge.
//
// Runs on opened / reopened / ready_for_review and on every push (synchronize). A
// person is only ever requested once per PR: anyone who is the author, is currently
// requested, has already reviewed, or was requested before (including people the
// author later removed) is skipped. So pushes only add owners of newly touched areas.

import fs from 'node:fs';
import {
  MAP_PATH,
  UNASSIGNED,
  annotate,
  areasForFile,
  code,
  compileAreas,
  createLog,
  github,
  parseMap,
  readMapText,
  requestReviewers,
  schemaErrors,
  uniqueHandles,
} from './area-owners-lib.mjs';

const repo = process.env.GITHUB_REPOSITORY;
const dryRun = (process.env.AREA_REVIEWERS_DRY_RUN ?? 'true').toLowerCase() !== 'false';
const event = JSON.parse(fs.readFileSync(process.env.GITHUB_EVENT_PATH, 'utf8'));
const number = event.pull_request.number;
const gh = github(process.env.GITHUB_TOKEN);
const { log, flush } = createLog();

// The pulls/files endpoint stops at this many files.
const FILES_API_LIMIT = 3000;

async function changedPaths() {
  const entries = await gh.paginate(`/repos/${repo}/pulls/${number}/files`);
  if (entries.length >= FILES_API_LIMIT) {
    log(`> Note: GitHub only lists the first ${FILES_API_LIMIT} changed files; areas touched beyond that are missed.`);
    log('');
  }
  // A move notifies the owners of both the old and the new location.
  return [...new Set(entries.flatMap((f) => [f.filename, f.previous_filename].filter(Boolean)))];
}

// Everyone who is, or has been, involved in reviewing this PR.
async function alreadyInvolved(pr) {
  const [reviews, timeline] = await Promise.all([
    gh.paginate(`/repos/${repo}/pulls/${number}/reviews`),
    gh.paginate(`/repos/${repo}/issues/${number}/timeline`),
  ]);
  const logins = [
    pr.user,
    ...(pr.requested_reviewers ?? []),
    ...reviews.map((r) => r.user),
    ...timeline.filter((e) => e.event === 'review_requested').map((e) => e.requested_reviewer),
  ]
    .filter(Boolean)
    .map((u) => u.login.toLowerCase());
  return new Set(logins);
}

async function main() {
  const data = parseMap(readMapText()).toJS();
  const errors = schemaErrors(data);
  if (errors.length > 0) {
    throw new Error(`${MAP_PATH} is invalid: ${errors.join('; ')}`);
  }
  const areas = compileAreas(data.areas);

  const { data: pr } = await gh.request(`/repos/${repo}/pulls/${number}`);
  log(`### Area reviewers${dryRun ? ' (dry run)' : ''}`);
  log('');
  if (pr.state !== 'open' || pr.draft) {
    log(`PR #${number} is ${pr.draft ? 'a draft' : pr.state}; nothing to do.`);
    return;
  }

  const files = await changedPaths();
  const matched = new Map();
  const unmatched = [];
  for (const file of files) {
    const owners = areasForFile(areas, file);
    if (owners.length === 0) unmatched.push(file);
    for (const area of owners) matched.set(area.id, area);
  }
  const owned = [...matched.values()].filter((a) => a.id !== UNASSIGNED);

  log(`PR #${number} (${event.action}) - ${files.length} changed path(s), ${owned.length} area(s) matched.`);
  for (const area of owned) {
    log(`- \`${area.id}\` ${area.name}: ${area.reviewers.map((r) => `@${r}`).join(', ')}`);
  }
  if (matched.has(UNASSIGNED)) {
    log(`- Some files are in the \`${UNASSIGNED}\` holding area (no owner yet) - CODEOWNERS applies.`);
  }
  if (unmatched.length > 0) {
    log(`- ${unmatched.length} file(s) match no area - CODEOWNERS applies:`);
    for (const f of unmatched.slice(0, 10)) log(`  - ${code(f)}`);
    if (unmatched.length > 10) log(`  - ...and ${unmatched.length - 10} more`);
  }

  const involved = await alreadyInvolved(pr);
  const reviewers = uniqueHandles(owned.flatMap((a) => a.reviewers)).filter(
    (login) => !involved.has(login.toLowerCase()),
  );

  log('');
  if (reviewers.length === 0) {
    log('Nothing to request - every matching owner is already on, or has already been on, this PR.');
    return;
  }

  if (dryRun) {
    log(`Would request: ${reviewers.map((r) => `@${r}`).join(', ')}`);
    log('');
    log('_Dry run - set the `AREA_REVIEWERS_DRY_RUN` repository variable to `false` to enable._');
    return;
  }

  await requestReviewers(gh, repo, number, reviewers, log);
}

main()
  .catch((err) => {
    log('');
    log(`**Area reviewers failed:** ${err.message}`);
    annotate('error', `Area reviewers failed: ${err.message}`);
    process.exitCode = 1;
  })
  .finally(flush);
