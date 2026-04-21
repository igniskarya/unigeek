---
name: sync-docs
description: Sync README.md, knowledge files, and catalog.js with recent firmware commits
---

# sync-docs

Sync README.md, knowledge/*.md, and website/content/features/catalog.js with recent firmware commits.

## Steps

1. **Find last sync commit**
   Read README.md and extract the SHA from the marker:
   `<!-- README last synced at commit: <sha> -->`

2. **List new commits since last sync**
   Run: `git log --oneline <last_sha>..HEAD`
   If no new commits, report "Already up to date." and stop.

3. **Inspect each commit**
   For each commit SHA, run: `git show --stat <sha>`
   Note which source files changed (screens/, core/, boards/, sdcard/, etc.)

4. **Read changed source files**
   For each relevant changed file, read it to understand what was added or modified.
   Focus on: new screens, new features, changed menu structure, new games, new achievements.

5. **Update README.md**
   - Add or update feature entries in the relevant section (WiFi, Bluetooth, Keyboard, Modules, Utility, Games, Settings)
   - Keep the existing format and style — bullet points with sub-bullets for sub-features
   - Do NOT rewrite unrelated sections
   - Update the sync marker at the end: `<!-- README last synced at commit: <new_sha> -->`
   - Get `<new_sha>` from: `git rev-parse HEAD`

6. **Update knowledge/*.md files**
   - If a commit adds a significant feature that already has a knowledge file, update that file
   - If a commit adds a major new feature with `hasDetail: true` in catalog.js, create a new knowledge file
   - Knowledge files live in `knowledge/<slug>.md`

   Knowledge files are rendered on the website by `website/content/features/index.js` via `marked`. Follow these conventions so they render correctly:
   - **First heading must be `# Title`** — the renderer strips the first H1 and uses the catalog entry's `title` instead. Keep it as a human-readable page title; never start a doc with H2.
   - **Callouts** — use GitHub-style callout blockquotes:
     - `> [!note]` — neutral info (green NOTE badge)
     - `> [!tip]` — helpful tip (green TIP badge)
     - `> [!warn]` — authorization / caution (amber WARN badge)
     - `> [!danger]` — legally risky / destructive (red DANGER badge)
     - Place the marker on its own line, then the content on the next line(s). Plain `>` blockquotes still render with a default NOTE badge.
   - **Tier pills** — in any table body, a cell containing exactly `Bronze`, `Silver`, `Gold`, or `Platinum` auto-renders as a coloured tier pill. Use these literal words for achievement-tier columns.
   - **Tables** — standard GFM tables render as bordered tables with uppercase mono headers. Keep column counts small (3–5) so they stay readable on mobile.
   - **Code fences** — triple-backtick blocks render in mono. Inline `code` is boxed.
   - **Relative file links** — do not link to files outside the repo root (`../docs/...`) because the website can't resolve them. Use absolute URLs or `code` refs instead.
   - **Heading hierarchy** — use `##` for top-level sections, `###` for subsections. Deeper nesting rarely reads well on mobile.

7. **Update website/content/features/catalog.js**
   - Add new entries for new features
   - Update summaries for changed features
   - Update `hasDetail` if a knowledge file was added or removed
   - Match slug to the knowledge filename (without .md)
   - Categories: `wifi`, `ble`, `keyboard`, `module`, `utility`, `game`, `setting`

8. **Report what changed**
   List each file updated and a one-line summary of what was changed.