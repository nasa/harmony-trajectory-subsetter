# Segmented Trajectory Subsetter.

This repository contains the source code and configuration scripts for the
Data Services segmented trajectory subsetter.

### Repository branching:

This repository contains two long-lived branches: `master` and `dev`. The
`master` branch will contain only code identified for an SDPS release. The
`dev` branch contains the most recent code deployed to Harmony (Sandbox, SIT,
and likely UAT). It also is equivalent to the SDPS `relb` ClearCase view. All
feature work should be added via pull request (PR) with a target branch of
`dev`.

### Repository contents:

This repository contains a number of subdirectories:
( See https://wiki.earthdata.nasa.gov/x/CwdoDQ )

* `bin` - A directory containing scripts used to build docker images and tests.
* `subsetter` - A directory containing  C++ code to build subset binary file (DAS-1252).
* `docker` - A directory containing Dockerfiles for creation images for Harmony service and test.
* `harmony_service` - A directory containing the Python code to invoke the
  Trajectory Subsetter within a Harmony service.
* `tests` - A directory containing Python unit tests for the Harmony service functionality.

Note also: `VERSION` - A file containing the version for the most recent SDPS
release. This file is only iterated shortly before merging to the `master`
branch, when a tarball will be placed in Nexus (a.k.a. Maven) ready for that
release.

### Harmony service conda environment:

The Harmony service for the trajectory subsetter runs within a conda
environment, itself within a Docker container. To recreate the same conda
environment in a local terminal, use the following commands:

```bash
conda create --name trajectory python=3.9 --channel conda-forge --channel defaults
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

There are two versions associated with the Trajectory Subsetter: the SDPS
release version and the Harmony service Docker image version.

### SDPS versioning:

The SDPS release version is managed via the `VERSION` file in the root
directory of this repository. Bamboo reads this file during the build plan,
and uses the contents in the deployment project when a release is deployed to
Nexus. This occurs for major SDPS releases and for hotshelves.

The SDPS releases version should only be iterated when a merging from the `dev`
branch to the `master` branch, at a time when it has been identified that a
static version of the Trajectory Subsetter is required. The process should be
as follows:

* Create a release branch from `dev` using `git checkout -b <release_branch_name>`.
* Update the contents of the `VERSION` file to be the name of the new release,
  either an SDPS release (e.g., `212_UPDATES`) or a hotshelf (e.g., `HOTSHELF-DAS-XXX`).
* Commit those changes, and create a pull request against the `master` branch.
* Once the pull request is merged, ensure an appropriately named artefact has
  been placed in the [Trajectory Subsetter part of Nexus](https://maven.earthdata.nasa.gov/#browse/browse:trajectorysubsetter).
* Merge back from `master` into `dev`, to ensure the `dev` branch is up to date.

### Harmony versioning:

Harmony releases occur every time changes are made to the code and merged into
the `dev` branch. The versions use semantic version numbers
(`major.minor.patch`). This version is included in the
`docker/service_version.txt` file. When updating the Trajectory Subsetter, the
version number contained in that file should be incremented before creating a
pull request.

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
conda env create -f ../environment.yaml && conda activate trajectory-subsetter-local-dev
```

and create a `makeit_local_conda` file here. Remember to replace the file path to h5c++. Note that the first build may take longer.

```
#!/bin/bash

if [ ! -f "./hdfeos5/include/HE5_GctpFunc.h" ] || [ ! -f "./hdfeos5/gctp/src/libGctp.la" ]; then
    echo "The required header and library files do not exit - building  hdfeos5 library..."
    git clone https://git.earthdata.nasa.gov/scm/sitc/hdfeos5.git && cd hdfeos5
    ./configure && make && make install
    cd ../
fi

/Path/to/conda/env/trajectory-subsetter-local-dev/bin/h5c++ -v -std=c++20 -g ./Subset.cpp \
-DSDPS \
-DH5_USE_18_API \
-Og \
-lgeotiff -ltiff -ljpeg -lGctp -llzma \
-lboost_program_options -lboost_filesystem \
-lboost_date_time       -lboost_regex \
-o subset
```

Build the source code (you may have to make it executable via `chmod 755 makeit_local_conda`):
```
./makeit_local_conda
```

A debug environment can be configured in Visual Studio Code using the `launch.json` file in the top-level `.vscode` directory. To create this file, go to "Run and Debug" in the left panel, click "create a launch.json file", and add the following configurations, including only relevant `args`. Remember to replace file paths as needed.
```
    "configurations": [
        {
            "name": "<Name of debug environment>",
            "type": "cppdbg",
            "request": "launch",
            "program": "/Path/to/subset",
            "args": [
              "--configfile","../harmony_service/subsetter_config.json",
              "--filename","/Path/to/input/file",
              "--outfile","/Path/to/output/file",
              "--includedataset","/Path/to/variable1,/Path/to/variable2",
              "--bbox","W,S,E,N",
              "--start","YYYY-MM-DDTHH:MM:SS",
              "--end","YYYY-MM-DDTHH:MM:SS"
              ],
            "cwd": "/Path/to/subset/directory",
            "environment": [],
            "MIMode": "lldb",
        }
    ]
```
**Temporal subsetting issue**: VScode mysteriously converts the `--start` and `--stop` arguments to `YYYY/MM/DD HH:MM:SS`. To work around this, note the addition of single quotes within the time arguments in `launch.json`, and go to `Subset.cpp` and add these two lines of code in the temporal argument processing section. <br>
**BEWARE**: You must remember to delete these two lines before committing.
```
startString = variables_map["start"].as<std::string>();
endString = variables_map["end"].as<std::string>();
boost::erase_all(startString, "'");     <----- New code
boost::erase_all(endString, "'");       <----- New code
boost::regex date_format(...
```

Now navigate to any source file to place a breakpoint, and hit "Start debugging" in the Debug Console. Refer to the [Visual Studio Code Debugging](https://code.visualstudio.com/docs/editor/debugging) documentation for how to further use the VSCode debugger. <br><br>
Note 1: A pop-up window may appear called "Developer Tools Access" that requires elevated privileges.<br>
Note 2: Additional required variables are not yet included as `earthdata-varinfo` is not linked.

### Best Coding Practices:

On-going development incorporates updates to new and existing code that will better adhere the Trajectory Subsetter source code to modern C++ best practices. The core guidelines are extracted from the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c-core-guidelines), written in part by Bjarne Stroustrup, the creator of the C++ programming language.

The code is not expected to completely and entirely adhere to these guidelines, as implementing every guideline is not time efficient. There is a general rule for maintaining consistency within the existing code as long as it is safe, efficicient, not error prone, or particularly convoluted.