#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
site_dir="$repo_root/build-web/web"
if [[ ! -s "$site_dir/index.html" || ! -s "$site_dir/chess_core.wasm" ]]; then
  printf 'Browser build not found. Build it first with the commands in README.md.\n' >&2
  exit 1
fi

worktree_root="$(mktemp -d)"
site_worktree="$worktree_root/site"
cleanup() {
  git -C "$repo_root" worktree remove --force "$site_worktree" >/dev/null 2>&1 || true
  rm -rf -- "$worktree_root"
}
trap cleanup EXIT

if git -C "$repo_root" ls-remote --exit-code --heads origin gh-pages >/dev/null 2>&1; then
  git -C "$repo_root" fetch --quiet origin gh-pages
  git -C "$repo_root" worktree add --detach "$site_worktree" FETCH_HEAD >/dev/null
else
  git -C "$repo_root" worktree add --detach "$site_worktree" HEAD >/dev/null
fi

find "$site_worktree" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf -- {} +
cp -a "$site_dir"/. "$site_worktree"/
touch "$site_worktree/.nojekyll"
git -C "$site_worktree" add --all

if git -C "$site_worktree" diff --cached --quiet; then
  printf 'The gh-pages branch already contains this browser build.\n'
  exit 0
fi

git -C "$site_worktree" commit -m "Deploy browser chess"
git -C "$site_worktree" push origin HEAD:refs/heads/gh-pages
