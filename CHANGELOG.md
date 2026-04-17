# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v1.0.5] - 2026-04-17

### Changed

- Updates and improves docker build process. No user facing changes, but a
  static hdf library is used instead of a build in docker version.

## [v1.0.4] - 2026-02-10

### Changed

- Adds new short name path to `trajectorysubsetter_varinfo_config.json` from
  the latest version of ATL24.
- Throws an exception when no short name can be found during local
  development (not a functional change).
- Copies .snyk alongside each of the tested requirements files.

## [v1.0.3] - 2025-12-15

### Changed

- Updates internal python dependencies to mitigate urllib3 vulnerabilities CVE-2025-66471 and CVE-2025-66418.

## [v1.0.2] - 2025-11-20

### Changed

- Add ruff and clang formatting to `pre-commit-config.yaml` to add to the
  Python formatting and also standardizes the C++ code.
- Apply the new pre-commit formatting across the entire repository, having
  no functional change to any affected files.

## [v1.0.1] - 2025-10-29

### Changed

- The `pre-commit-config.yaml` has been applied across the entire repository.
  This primarily white space fixing, and has no functional change to any
  affected files.

## [v1.0.0] - 2025-10-29

This version of the Harmony Trajectory Subsetter service contains all
functionality previously released internally to EOSDIS as
`sds/trajectory-subsetter:0.4.0`. Minor reformatting of the repository
structure has occurred to better comply with recommended best practices for a
Harmony backend service repository, but the service itself is fundamentally
unchanged.

For more information on internal releases prior to NASA open-source migration,
see `legacy-CHANGELOG.md`.

### Added

- LICENSE file as required by NASA Open Source Software guidelines.
- CODEOWNERS file to ensure default reviewers for pull requests.
- GitHub workflows for running tests and publishing Docker images to GHCR.

### Changed

- Dockerfiles and scripts in the `bin` directory have been updated to make use
  of new GHCR image names.

### Removed

- On-premises scripts and artefacts for the SDPS system have been removed from
  the repository.

[v1.0.5]: https://github.com/nasa/harmony-trajectory-subsetter/releases/tag/1.0.5
[v1.0.4]: https://github.com/nasa/harmony-trajectory-subsetter/releases/tag/1.0.4
[v1.0.3]: https://github.com/nasa/harmony-trajectory-subsetter/releases/tag/1.0.3
[v1.0.2]: https://github.com/nasa/harmony-trajectory-subsetter/releases/tag/1.0.2
[v1.0.1]: https://github.com/nasa/harmony-trajectory-subsetter/releases/tag/1.0.1
[v1.0.0]: https://github.com/nasa/harmony-trajectory-subsetter/releases/tag/1.0.0
