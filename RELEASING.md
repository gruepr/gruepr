# Releasing a new version of gruepr

## Quick checklist

1. Bump `gruepr_version` (and `copyright_year` if needed) in `gruepr.pro`. Fill in
`// DONE:` in `main.cpp`. Commit + push to `master`.
2. Actions → **Build** → Run workflow. Approve signing twice in SignPath when prompted.
3. Review the draft release under **Releases** (tag, 3 assets, notes).
4. Actions → **Release** → Run workflow.
5. Ctrl+F5 gruepr.com once `deploy.yaml` and the Pages run are both green.
6. Microsoft Store: Partner Center → new submission → update Package URL → Submit.
7. Mac App Store: App Store Connect → new version (if needed) → select the uploaded
build → paste release notes into What's New → Submit for Review.

Full detail for each step, plus what to do when something breaks, is below (Parts
A-F, then Ongoing maintenance).

---

## Background

The release runs as **two workflows**:

* **Build** (`Build.yaml`, you run it) — builds the three downloadable executables,
uploads the macOS build to App Store Connect, and creates a **draft** release whose
notes are generated from your `// DONE:` items. Nothing is committed.
* **Release** (`Release.yaml`, you run it after reviewing the draft) — running it is
your approval. It publishes the reviewed draft, writes the notes into
`CHANGELOG.md`, clears the `// DONE:` block in `main.cpp`, commits both to `master`,
and triggers the gruepr.com deploy.

A few facts the process relies on:

* **The version and copyright year have a single source of truth:** lines 7-8 of
`gruepr.pro` (`gruepr_version = 13.1`). It flows into `GRUEPR_VERSION_NUMBER`, the
`VERSION` embedded in `gruepr.exe`, and the app's macOS bundle version — the NSIS
installer and Mac App Store upload both read it back out automatically. A two-part
value is fine — the automation normalizes it to a three-part tag (`v13.1.0`).
* **Release notes come from your `// DONE:` notes in `main.cpp`** — you don't
hand-write `CHANGELOG.md`. Build turns DONE into the draft body; Release writes the
published notes into `CHANGELOG.md` and clears DONE. The same text also gets reused
by hand for the Mac App Store's "What's New" field (Part F).
* **The Release workflow is the point of no return.** It publishes the GitHub release
(which becomes `releases/latest`, driving the in-app upgrade check in
`startDialog.cpp` and the `releases/latest/download/...` buttons on gruepr.com) and
is the only thing that commits anything. Nothing is published or committed until you
run it.

---

## Part A — Code, version, and DONE notes (main `gruepr` repo)

1. **Make and test the code changes** for this release.
2. **Bump the version and check the copyright year.** Edit lines 7-8 of `gruepr.pro`:

   ```
   gruepr_version = 13.2
   ```

   Two-part (`13.2`) or three-part (`13.2.0`) both work; the tag comes out three-part
   either way. If the calendar year has rolled over, update `copyright_year`.

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

---

## Part B — Build and draft the release (Workflow 1)

5. In the `gruepr` repo: **Actions** tab → the combined **Build** workflow
(`Build.yaml`) → **Run workflow** (on `master`). This is the only workflow used for
releasing — one run builds Windows, macOS (DMG), macOS (App Store), and Linux. The
per-platform `Build_Windows.yaml`, `Build_macOS.yaml`, `Build_macOS_AppStore.yaml`,
and `Build_linux.yaml` workflows are for testing individual platforms and play no
part in releasing.
6. **Wait for the build — and approve the code signing twice.** Five jobs run:
`build-windows`, `build-macos`, `build-macos-appstore`, `build-linux`, and
`draft-release`. **The `build-windows` job pauses twice for signing:** it submits
`gruepr.exe`, then the Windows installer, to SignPath under the `release-signing`
policy, and waits for you each time. **Log into the SignPath web console and approve
both requests** as they appear — the build will not finish otherwise. macOS DMG
signing/notarization and the Mac App Store build/sign/upload are all automatic —
nothing to approve there.
7. `draft-release` only waits on `build-windows`, `build-macos`, and `build-linux` — a
`build-macos-appstore` failure doesn't block the GitHub release. If it fails, you can
just re-run that one job once you've diagnosed it; it doesn't need a full rebuild.

---

## Part C — Review the draft, then run Release (Workflow 2)

8. **Review the draft.** Open it under **Releases** (or from the Build run summary)
and check:

   * tag is `vX.Y.Z`,
   * the title reads `vX.Y.Z`,
   * all three assets are attached: `install_gruepr.exe`, `gruepr.dmg`,
   `gruepr.AppImage`,
   * the notes read well — **edit them right here** if any `// DONE:` wording was too
   terse. Whatever the draft says when you run Release is exactly what gets published
   and written to `CHANGELOG.md`.
9. **Run the Release workflow.** Actions tab → **Release** → **Run workflow**. Running
it is your approval. It publishes the draft (which becomes `releases/latest`,
lighting up the in-app upgrade notice and the gruepr.com download buttons), prepends
`## vX.Y.Z — <today>` plus the published notes to `CHANGELOG.md`, clears the
`// DONE:` block in `main.cpp`, commits both to `master`, and triggers the gruepr.com
deploy (Part D).
10. Confirm the Release run succeeded (Actions tab) and that the new commit landed on
`master`.

---

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
* `https://www.gruepr.com/downloads/vX.Y/install_gruepr.exe` downloads.

If the deploy *doesn't fire at all*, that's the `WEBAPP_DISPATCH_TOKEN` — see Ongoing
maintenance. (The deploy workflow itself already handles the two things that can trip a
CRA deploy in CI — treating build warnings as non-fatal, and authenticating the
`gh-pages` push — so those shouldn't recur.)

**If you ever need to do it by hand** (the secret isn't set, or a deploy failed), the
manual steps are below (conventions: edit in `src/`, GitHub Desktop for git, PowerShell
for `npm`, never touch `gh-pages` directly, `npm install` once per fresh checkout):

11. Pull latest `main` in GitHub Desktop; `npm install` if it's a fresh checkout.
12. Edit `content.js`: bump the displayed version and release date. Download buttons
that use `releases/latest/download/...` need no change. Only touch a download URL
if it is version-stamped.
13. Copy `install_gruepr.exe` (from the published release) into a new folder
`public/downloads/vX.Y/install_gruepr.exe`. This folder uses the version string
exactly as it appears in `gruepr.pro` (e.g. `v13.0.2`) — only the Git tag and
changelog headings get normalized to three parts.
14. The changelog page renders straight from `CHANGELOG.md` (via react-markdown in
`ChangeLog.js`), so there's nothing to edit for the changelog itself.
15. Preview with `npm start`; confirm the changelog shows `vX.Y.Z` and links resolve.
16. Commit + push to `main`, run `npm run deploy`, then verify on gruepr.com with
**Ctrl+F5**. Test the hosted installer URL directly:
`https://www.gruepr.com/downloads/vX.Y/install_gruepr.exe`.

---

## Part E — Microsoft Store (Partner Center)

Manual, every release. gruepr is on the Store as an **EXE app** that points at a
**versioned installer URL**, and that URL changes each version — Partner Center
requires a fixed, versioned URL and rejects a `latest`-style redirect. The app already
exists in Partner Center, so each release is an **update submission**, and in practice
**the only field that changes is the Package URL** — the rest carry over from the last
submission.

17. Sign in to Partner Center → **Apps and games** → **gruepr** (Store ID
`xpdc98f0ts7gcs`). Start a new submission (the **Update** / "Create submission"
button on the app overview), open the **Packages** section, and open the existing
package to edit it.
18. Set the package details. The values gruepr uses:

   |Field|Value|Change each release?|
   |-|-|-|
   |**Package URL**|`https://www.gruepr.com/downloads/vX.Y/install_gruepr.exe`|**Yes** — point at the new version's folder|
   |**Architecture**|x64|No|
   |**Languages**|English (`en`)|No|
   |**App type**|EXE|No|
   |**Installer parameters**|`/S` (NSIS's silent-install switch — capital S)|No|
   |**Installer handling** (return codes)|Optional — leave as set|No|

   The installer at that URL is already hosted for you: Part D's deploy publishes it to
`public/downloads/vX.Y/`. Use the **exact** folder name the deploy created (it's your
`gruepr.pro` version string, e.g. `v13.0.2`) — the simplest way to be sure is to open
`https://www.gruepr.com/downloads/vX.Y/install_gruepr.exe` in a browser and confirm it
downloads, then paste that URL.

19. Save the package, then **Submit** for certification. It takes a few business days;
once it passes, the Store pushes the update to Store users automatically. (The GitHub
release and the gruepr.com direct-download buttons are already live from the earlier
parts — this governs only the Microsoft Store copy.)

Two things not to trip on: the binary at a submitted URL must **never change** after
submission, which is automatically satisfied because every release gets a fresh `vX.Y/`
folder — so never overwrite an old version's installer. And `/S` is what lets the Store
install silently (a UAC prompt is still allowed); if a future installer change ever
breaks silent install, certification will fail there.

---

## Part F — Mac App Store (App Store Connect)

Manual, every release — Apple's review step can't be automated away, so this mirrors
Part E's shape. All account-side setup (certificates, provisioning profile, the app
record, the API key) is already done; the `build-macos-appstore` job in Part B already
built, signed, and uploaded the new build to App Store Connect for you. What's left is
purely content and approval:

20. Sign in to **App Store Connect** → **My Apps** → **gruepr**. If there's no version
entry in **Prepare for Submission** for this release yet, create one (**+ Version**).
21. Open that version and select the build Part B just uploaded, under **Build** — it
can take a few minutes after upload to finish processing before it's selectable.
22. Paste your release notes into **What's New in This Version** — reuse the same
`// DONE:`-derived text from the GitHub draft (Part C) rather than rewriting it.
23. Click **Save**, then **Add for Review** / **Submit for Review**. Apple's review
typically takes 1-2 days. Watch for a Resolution Center message if it's rejected —
the most common one so far has been the automated entitlement scanner flagging
`com.apple.security.network.server` (used for the OAuth loopback redirect handler in
`LMS/LMS.h`); replying with a technical explanation of that listener (loopback-only,
single-use, closes after catching the redirect) and updating the App Review Notes
field with the same text has been sufficient to clear it without a new build.

---

## Quick reference

**Where the version and copyright year is set:** `gruepr.pro` line 7-8.

**Where release notes come from:** the `// DONE:` block in `main.cpp`. They flow into
the draft, then into `CHANGELOG.md` when you run Release, and get copy-pasted into the
Mac App Store's "What's New" field (Part F).

**Version forms:** the Git tag and `CHANGELOG.md` headings use three parts (`v13.2.0`)
— the automation normalizes a two-part value to three. The gruepr.com installer folder
under `public/downloads/` and the version text on the site use the string exactly as
you typed it in `gruepr.pro` (so `13.0.2` → folder `v13.0.2`, but `13.2` → folder
`v13.2`). The Microsoft Store Package URL points at that same folder.

**Release asset names (must stay stable so `releases/latest/download/<name>` keeps
working):**

|Platform|CI artifact name|File in artifact|Published as|
|-|-|-|-|
|Windows|`gruepr-signed-windows-installer`|`install_gruepr.exe`|`install_gruepr.exe`|
|macOS|`gruepr-macos-universal`|`Install gruepr.dmg`|`gruepr.dmg`|
|Linux|`gruepr-linux-appimage`|`gruepr-linux-x86_64.AppImage`|`gruepr.AppImage`|

**Two Windows copies, on purpose:** the GitHub release asset
(`releases/latest/download/install_gruepr.exe`) serves direct downloads and the
in-app check; the gruepr.com `/downloads/vX.Y/install_gruepr.exe` copy exists only
because the Microsoft Store needs a stable, fetchable URL.

**The Mac App Store build produces no downloadable asset** — `build-macos-appstore`
uploads straight to Apple (artifact `gruepr-macos-appstore-pkg` is kept only as a
diagnostic copy) and isn't part of `draft-release`'s inputs, so it can fail or be
re-run independently of the other three platforms.

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
credential:

* The Developer ID certificate used for DMG notarization (`APPLE_CERTIFICATE` and
friends) renews annually.
* The Apple Distribution and Mac Installer Distribution certificates, and the Mac App
Store provisioning profile, used by `build-macos-appstore`
(`APPLE_DISTRIBUTION_CERTIFICATE`/`_PASSWORD`, `APPLE_INSTALLER_CERTIFICATE`/`_PASSWORD`,
`APPLE_PROVISIONING_PROFILE`) also expire and need periodic renewal in the Apple
Developer portal.
* The SignPath token/certificate behind the two Windows signing approvals can lapse
too.

These predate this pipeline and live in the same repo secrets; renew them the same way
you originally set them up.
