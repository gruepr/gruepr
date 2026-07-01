# Releasing a new version of gruepr

End-to-end checklist for cutting a release — start at **Part A**. The pipeline that
powers these steps is already built; the only thing that needs occasional attention is
covered in **Ongoing maintenance** at the end.

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

**This whole part is automatic** — the Release workflow dispatches to `deploy.yaml` in
`gruepr-webapp`, which updates the version/date in `content.js`, publishes the new
installer under `public/downloads/vX.Y/`, and deploys the site (the changelog page
re-renders from `CHANGELOG.md` on its own). You don't do anything here; just confirm the
deploy run went green and spot-check gruepr.com. (It relies on the
`WEBAPP_DISPATCH_TOKEN` — if it ever stops firing, see Ongoing maintenance.)

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

13. The changelog page renders straight from `CHANGELOG.md` (via react-markdown in
    `ChangeLog.js`), so there's nothing to edit for the changelog itself.

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

## Ongoing maintenance

Almost everything runs itself. The few things that will need attention over time are
credentials that expire — most importantly the token that lets a release deploy the
website.

### The `WEBAPP_DISPATCH_TOKEN` (expires — plan to re-create it)

This is the fine-grained PAT that lets the Release workflow (in `gruepr`) trigger the
deploy workflow (in `gruepr-webapp`). Fine-grained tokens can't be permanent, so when
it lapses the release itself still works, but the final "Trigger gruepr.com deploy"
step stops firing and the site won't update (a manual API call would return
`403 Resource not accessible`). When that happens — or before it does — re-create it:

1. GitHub → your avatar → **Settings** → **Developer settings** → **Personal access
   tokens** → **Fine-grained tokens** → **Generate new token**.
2. Name it, set an expiration, **Resource owner:** `gruepr`.
3. **Repository access:** Only select repositories → **`gruepr-webapp`**.
4. **Repository permissions:** **Contents → Read and write** (Metadata: Read is added
   automatically). Nothing else is needed.
5. **Generate token** and copy the `github_pat_...` value (shown once).
6. In the **`gruepr`** repo → **Settings** → **Secrets and variables** → **Actions** →
   open **`WEBAPP_DISPATCH_TOKEN`** → **Update secret**, paste the new value.

The token grants access to `gruepr-webapp`, but the secret lives in `gruepr` — that
crossing is the part that's easy to get backwards. If `gruepr` is an organization, its
"Allow access via fine-grained personal access tokens" setting must be on, or the
dispatch returns a 403 regardless of the token's permissions.

### Other expiring credentials

If a build (rather than the deploy) suddenly fails, suspect an expired signing
credential: the Apple Developer certificate used for the macOS notarization
(`APPLE_CERTIFICATE` and friends) renews annually, and the SignPath token/certificate
behind the two Windows signing approvals can lapse too. These predate this pipeline and
live in the same repo secrets; renew them the same way you originally set them up.

### The copyright year

Handled in the release flow (Part A, step 2): bump `copyright_year` in `gruepr.pro`
line 8 at the new year and it flows to both the app and the installer. No separate
action needed.
