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
