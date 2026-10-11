# Enable Renovate

This repository is configured for the hosted [Renovate GitHub App](https://github.com/apps/renovate).
Renovate opens dependency-update pull requests; GitHub Actions builds and tests them.

## Activation checklist

- [ ] Commit and push `renovate.json`, `requirements.txt`, and this guide, then
  merge them into the repository's default branch (`master`). Renovate reads its
  configuration from the default branch.
- [ ] Install the [Renovate GitHub App](https://github.com/apps/renovate) for the
  account or organization owning this repository. Select this repository when
  granting access. If the app is already installed, add the repository to its
  installation's repository access list.
- [ ] If the organization requires approval for GitHub Apps, have an organization
  owner approve the installation and requested permissions, including access to
  update workflow files.
- [ ] Check for a Renovate onboarding PR (usually titled **Configure Renovate**).
  If one appears, review its dependency summary and merge it, preserving the
  custom managers in the existing `renovate.json`. A repository with configuration
  already on its default branch may be treated as already onboarded.
- [ ] Ensure GitHub Issues are enabled under **Settings > General > Features**
  so Renovate can create its **Dependency Dashboard** issue. Check the dashboard
  for detected dependencies, pending updates, and configuration or lookup errors.
- [ ] Confirm GitHub Actions is enabled for the repository. Once the first
  Renovate PR opens, verify that **PlatformIO Bluepill Environment**, **Linux Unit
  Testing**, and **Windows Unit Testing** run on it. These workflows already
  listen for `pull_request` events. The release workflow runs on release events,
  rather than on update PRs.
- [ ] Recommended: configure a ruleset under **Settings > Rules > Rulesets**, or
  branch protection under **Settings > Branches**, for `master`. Require a pull
  request, review, and the firmware and unit-test checks before merging. Select
  the actual check names shown by GitHub after they have run.

No scheduled Actions workflow, repository secret, personal access token, or
Actions "Allow GitHub Actions to create and approve pull requests" setting is
needed for the hosted app. The app uses its own installation credentials.

## Update policy

- **Schedule:** Mondays from 00:00 until 06:00, Europe/Berlin time. This is a
  permitted window for updates, not a guarantee that the hosted app runs at an
  exact time. Existing PR maintenance and security-related updates may happen
  outside this window.
- **Volume:** at most five concurrent update PRs. Renovate's recommended preset
  may apply additional rate limits or require dashboard approval for some updates.
- **Merging:** automatic merging is disabled; review and merge PRs manually.
- **GitHub Actions:** discovered automatically in `.github/workflows/`.
- **PlatformIO CLI:** discovered automatically in `requirements.txt`. It is
  pinned to `6.2.0`, the PyPI release current when this setup was added, so future
  version changes are reviewable PRs. The existing Python 3.9 workflows satisfy
  that version's declared Python requirement. Let CI verify the initial pin.
- **Firmware libraries:** the four Git URLs in `c6021light/platformio.ini` stay
  pinned to full commit SHAs. Renovate proposes the latest commit from each
  upstream repository's default branch (`HEAD`), including unreleased changes.
  To follow a maintenance branch instead, split the relevant custom manager into
  repository-specific rules and set `currentValueTemplate` to that branch.
- **STM32 platform:** Renovate reads version tags from
  `platformio/platform-ststm32` and updates `ststm32@...`. The firmware CI build
  also verifies that the proposed version is available in the PlatformIO registry.
- **GoogleTest:** Renovate updates the release tag in
  `c6021light/CMakeLists.txt.in`, restricted to versions **below 1.15.0**.
  v1.14.0 is the last release supporting C++14. When the project's C++ standard
  is upgraded, revisit the `allowedVersions` limit in `renovate.json`.

The configuration tracks direct dependencies declared in the files above.
PlatformIO's transitive frameworks/toolchains and pip's transitive packages are
not individually locked or updated by these rules. Vendored source code without
a tracked dependency declaration is also outside this setup.

## Verify the first run

1. Check the Dependency Dashboard or hosted Renovate logs for all four firmware
   library pins, the STM32 platform, GoogleTest, PlatformIO CLI, and Actions.
2. To test without waiting until Monday, use a dashboard option to create a PR
   immediately when offered, or temporarily remove `schedule` from the default
   branch configuration. Restore the schedule after verification.
3. Inspect the first PRs: firmware-library updates should replace SHAs;
   GoogleTest, platform, CLI, and tagged Actions updates should change version pins.
4. Wait for CI and review upstream changes before merging. Firmware-library and
   platform updates may also need the hardware checks in
   [`c6021light/test/TestManual.md`](c6021light/test/TestManual.md).

If no updates appear, check app repository access, onboarding status, the default
branch configuration, schedule, dashboard approvals, and Renovate's logs. A
dependency that is already current does not need a PR.

## Validate future configuration changes

With a current Node.js/npm installation, run from the repository root:

```sh
npx --yes --package renovate -- renovate-config-validator --strict renovate.json
```

Or use Docker (this version was used to validate the initial configuration):

```sh
docker run --rm -v "$PWD:/workspace:ro" \
  --entrypoint renovate-config-validator \
  ghcr.io/renovatebot/renovate:44.151.0 --strict /workspace/renovate.json
```

Validation checks configuration structure; the hosted app's first run verifies
live dependency extraction and upstream lookups.
