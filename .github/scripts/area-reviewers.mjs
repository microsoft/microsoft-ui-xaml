// Requests optional reviewers on a PR: every area in .github/area-owners.yml whose
// paths match a changed file contributes its reviewers. Nothing here blocks a merge.

import fs from 'node:fs';
import yaml from 'js-yaml';

const token = process.env.GITHUB_TOKEN;
const repo = process.env.GITHUB_REPOSITORY;
const dryRun = (process.env.AREA_REVIEWERS_DRY_RUN ?? 'true').toLowerCase() !== 'false';

const pr = JSON.parse(fs.readFileSync(process.env.GITHUB_EVENT_PATH, 'utf8')).pull_request;

const out = [];
const log = (line) => {
  console.log(line);
  out.push(line);
};

async function api(route, init = {}) {
  const res = await fetch(`https://api.github.com${route}`, {
    ...init,
    headers: {
      accept: 'application/vnd.github+json',
      authorization: `Bearer ${token}`,
      'content-type': 'application/json',
    },
  });
  return { ok: res.ok, status: res.status, body: await res.json().catch(() => null) };
}

// Glob -> regex. `**` crosses directories, `*` does not, everything else is literal.
function toRegExp(glob) {
  const source = glob
    .replace(/[.+^${}()|[\]\\]/g, '\\$&')
    .replace(/\*\*\//g, '\u0000')
    .replace(/\*\*/g, '\u0001')
    .replace(/\*/g, '[^/]*')
    .replace(/\?/g, '[^/]')
    .replace(/\u0000/g, '(?:.*/)?')
    .replace(/\u0001/g, '.*');
  return new RegExp(`^${source}$`, 'i');
}

async function changedFiles() {
  const files = [];
  for (let page = 1; ; page++) {
    const { body } = await api(`/repos/${repo}/pulls/${pr.number}/files?per_page=100&page=${page}`);
    if (!Array.isArray(body) || body.length === 0) return files;
    files.push(...body.map((f) => f.filename));
    if (body.length < 100) return files;
  }
}

async function main() {
  const areas = yaml.load(fs.readFileSync('.github/area-owners.yml', 'utf8')).areas;
  const files = await changedFiles();

  const matched = areas.filter((area) =>
    area.paths.some((glob) => files.some((file) => toRegExp(glob).test(file))),
  );

  log(`### Area reviewers${dryRun ? ' (dry run)' : ''}`);
  log('');
  log(`PR #${pr.number} - ${files.length} changed file(s), ${matched.length} area(s) matched.`);
  for (const area of matched) {
    log(`- \`${area.id}\` ${area.name}: ${area.reviewers.map((r) => `@${r}`).join(', ')}`);
  }

  // Requesting the author, or someone already on the PR, is rejected by the API.
  const onPr = new Set(
    [pr.user, ...(pr.requested_reviewers ?? [])].map((u) => u.login.toLowerCase()),
  );
  const reviewers = [...new Set(matched.flatMap((area) => area.reviewers))].filter(
    (login) => !onPr.has(login.toLowerCase()),
  );

  log('');
  if (reviewers.length === 0) {
    log('Nothing to request.');
    return;
  }

  const names = reviewers.map((r) => `@${r}`).join(', ');

  if (dryRun) {
    log(`Would request: ${names}`);
    log('');
    log('_Dry run - set the `AREA_REVIEWERS_DRY_RUN` repository variable to `false` to enable._');
    return;
  }

  const { ok, status } = await api(`/repos/${repo}/pulls/${pr.number}/requested_reviewers`, {
    method: 'POST',
    body: JSON.stringify({ reviewers }),
  });
  log(ok ? `Requested: ${names}` : `Could not request ${names} (HTTP ${status}).`);
}

main()
  .catch((err) => log(`Area reviewers failed: ${err.message}`))
  .finally(() => {
    if (process.env.GITHUB_STEP_SUMMARY) {
      fs.appendFileSync(process.env.GITHUB_STEP_SUMMARY, `${out.join('\n')}\n`);
    }
  });
