# Harmony Segmented Trajectory Subsetter.

This repository contains the source code and configuration scripts for the
Harmony Segmented Trajectory Subsetter.

### Repository contents:

This repository contains a number of subdirectories:
( See https://wiki.earthdata.nasa.gov/x/CwdoDQ )

* `bin` - A directory containing scripts used to build docker images and tests.
* `subsetter` - A directory containing  C++ code to build subset binary file (DAS-1252).
* `docker` - A directory containing Dockerfiles for creation images for Harmony service and test.
* `harmony_service` - A directory containing the Python code to invoke the
  Trajectory Subsetter within a Harmony service.
* `tests` - A directory containing unit tests for the Python Harmony service functionality and C++ source code.

### Harmony service conda environment:

The Harmony service for the trajectory subsetter runs within a conda
environment, itself within a Docker container. To recreate the same conda
environment in a local terminal, use the following commands:

```bash
conda create --name trajectory python=3.11 --channel conda-forge \
    --channel nodefaults --override-channels -y
conda activate trajectory
pip install -r ./harmony_service/pip_requirements.txt
```

### Configuration file:

`harmony_service/subsetter_config.json` is a configuration file whose path is
passed to the Trajectory Subsetter binary at runtime. This configuration file
contains supplementary information necessary to successfully subset some
collections. The configuration file is split into several sections described
below:

* `ShortNamePath` - A list of metadata locations within a granule that might
  contain a reference to the collection short name.
* `Missions` - A dictionary mapping collection short names to mission names.
  The keys are regular expression patterns designed to match multiple
  collection short names.
* `CoordinateDatasetNames` - A dictionary containing known coordinate datasets.
  The keys of this dictionary are regular expression patterns that match
  collection short names, such as "ATL\d{2}", which would match any of the
  ICESat-2/ATLAS products (e.g., "ATL03"). The values are a further dictionary
  that specifies known coordinates split into types of: time, latitude and
  longitude.
* `SuperGroups` - A dictionary mapping hierarchical groups to others that
  contain coordinates relevant to them.
* `SuperGroupCoordinates` - A dictionary denoting the coordinate variables
  within a super set group.
* `ProductEpochs` - A dictionary containing supplementary information regarding
  the epoch of a given data product. The keys of the dictionary are regular
  expression patterns designed to match collection short names. The values are
  ISO-8601 timestamps.
* `SubsettableGroups` - A dictionary specifying groups for a collection that
  contain variables that can be subsetted. The keys of the dictionary are
  regular expression patterns designed to match collection short names. If
  a collection does not match an entry in this dictionary, all groups within
  the collection (that aren't in `UnsubsettableGroups`) can be subsetted.
* `UnsubsettableGroups` - A dictionary specifying hierarchical groups within a
  granule that cannot be subsetted. The keys are regular expression patterns
  that match collection short names. Each value is a list of variable paths
  that should not be subsetted. These paths are also regular expression
  patterns.
* `PhotonSegmentGroups` - Groups that require special handling.
* `FileSizeLimit` - Output file size limit.
* `RequiredDatasets` - A dictionary denoting required variables for different
  collections. A regular expression pattern specifies the variables to which
  the required variables are applicable.
* `RequiredDatasetByFormat` - A dictionary that specifies variables that are
  required for specific output file formats. A regular expression pattern
  ensures that variables will only be included if they are prepended with a
  specific string.
* `Resolutions` - A dictionary denoting pixel resolutions to be used for
  GeoTIFF output.
* `Projections` - A dictionary mapping the prefix of some variable paths to a
  string representing the projection (e.g., "NLAEA", "SLAEA" or "CEA").

### Versioning:

Harmony releases occur every time changes are made to the service version as
listed in `docker/service_version.txt`, and those changes are merged into the
`main` branch. The versions use semantic version numbers (`major.minor.patch`).
When updating the Trajectory Subsetter, the version number contained in that
file should be incremented before creating a pull request.

The general rules for which version number to increment are:

* Major: When API changes are made to the service that are not backwards
  compatible.
* Minor: When functionality is added in a backwards compatible way.
* Patch: Used for backwards compatible bug fixes or performance improvements.

When the Docker image is built, it will be tagged with the semantic version
number as stored in `docker/service_version.txt`.

### Local development:

A local Conda development environment can be configured using the following Conda environment and a customized `makeit` file. In the `./subsetter` directory:
```
conda env create -f ../environment.yaml
```
and activate the environment:
```
conda activate trajectory-subsetter-local-dev
```

Build the source code:
```
./makeit_local_conda
```

 * Note: If the error "_fatal: destination path 'hdfeos5' already exists..._" is returned, it's likely the library was not fully built. Remove the library altogether and re-build.

A debug environment for both Intel and ARM64 architectures can be
configured in Visual Studio Code by:
1. Installing the `CodeLLDB` VS Code
extension
2. Using the `launch.json` file in the top-level `.vscode`
directory. To create this file, go to "Run and Debug" in the left panel,
click "create a launch.json file", and add the following configurations,
including only relevant `args` (`includedataset`, `bbox`, `start/end` are
not required). Remember to replace file paths as needed.
```
    "configurations": [
        {
            "name": "<Name of debug environment>",
            "type": "lldb",
            "request": "launch",
            "program": "/Path/to/subset",
            "args": [
              "--configfile","../harmony_service/subsetter_config.json",
              "--filename","/Path/to/input/file",
              "--outfile","/Path/to/output/file",
              "--includedataset","/Path/to/variable1,/Path/to/variable2",
              "--bbox","W,S,E,N",
              "--shortname","ATLxx",
              "--loglevel","INFO",
              "--logfile","/Path/to/log_output/file",
              "--start","'YYYY-MM-DDTHH:MM:SS'",
              "--end","'YYYY-MM-DDTHH:MM:SS'"
              ],
            "cwd": "/Path/to/subset/directory",
            "environment": [],
            "MIMode": "lldb",
        }
    ]
```
**Temporal subsetting issue**: VScode mysteriously converts the `--start` and
`--end` arguments to `YYYY/MM/DD HH:MM:SS`. To work around this, note the
addition of single quotes within the time arguments in `launch.json`.

Now navigate to any source file to place a breakpoint, and hit "Start debugging"
in the Debug Console. Refer to the [Visual Studio Code Debugging](https://code.visualstudio.com/docs/editor/debugging)
documentation for how to further use the VSCode debugger. <br><br>

Note 1: A pop-up window may appear called "Developer Tools Access" that
requires elevated privileges.<br>
Note 2: Additional required variables are not yet included as `earthdata-varinfo`
is not linked.

### GoogleTest:

GoogleTest is the unit test framework used to test the C++ source code. To build and run your test:

```
cd tests/unit/gtest
cmake -S . -B build
cmake --build build
```

To rebuild and rerun your test:
```
cmake --build build
```

### Best Coding Practices:

On-going development incorporates updates to new and existing code that will better adhere the Trajectory Subsetter source code to modern C++ best practices. The core guidelines are extracted from the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c-core-guidelines), written in part by Bjarne Stroustrup, the creator of the C++ programming language.

The code is not expected to completely and entirely adhere to these guidelines, as implementing every guideline is not time efficient. There is a general rule for maintaining consistency within the existing code as long as it is safe, efficient, not error prone, or particularly convoluted.

### pre-commit hooks:

This repository uses [pre-commit](https://pre-commit.com/) to enable pre-commit
checking the repository for some coding standard best practices. These include:

* Removing trailing whitespaces.
* Removing blank lines at the end of a file.
* JSON files have valid formats.
  formatting checks.

To enable these checks:

```bash
# Install pre-commit Python package:
pip install -r pre-commit

# Install the git hook scripts:
pre-commit install

# (Optional) Run against all files:
pre-commit run --all-files
```

When you try to make a new commit locally, `pre-commit` will automatically run.
If any of the hooks detect non-compliance (e.g., trailing whitespace), that
hook will state it failed, and also try to fix the issue. You will need to
review and `git add` the changes before you can make a commit.

It is planned to implement additional hooks, possibly including tools such as
`mypy`.

[pre-commit.ci](pre-commit.ci) is configured such that these same hooks will be
automatically run for every pull request.

## Get in touch:

You can reach out to the maintainers of this repository via email:

* david.p.auty@nasa.gov
* josie.lyon@nasa.gov
