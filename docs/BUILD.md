# Building Spoon Client

There are two ways to build Spoon Client:

1. **The GitHub Actions way (recommended)** — push the repository to
   GitHub and let the workflow do everything. You download the
   finished `.geode` artifact; no local tools needed.
2. **The local way** — install the Geode CLI + a C++23 compiler on
   your machine and build directly.

## Method 1: GitHub Actions (no local tools needed)

### 1.1 Push the project to GitHub

1. Download the project ZIP and unzip it locally (it'll be a folder
   called `spoon-client/`).
2. Create a new GitHub repository named `spoon-client` (or any name
   you prefer). Set it to Public so Actions has unlimited free
   minutes (Private repos get 2000 minutes/month, which is plenty
   but capped).
3. Push the contents of the `spoon-client/` folder to the new repo:

   ```bash
   cd spoon-client
   git init
   git add .
   git commit -m "Initial commit: Spoon Client v2.0.0"
   git branch -M main
   git remote add origin https://github.com/<your-username>/spoon-client.git
   git push -u origin main
   ```

   (Or use GitHub's web UI to upload the ZIP contents directly: open
   the repo → "Add file" → "Upload files" → drag the ZIP contents in.)

### 1.2 Trigger the build

The workflow runs automatically on every push to `main` and on every
pull request. To trigger it manually:

1. Open your repo on GitHub.
2. Click the **Actions** tab.
3. On the left, click **Build Spoon Client**.
4. Click **Run workflow** (top-right), choose the `main` branch, and
   click the green **Run workflow** button.

A new run starts immediately. Builds typically take 5–8 minutes.

### 1.3 Download the .geode artifact

1. Once the run shows a green ✓, click into it.
2. Scroll to the bottom of the run page — you'll see an **Artifacts**
   section.
3. Click the artifact named
   `SpoonClient-Geode-v5-GD-2.2081-<commit-sha>` to download it as
   a ZIP file.
4. Unzip the downloaded file — it contains the `SpoonClient.geode`
   package (plus the `.pdb` debug symbols if you enabled that).
5. Drop `SpoonClient.geode` into your Geometry Dash mods directory:
   - Windows: `%APPDATA%/GeometryDash/geode/mods/dev.spoonclient.mod/`
6. Launch Geometry Dash. Open the Geode log viewer
   (Window → Geode → Logs, or press F12). Confirm you see the line:
   ```
   [SpoonClient] v2.0.0 initialized. F6 menu key = 'F6', AG emergency stop = 'F8'.
   ```
7. Press F6 in any menu or during gameplay — the Spoon Client panel
   appears.

> **If the build fails:** see `docs/TROUBLESHOOTING.md` for the most
> common failures and how to fix them.

## Method 2: Local build (advanced)

### 2.1 Install the Geode CLI

Follow the [official Geode install
instructions](https://geode-sdk.org/install/). The CLI bundles the Geode
SDK, the GD binding headers, and a working C++ toolchain.

Verify the CLI is installed:

```powershell
geode --version    # should print v5.x
```

### 2.2 Bootstrap the SDK

From inside the project root:

```powershell
geode sdk install
# Or, if you already have an SDK somewhere:
$env:GEODE_SDK = "C:\path\to\geode-sdk"
```

### 2.3 Build

The Geode CLI wraps CMake:

```powershell
geode build
```

Or, if you prefer raw CMake (the v5 example-mod template supports both):

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The `.geode` package lands at `build/SpoonClient.geode` (CLI build) or
`build/Release/SpoonClient.geode` (raw CMake build).

## Updating to a new Geode release

When a new Geode SDK ships (e.g., v5.12.0), bump the pin in
`.github/workflows/build.yml`:

```yaml
with:
  sdk: 'v5.12.0'   # was v5.11.2
```

The mod.json uses `"geode": "$GEODE_VERSION"` so the .geode package
automatically declares compatibility with whatever SDK version the
workflow built against.

## Updating to a new GD version

When RobTop releases a new GD version (e.g., 2.2091):

1. Update `mod.json`'s `gd` block:
   ```json
   "gd": { "win": "2.2091", "android": "2.2091", "mac": "2.2091", "ios": "2.2091" }
   ```
2. Wait for the Geode SDK to ship a release that supports the new GD
   version (watch [GeodeSDK on X](https://x.com/GeodeSDK) or the
   [GitHub releases](https://github.com/geode-sdk/geode/releases)).
3. Bump `sdk:` in `.github/workflows/build.yml` to the new Geode
   version.
4. Push a commit and let the workflow run.

Until Geode itself supports the new GD version, the build will fail
because the binding headers don't match. There's nothing the mod
author can do about that — Geode's maintainers have to release an
SDK update first.
