# Releasing a new version of gruepr

End-to-end checklist for cutting a release. The recurring steps come first (start at
**Part A**). The **One-time setup** at the end describes the automation those steps
assume — build that first if it isn't in place yet.

The release runs as **two workflows**:

- **Build** (`Build.yaml`, you run it) — builds the three executables and creates a
  **draft** release whose notes are generated from your `// DONE:` items. Nothing is
  committed.
- **Release** (`Release.yaml`, you run it after reviewing the draft) — running it is
  your approval. It publishes the reviewed draft, writes the notes into
  `CHANGELOG.md`, clears the `// DONE:` block in `main.cpp`, commits both to `master`,
  and triggers the gruepr.com deploy. It's manually triggered (not `on: release`), so
  "Release" is an accurate name — it's the workflow that performs the release.

A few facts the process relies on:

- **The version has a single source of truth:** line 7 of `gruepr.pro`
  (`gruepr_version = 13.1`). It flows into `GRUEPR_VERSION_NUMBER`, the `VERSION`
  embedded in `gruepr.exe`, and the NSIS installer reads it back out of the exe
  automatically. You never hand-edit a version in the installer script. A two-part
  value is fine — the automation normalizes it to a three-part tag (`v13.1.0`).
- **Release notes come from your `// DONE:` notes in `main.cpp`** — you don't
  hand-write `CHANGELOG.md`. Build turns DONE into the draft body; the Release
  workflow writes the published notes into `CHANGELOG.md` and clears DONE.
- **The Release workflow is the point of no return.** It publishes the GitHub release
  (which becomes `releases/latest`, driving the in-app upgrade check in
  `startDialog.cpp` and the `releases/latest/download/...` buttons on gruepr.com) and
  is the only thing that commits anything. Nothing is published or committed until you
  run it.

---

## Part A — Code, version, and DONE notes (main `gruepr` repo)

1. **Make and test the code changes** for this release.

2. **Bump the version.** Edit line 7 of `gruepr.pro`:
   ```
   gruepr_version = 13.2
   ```
   Two-part (`13.2`) or three-part (`13.2.0`) both work; the tag comes out three-part
   either way. If the calendar year has rolled over, update `copyright_year` (line 8 of
   `gruepr.pro`) — that's now the only place to change it; both the app and the NSIS
   installer read the year from there.

3. **Record what you finished under `// DONE:` in `main.cpp`.** As you complete work,
   move or write the items into the `// DONE:` block (above `// TO DO:`), in the same
   comment style as the existing list:
   ```
   // DONE:
   //    BUG FIXES:
   //  - fixed Canvas import crash for students who had not submitted a survey
   //  - parse LMS IDs as long long instead of int
   //  - added experimental -O3 optimization setting
   //      - falls back to -O2 if the compiler chokes
   //
   // TO DO:
   ```
   These become the release notes. `//  - text` is a bullet, a deeper-indented
   `//      - text` is a sub-bullet, and a `SOMETHING:` line becomes a bold
   sub-header. Anything under `// TO DO:` is ignored. Write them user-facing — you can
   still polish the wording in the draft before publishing.

4. **Commit and push to `master`** (code + `gruepr.pro` + the `main.cpp` DONE notes).
   You do **not** edit `CHANGELOG.md` by hand — the Release workflow does that.

---

## Part B — Build and draft the release (Workflow 1)

5. In the `gruepr` repo: **Actions** tab → the combined **Build** workflow
   (`Build.yaml`) → **Run workflow** (on `master`). This is the only workflow used for
   releasing — it builds all three platforms in one run. The per-platform
   `Build_Windows.yaml`, `Build_macOS.yaml`, and `Build_linux.yaml` workflows are for
   testing individual platforms and play no part in releasing.

6. **Wait for the build — and approve the code signing twice.** All four jobs in
   `Build.yaml` run: `build-windows`, `build-macos`, `build-linux`, and
   `draft-release` (jobs *inside* the one workflow). **The `build-windows` job pauses
   twice for signing:** it submits the `gruepr.exe`, then the Windows installer, to
   SignPath under the `release-signing` policy, and waits (`wait-for-completion`) for
   you each time. **Log into the SignPath web console and approve both requests** as
   they appear — the build will not finish otherwise. (macOS signing and notarization
   are automatic via Apple's tooling, so there's nothing to approve there.) When all
   jobs complete, `draft-release` has normalized the version from `gruepr.pro`,
   generated the notes from the `// DONE:` items, and created a **draft** release
   tagged `vX.Y.Z` with all three assets attached.

---

## Part C — Review the draft, then run Release (Workflow 2)

Between the two workflows, **the only thing to review is the draft release on
GitHub** — nothing has been committed to the repo yet, so there are no file diffs to
read. The `CHANGELOG.md` entry and the gruepr.com changes are derived from the draft
notes, so reviewing the notes is reviewing the changelog. (If you'd rather eyeball the
actual `CHANGELOG.md` / `content.js` diffs before they land, the Release workflow can
open a pull request instead of committing directly — say the word and I'll switch it.)

7. **Review the draft.** Open it under **Releases** (or from the Build run summary)
   and check:
   - tag is `vX.Y.Z`,
   - all three assets are attached: `install_gruepr.exe`, `gruepr.dmg`,
     `gruepr.AppImage`,
   - the notes read well — **edit them right here** if any `// DONE:` wording was too
     terse. Whatever the draft says when you run Release is exactly what gets published
     and written to `CHANGELOG.md`.

8. **Run the Release workflow.** Actions tab → **Release** → **Run workflow**. Running
   it is your approval. It publishes the draft (which becomes `releases/latest`,
   lighting up the in-app upgrade notice and the gruepr.com download buttons), prepends
   `## vX.Y.Z — <today>` plus the published notes to `CHANGELOG.md`, clears the
   `// DONE:` block in `main.cpp`, commits both to `master` (`[skip ci]`), and triggers
   the gruepr.com deploy (Part D).

9. Confirm the Release run succeeded (Actions tab) and that the new commit landed on
   `master`.

---

## Part D — gruepr.com update (`gruepr-webapp` repo)

Once the `WEBAPP_DISPATCH_TOKEN` secret is set (One-time setup #6), **this whole part
is automatic** — the Release workflow dispatches to `deploy.yaml` in `gruepr-webapp`,
which updates the version/date in `content.js`, publishes the new installer under
`public/downloads/vX.Y/`, and deploys the site (the changelog page re-renders from
`CHANGELOG.md` on its own). You don't do anything here; just confirm the deploy run
went green and spot-check gruepr.com.

**If you ever need to do it by hand** (the secret isn't set, or a deploy failed),
the manual steps are below (conventions: edit in `src/`, GitHub Desktop for git,
PowerShell for `npm`, never touch `gh-pages` directly, `npm install` once per fresh
checkout):

10. Pull latest `main` in GitHub Desktop; `npm install` if it's a fresh checkout.

11. Edit `content.js`: bump the displayed version and release date. Download buttons
    that use `releases/latest/download/...` need no change. Only touch a download URL
    if it is version-stamped.

12. Copy `install_gruepr.exe` (from the published release) into a new folder
    `public/downloads/vX.Y/install_gruepr.exe`. This gruepr.com folder keeps the
    short two-part form (e.g. `v13.2`), matching your existing `v13.0` folder — only
    the Git tag and changelog headings use three parts.

13. The changelog page renders straight from `CHANGELOG.md` (One-time setup #5), so
    there's nothing to edit for the changelog itself.

14. Preview with `npm start`; confirm the changelog shows `vX.Y.Z` and links resolve.

15. Commit + push to `main`, run `npm run deploy`, then verify on gruepr.com with
    **Ctrl+F5**. Test the hosted installer URL directly:
    `https://www.gruepr.com/downloads/vX.Y/install_gruepr.exe`.

---

## Part E — Microsoft Store (Partner Center)

Because the Store submission points at a **versioned** URL, every release needs a new
submission. This stays manual.

16. In Partner Center, update the package installer URL to
    `https://www.gruepr.com/downloads/vX.Y/install_gruepr.exe` and set the declared
    version.

17. Submit for certification. It takes a few business days; once it passes, the Store
    pushes the update to Store users automatically.

---

## Quick reference

**Where the version is set:** `gruepr.pro` line 7 — nowhere else.

**Where release notes come from:** the `// DONE:` block in `main.cpp`. They flow into
the draft, then into `CHANGELOG.md` when you run Release. Never hand-edit
`CHANGELOG.md`.

**Version forms:** the Git tag and `CHANGELOG.md` headings use three parts
(`v13.2.0`); the automation normalizes a two-part value to three. The gruepr.com
installer folder under `public/downloads/` uses the short two-part form (`v13.2`).
The Microsoft Store has its own version field in Partner Center.

**Release asset names (must stay stable so `releases/latest/download/<name>` keeps
working):**

| Platform | CI artifact name              | File in artifact                 | Published as          |
|----------|-------------------------------|----------------------------------|-----------------------|
| Windows  | `gruepr-signed-windows-installer` | `install_gruepr.exe`         | `install_gruepr.exe`  |
| macOS    | `gruepr-macos-universal`      | `Install gruepr.dmg`             | `gruepr.dmg`          |
| Linux    | `gruepr-linux-appimage`       | `gruepr-linux-x86_64.AppImage`   | `gruepr.AppImage`     |

**Two Windows copies, on purpose:** the GitHub release asset
(`releases/latest/download/install_gruepr.exe`) serves direct downloads and the
in-app check; the gruepr.com `/downloads/vX.Y/install_gruepr.exe` copy exists only
because the Microsoft Store needs a stable, fetchable URL.

**Annual:** the copyright year lives only in `gruepr.pro` line 8 (`copyright_year`).
The app embeds it at compile time, and the NSIS installer reads it from there (CI
passes it in; local builds parse it out of `gruepr.pro`). Nothing else to touch at the
new year.

---

## One-time setup

Implement and test these **one at a time** — the order below works. Items 1–4 are the
`gruepr` repo (no special credentials). Items 5–6 are the gruepr.com automation.

### 1. `CHANGELOG.md` — done

Already generated from your live gruepr.com changelog: 57 entries, newest first,
three-part `## vX.Y.Z` headings, dates for v10.8.0–v13.0.0 (older versions predate
GitHub releases). Place it at the repo root. From here on it's maintained
automatically by the Release workflow.

### 2. `changelog.py` — done

Commit the provided script to the **gruepr** repo at `.github/workflows/changelog.py`.
GitHub only runs `.yml`/`.yaml` files in that folder as workflows, so the script just
sits there as a regular file. It runs **only on the GitHub Actions runner**, during the
Build and Release workflows — you never run it locally. The Build workflow calls
`extract-done` (main.cpp DONE block → Markdown bullets); the Release workflow calls
`prepend` (add a dated section to `CHANGELOG.md`) and `clear-done` (empty the DONE
block). GitHub's Ubuntu runners have `python3` preinstalled, so there's nothing else to
set up.

### 3. `draft-release` job in `Build.yaml` (Workflow 1)

Append this job. It builds the notes from `// DONE:`, names assets to match the
stable download URLs, and leaves the release as a draft:

```yaml
  draft-release:
    needs: [build-windows, build-macos, build-linux]
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - uses: actions/checkout@v6

      - name: Download all build artifacts
        uses: actions/download-artifact@v6
        with:
          path: artifacts

      - name: Read and normalize version from gruepr.pro
        id: ver
        run: |
          RAW=$(grep -m1 'gruepr_version' gruepr.pro | sed 's/.*=//' | tr -d '[:space:]')
          VERSION=$(echo "$RAW" | awk -F. '{ if (NF==1) print $1".0.0"; else if (NF==2) print $1"."$2".0"; else print $0 }')
          echo "version=$VERSION" >> "$GITHUB_OUTPUT"

      - name: Build release notes from DONE items
        run: |
          python3 .github/workflows/changelog.py extract-done --main main.cpp > release_notes.md
          test -s release_notes.md || { echo "::error::No DONE items found in main.cpp"; exit 1; }

      - name: Stage assets with stable names
        run: |
          mkdir release-assets
          cp "artifacts/gruepr-signed-windows-installer/install_gruepr.exe" "release-assets/install_gruepr.exe"
          cp "artifacts/gruepr-macos-universal/Install gruepr.dmg"          "release-assets/gruepr.dmg"
          cp "artifacts/gruepr-linux-appimage/gruepr-linux-x86_64.AppImage" "release-assets/gruepr.AppImage"

      - name: Create draft release
        uses: softprops/action-gh-release@v2
        with:
          draft: true
          tag_name: v${{ steps.ver.outputs.version }}
          name: gruepr v${{ steps.ver.outputs.version }}
          body_path: release_notes.md
          files: release-assets/*
```

Verify on the first run: (a) the three `cp` source paths match how
`download-artifact` lays out the folders, (b) `extract-done` produces the bullets you
expect from your DONE block, (c) the normalized version yields the right three-part
tag, and (d) the asset names match what the gruepr.com download buttons link to. (The
Linux button has historically linked to `gruepr.AppImage`, which is why it's renamed.)

### 4. `Release.yaml` (Workflow 2 — Release)

Add the provided `Release.yaml` as a new workflow. It's manually triggered
(`workflow_dispatch`). When you run it, it finds the draft release for the version in
`gruepr.pro`, publishes it, captures the (reviewed) notes, prepends them to
`CHANGELOG.md`, clears the DONE block in `main.cpp`, and commits both to `master`. It
needs no special credentials for those steps. Its final step dispatches the gruepr.com
deploy and is **guarded** — it skips cleanly until the `WEBAPP_DISPATCH_TOKEN` secret
exists (#6), so you can add this workflow now and the deploy trigger lights up later.

### 5. Render `CHANGELOG.md` on gruepr.com — replace `ChangeLog.js`

Replace the changelog page component with the provided `ChangeLog.js`. It drops the
`change-log-content.js` import and instead fetches `CHANGELOG.md` from the gruepr repo
at runtime and renders it with `react-markdown`, styled to match the site (it maps
headings/lists onto your `heading3` / `body list-disc` classes and hides the duplicate
top title). One install in the webapp repo:

```
npm install react-markdown
```

After this, `change-log-content.js` is no longer used and can be deleted. The Download
page's buttons already point at `releases/latest/download/...`, so none of them need
per-release edits.

### 6. Webapp auto-deploy — add `deploy.yaml`, set one secret

This makes Part D fully automatic. Add the provided `webapp-deploy.yaml` to the
`gruepr-webapp` repo as `.github/workflows/deploy.yaml`. On the `gruepr-release`
dispatch from the Release workflow it: updates the `downloadPage.version` line in
`content.js` (new version + date), downloads the just-published `install_gruepr.exe`
and writes it to `public/downloads/vX.Y/` (the URL the Microsoft Store pulls from),
commits both, then runs `npm ci && npm run deploy` to publish the site. It can also be
run by hand (`workflow_dispatch`) with a version and date if you need to re-deploy.

**The one thing you must set up:** a **fine-grained PAT** with `contents: write` (and
"Read and write" on Actions, so the dispatch is allowed) on `gruepr-webapp`, saved in
the **gruepr** repo as the `WEBAPP_DISPATCH_TOKEN` secret. That's the only credential
the chain needs — `GITHUB_TOKEN` can't reach another repo, but once this secret exists
the Release workflow's guarded final step activates and the whole publish→deploy chain
runs end to end. (A GitHub App is the longer-lived alternative if you'd rather not
rotate a PAT.)

Worth knowing: `deploy.yaml` commits the signed `install_gruepr.exe` into the webapp
repo each release (matching your existing `v13.0` folder), which grows the repo over
time. That's by design — the Microsoft Store submission needs a stable, direct
download URL, and Partner Center will not accept the GitHub `releases/latest/...`
link because that's a redirect rather than a real endpoint. So the versioned
`public/downloads/vX.Y/` hosting is the supported path.

### 7. Single-source the NSIS copyright year (main `gruepr` repo)

Replace `gruepr_InstallScript.nsi` with the provided version. Instead of a hardcoded
`(c) 2019-2026`, it now defines `COPYRIGHT_YEAR` from `gruepr.pro` and uses it in the
`LegalCopyright` version key. In CI the value is passed in (trimmed) so there's no
chance of a stray carriage return; local builds read it directly from `gruepr.pro`.

That requires one small change to the existing **`Make installer`** step in
`Build.yaml`'s `build-windows` job — read the year and pass it as `/DCOPYRIGHT_YEAR`:

```powershell
- name: Make installer
  run: |
       $year = ((Select-String -Path gruepr.pro -Pattern '^copyright_year').Line -split '=')[1].Trim()
       & "C:\Program Files (x86)\NSIS\makensis.exe" /DBUILDDIR="$pwd" /DBASELOCATION="$pwd\.." /DCOPYRIGHT_YEAR="$year" /DCI windows\gruepr_InstallScript.nsi
```

After this, the year you set in `gruepr.pro` line 8 flows to the app *and* the
installer — nothing else to edit at the new year.
