# Releasing a new version of gruepr

End-to-end checklist for cutting a release — start at **Part A**. The pipeline that
powers these steps is already built; the only thing that needs occasional attention is
covered in **Ongoing maintenance** at the end.

The release runs as **two workflows**:

* **Build** (`Build.yaml`, you run it) — builds the three executables and creates a
**draft** release whose notes are generated from your `// DONE:` items. Nothing is
committed.
* **Release** (`Release.yaml`, you run it after reviewing the draft) — running it is
your approval. It publishes the reviewed draft, writes the notes into
`CHANGELOG.md`, clears the `// DONE:` block in `main.cpp`, commits both to `master`,
and triggers the gruepr.com deploy.

A few facts the process relies on:

* **The version and copyright year have a single source of truth:** lines 7-8 of
`gruepr.pro` (`gruepr\_version = 13.1`). It flows into `GRUEPR\_VERSION\_NUMBER`,
the `VERSION` embedded in `gruepr.exe`, and the NSIS installer reads it back out of
the exe automatically. A two-part value is fine — the automation normalizes it to a
three-part tag (`v13.1.0`).
* **Release notes come from your `// DONE:` notes in `main.cpp`** — you don't
hand-write `CHANGELOG.md`. Build turns DONE into the draft body; the Release
workflow writes the published notes into `CHANGELOG.md` and clears DONE.
* **The Release workflow is the point of no return.** It publishes the GitHub release
(which becomes `releases/latest`, driving the in-app upgrade check in
`startDialog.cpp` and the `releases/latest/download/...` buttons on gruepr.com) and
is the only thing that commits anything. Nothing is published or committed until you
run it.

\---

## Part A — Code, version, and DONE notes (main `gruepr` repo)

1. **Make and test the code changes** for this release.
2. **Bump the version and check the copyright year.** Edit lines 7-8 of `gruepr.pro`:

```
   gruepr\_version = 13.2
   ```

   Two-part (`13.2`) or three-part (`13.2.0`) both work; the tag comes out three-part
either way. If the calendar year has rolled over, update `copyright\_year`.

3. **Record what you finished under `// DONE:` in `main.cpp`.** As you complete work,
move or write the items into the `// DONE:` block (above `// TO DO:`), in the same
comment style as the existing list:

   ```
   // DONE:
   //  - bugfix: Canvas import crash for students who had not submitted a survey
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

   \---

   ## Part B — Build and draft the release (Workflow 1)

5. In the `gruepr` repo: **Actions** tab → the combined **Build** workflow
(`Build.yaml`) → **Run workflow** (on `master`). This is the only workflow used for
releasing — it builds all three platforms in one run. The per-platform
`Build\_Windows.yaml`, `Build\_macOS.yaml`, and `Build\_linux.yaml` workflows are for
testing individual platforms and play no part in releasing.
6. **Wait for the build — and approve the code signing twice.** All four jobs in
`Build.yaml` run: `build-windows`, `build-macos`, `build-linux`, and
`draft-release` (jobs *inside* the one workflow). **The `build-windows` job pauses
twice for signing:** it submits the `gruepr.exe`, then the Windows installer, to
SignPath under the `release-signing` policy, and waits (`wait-for-completion`) for
you each time. **Log into the SignPath web console and approve both requests** as
they appear — the build will not finish otherwise. (macOS signing and notarization
are automatic via Apple's tooling, so there's nothing to approve there.)

   \---

   ## Part C — Review the draft, then run Release (Workflow 2)

7. **Review the draft.** Open it under **Releases** (or from the Build run summary)
and check:

   * tag is `vX.Y.Z`,
   * the title reads `vX.Y.Z`,
   * all three assets are attached: `install\_gruepr.exe`, `gruepr.dmg`,
`gruepr.AppImage`,
   * the notes read well — **edit them right here** if any `// DONE:` wording was too
terse. Whatever the draft says when you run Release is exactly what gets published
and written to `CHANGELOG.md`.
8. **Run the Release workflow.** Actions tab → **Release** → **Run workflow**. Running
it is your approval. It publishes the draft (which becomes `releases/latest`,
lighting up the in-app upgrade notice and the gruepr.com download buttons), prepends
`## vX.Y.Z — <today>` plus the published notes to `CHANGELOG.md`, clears the
`// DONE:` block in `main.cpp`, commits both to `master`, and triggers the gruepr.com
deploy (Part D).
9. Confirm the Release run succeeded (Actions tab) and that the new commit landed on
`master`.

   \---

   ## Part D — gruepr.com update (`gruepr-webapp` repo)

   **This whole part is automatic** — the Release workflow dispatches to `deploy.yaml` in
`gruepr-webapp`, which updates the version/date in `content.js`, publishes the new
installer under `public/downloads/vX.Y/`, and runs `npm run deploy` to push the built
site to the `gh-pages` branch (the changelog page re-renders from `CHANGELOG.md` on its
own). That push then kicks off GitHub's own **"pages build and deployment"** run — a
second, GitHub-managed workflow that actually serves the new content. So a complete
deploy is **two green runs**: your `deploy.yaml`, then the Pages build that starts by
itself afterward.

   You don't do anything here except confirm. Give Pages a minute after both runs go
green, then **Ctrl+F5** on gruepr.com and check:

* the page shows the new version and date,
* the Changelog page renders the list (not the "couldn't be loaded" fallback),
* `https://www.gruepr.com/downloads/vX.Y/install\_gruepr.exe` downloads.

  If the deploy *doesn't fire at all*, that's the `WEBAPP\_DISPATCH\_TOKEN` — see Ongoing
maintenance. (The deploy workflow itself already handles the two things that can trip a
CRA deploy in CI — treating build warnings as non-fatal, and authenticating the
`gh-pages` push — so those shouldn't recur.)

  **If you ever need to do it by hand** (the secret isn't set, or a deploy failed),
the manual steps are below (conventions: edit in `src/`, GitHub Desktop for git,
PowerShell for `npm`, never touch `gh-pages` directly, `npm install` once per fresh
checkout):

10. Pull latest `main` in GitHub Desktop; `npm install` if it's a fresh checkout.
11. Edit `content.js`: bump the displayed version and release date. Download buttons
that use `releases/latest/download/...` need no change. Only touch a download URL
if it is version-stamped.
12. Copy `install\_gruepr.exe` (from the published release) into a new folder
`public/downloads/vX.Y/install\_gruepr.exe`. This folder uses the version string
exactly as it appears in `gruepr.pro` (e.g. `v13.0.2`) — only the Git tag and
changelog headings get normalized to three parts.
13. The changelog page renders straight from `CHANGELOG.md` (via react-markdown in
`ChangeLog.js`), so there's nothing to edit for the changelog itself.
14. Preview with `npm start`; confirm the changelog shows `vX.Y.Z` and links resolve.
15. Commit + push to `main`, run `npm run deploy`, then verify on gruepr.com with
**Ctrl+F5**. Test the hosted installer URL directly:
`https://www.gruepr.com/downloads/vX.Y/install\_gruepr.exe`.

    \---

    ## Part E — Microsoft Store (Partner Center)

    Manual, every release. gruepr is on the Store as an **EXE app** that points at a
**versioned installer URL**, and that URL changes each version — Partner Center
requires a fixed, versioned URL and rejects a `latest`-style redirect. The app already
exists in Partner Center, so each release is an **update submission**, and in practice
**the only field that changes is the Package URL** — the rest carry over from the last
submission.

16. Sign in to Partner Center → **Apps and games** → **gruepr** (Store ID
`xpdc98f0ts7gcs`). Start a new submission (the **Update** / "Create submission"
button on the app overview), open the **Packages** section, and open the existing
package to edit it.
17. Set the package details. The values gruepr uses:

|Field|Value|Change each release?|
|-|-|-|
|**Package URL**|`https://www.gruepr.com/downloads/vX.Y/install\_gruepr.exe`|**Yes** — point at the new version's folder|
|**Architecture**|x64|No|
|**Languages**|English (`en`)|No|
|**App type**|EXE|No|
|**Installer parameters**|`/S` (NSIS's silent-install switch — capital S)|No|
|**Installer handling** (return codes)|Optional — leave as set|No|

    The installer at that URL is already hosted for you: Part D's deploy publishes it to
`public/downloads/vX.Y/`. Use the **exact** folder name the deploy created (it's your
`gruepr.pro` version string, e.g. `v13.0.2`) — the simplest way to be sure is to
open `https://www.gruepr.com/downloads/vX.Y/install\_gruepr.exe` in a browser and
confirm it downloads, then paste that URL.

18. Save the package, then **Submit** for certification. It takes a few business days;
once it passes, the Store pushes the update to Store users automatically. (The GitHub
release and the gruepr.com direct-download buttons are already live from the earlier
parts — this governs only the Microsoft Store copy.)

    Two things not to trip on: the binary at a submitted URL must **never change** after
submission, which is automatically satisfied because every release gets a fresh
`vX.Y/` folder — so never overwrite an old version's installer. And `/S` is what lets
the Store install silently (a UAC prompt is still allowed); if a future installer
change ever breaks silent install, certification will fail there.

    \---

    ## Quick reference

    **Where the version and copyright year is set:** `gruepr.pro` line 7-8.

    **Where release notes come from:** the `// DONE:` block in `main.cpp`. They flow into
the draft, then into `CHANGELOG.md` when you run Release.

    **Version forms:** the Git tag and `CHANGELOG.md` headings use three parts
(`v13.2.0`) — the automation normalizes a two-part value to three. The gruepr.com
installer folder under `public/downloads/` and the version text on the site use the
string exactly as you typed it in `gruepr.pro` (so `13.0.2` → folder `v13.0.2`, but
`13.2` → folder `v13.2`). The Microsoft Store Package URL points at that same folder.

    **Release asset names (must stay stable so `releases/latest/download/<name>` keeps
working):**

|Platform|CI artifact name|File in artifact|Published as|
|-|-|-|-|
|Windows|`gruepr-signed-windows-installer`|`install\_gruepr.exe`|`install\_gruepr.exe`|
|macOS|`gruepr-macos-universal`|`Install gruepr.dmg`|`gruepr.dmg`|
|Linux|`gruepr-linux-appimage`|`gruepr-linux-x86\_64.AppImage`|`gruepr.AppImage`|

**Two Windows copies, on purpose:** the GitHub release asset
(`releases/latest/download/install\_gruepr.exe`) serves direct downloads and the
in-app check; the gruepr.com `/downloads/vX.Y/install\_gruepr.exe` copy exists only
because the Microsoft Store needs a stable, fetchable URL.

\---

## Ongoing maintenance

Almost everything runs itself. The few things that will need attention over time are
credentials that expire — most importantly the token that lets a release deploy the
website.

### The `WEBAPP\_DISPATCH\_TOKEN` (expires — plan to re-create it)

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
5. **Generate token** and copy the `github\_pat\_...` value (shown once).
6. In the **`gruepr`** repo → **Settings** → **Secrets and variables** → **Actions** →
open **`WEBAPP\_DISPATCH\_TOKEN`** → **Update secret**, paste the new value.

The token grants access to `gruepr-webapp`, but the secret lives in `gruepr` — that
crossing is the part that's easy to get backwards. If `gruepr` is an organization, its
"Allow access via fine-grained personal access tokens" setting must be on, or the
dispatch returns a 403 regardless of the token's permissions.

### Other expiring credentials

If a build (rather than the deploy) suddenly fails, suspect an expired signing
credential: the Apple Developer certificate used for the macOS notarization
(`APPLE\_CERTIFICATE` and friends) renews annually, and the SignPath token/certificate
behind the two Windows signing approvals can lapse too. These predate this pipeline and
live in the same repo secrets; renew them the same way you originally set them up.


