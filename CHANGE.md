[version 0.4.0] 2025-09-08

- **DAS-2325**
 - Add functionality that writes a history metadata attribute containing
   request date and time, service name, service version, and subset
   parameters in each file output to maintain file provenance.

[version 0.3.8] 2025-08-21

- **DAS-2405**
 - Update -boundingshapes parsing to verify .geojson is a valid file before
   attempting to open it.

[version 0.3.7] 2025-08-14

- **DAS-2390**
 - Update subsetter_config.json to support the new lat and lon variables in ATL07 v7
 - Update the build script to suppress Variable Length Arrays (VLAs) (-Wno-vla-extension)
 - Resolve various warning messages.

[version 0.3.6] 2025-08-05

- **DAS-2402**
 - Makes version detection more robust by allowing any ground track to be
   tested to guess the version of ATL10 data.

[version 0.3.5] 2025-07-31

- **DAS-2398**
 - Add support to pass the logging level from Harmony using the --loglevel argument
 - Add support to pass the log file from command line using the --logfile argument
 - Update std::cout with log level API LOG_DEBUG(), LOG_INFO(), LOG_WARNING()
   LOG_ERROR(), and LOG_CRITICAL()
 - Add a Google Unit Test (test_LogLevel.cpp) to verify code.

[version 0.3.4] 2025-07-17

- **DAS-2329**
 - Add shortName to harmony adapter and command line arguments for collection
   that do not contain a shortName variable

[version 0.3.3] 2025-07-08

- **DAS-2376**
 - Add exception handling to the boost::posix_time API to manage cases
   where the value is 0.
 - Add make_local_conda script for building TS, replacing the need
   to copy instructions from README.md
 - Add a Google Unit Test (test_Temporal.cpp) to verify exception handling.

[version 0.3.2] 2025-06-25

- **DAS-2364**
 - Add SMAP L2 EASE grid row and column datasets as `ancillary_variables` in
   the `earthdata-varinfo` configuration file to ensure that only the EASE
   variables associated with the requested variables are included.
 - Explicitly remove the `defaults` channel from the Miniconda installation in
   service.Dockerfile, and add the `nodefaults` channel to the environment
   creation to avoid an Anaconda rate limit error.

[version 0.3.1] 2025-03-27

- **DAS-2233**
 - Fix bug that caused ATL03, ATL08, and ATL10 subsets to write out random
   values to the index begin dataset when a subset has only temporal
   constraints.

[version 0.3.0] 2025-03-24

- **DAS-2327**
 - Raise harmony-defined 'NoDataException' when the service finds no
   data to process (e.g., no data found by the service in the subset
   region)

[version 0.2.5] 2025-02-21

- **DAS-2310**
 - Correct how requested groups with no spatial coordinates in subsets
   with only spatial constraints are determined and added for special
   temporal subsetting.

[version 0.2.4] 2025-01-22

- **DAS-2289**
 - Update the missing datasets in the Applicability attributes
   of trajectorysubsetter_varinfo_config.json to support
   earthdata-varinfo==3.0.1

[version 0.2.3] 2025-01-02

- **DAS-2273**
 - Working on the CI/CD team for the Trajectory Subsetter build, we
   are still experiencing intermittent failures, though they have
   decreased in frequency. To further reduce these failures, the
   following changes were implemented.
 - Add "--no-cache" to tests.Dockerfile
 - Update to "rockylinux:8" per suggestion from CI/CD
 - Remove "channel default" per DAS-2237

- **DAS-2245**
 - Update configuration files (trajectorysubsetter_varinfo_config.json,
   subsetter_config) to reflect ATL10 v006's current architecture so
   that ATL10 subset. Update earthdata-varinfo==3.0.0

[version 0.2.2] 2024-11-12

- **DAS-2272**
 - Remove beam_refsur_ndx variable from ATL10 configuration since it no
   longer exists in the ATL10 version 6.

- **DAS-2247**
 - Add support for ATL10 v006 configuration and functionality while
   maintaining backward compatibility with ATL10 v005. Include unit
   tests for both ATL10 v005 and v006.

[version 0.2.1] 2024-10-29

- **DAS-2268**
 - Fix bug that caused incorrect subsetting when the subset included the end
   of the index begin dataset.

[version 0.2.0] 2024-10-04

- **DAS-2205**
 - Fix bug that caused trajectory values to be cut off the end of each
   trajectory segment, and add a unit test framework for the C++ source code.

[version 0.1.22] 2024-10-07

- **DAS-None**
 - Add flag `--platform linux/amd64` to the Docker build files so Docker
   binaries can be built on ARM64 machines.

[version 0.1.21] 2024-07-03

- **DAS-2194**
 - Fixed bug that would try to access the index selection size for a dataset
   when there is no index selection. This occur in two places, both regarding
   the calculation of the total time range.

[version 0.1.20] 2024-06-19

- **DAS-2175**
 - Update the earthdata-varinfo configuration file to add spatial coordinates
   whenever variables with only temporal coordinates are requested, namely in
   the groups `bckgrd_atlas` and `signal_find_output` in the ATL03 collection.

[version 0.1.19] 2024-06-13

- **DAS-2151**
 - Add ability to subset datasets that only have temporal coordinates
   in requests with only spatial constraints. This includes adding
   latitude, longitude, and time coordinate getters to Configuration.h,
   and adding a second Temporal constructor that takes seconds instead
   of datetime.

[version 0.1.18] 2024-06-13

- **DAS-None**
 - Update internal library versions for the Harmony Docker images.

[version 0.1.17] 2024-06-10

- **DAS-2161**
 - Internal change to how some parameters are passed to the trajectory subsetter
   binary. No user facing changes.

[version 0.1.16] 2024-06-11

- **DAS-2149**
  - Fixed issue introduced in original change by ensuring that the time
    range is only calculated when we have spatial or temporal constraints.

[version 0.1.15] 2024-05-24

- **DAS-2149**
  - Add functionality to track the largest time range of all the subset time
    coordinates in a request. This change also includes removing commented
    out code across the binary source code and improves comments in
    `Subsetter.h`.

[version 0.1.14] 2024-05-20

- **DAS-2173**
  - Update harmony-service-library-py to ~1.0.27.

[version 0.1.13] 2023-08-17

- **DAS-1945**
  - Update Python dependencies to use earthdata-varinfo instead of sds-varinfo.

[version 0.1.12] 2023-08-03

- **DAS-1931**
  - Bug fix: Added the required format output argument to an instantiation of the
    Subsetter class.


[version 0.1.11] 2023-07-20

- **DAS-1887**
  - Bug fix: Updated ForwardRefCoordinates.h to use forward and backward scanning
    methods to ensure that the fill values in the index begin dataset are skipped.

[version 0.1.10] 2023-06-19

- **DAS-1731**
  - Bug fix: Updated FwdRefBeginDataset.h to correctly calculate index begin
    datasets for ATL03.

[version 0.1.9] 2023-04-17

- **DAS-1806**
  - Refactoring: Removed global namespaces and updated Subset.cpp and
    Subsetter.h to incorporate modern coding standards.

[version 0.1.8] 2023-03-22

- **DAS-1773**
  - Removed metadata dataset assumption/requirements.

[version 0.1.7] 2023-03-13

- **DAS-1772**
  - Update to extract the collection short name from the input Source object.

[version 0.1.6] 2022-12-19

- **DAS-1726**
  - Convert the service to use `sds-varinfo==3.0.0`, which uses a JSON format
    configuration file instead of a YAML format. Note, the underlying schema
	remains the same.

[version 0.1.5] 2022-11-16

- **DAS-1701**
  - Ensures that if variables in a Harmony request do not include a leading
    slash, one will be added before determining required variables using
    `sds-varinfo`. `sds-varinfo` expects all variable paths to have a leading
    slash.

[version 0.1.4] 2022-11-15

- **DAS-1683**
  - Added a small fix to the function SubsetDataLayers::add_dataset() that corrected an iterator that allows added datasets that are already included within an included ancestor dataset to be purged.

[version 0.1.3] 2022-09-26

- **DAS-1639**
  - Update sds-varinfo configuration file for GEDI L1 and L2 supplements.

[version 0.1.2] 2022-09-23

- **DAS-1639**
   - Update sds-varinfo config file to include GEDI L1 and L2 overrides.
   - Update sds-varinto to 2.2.0
   - Update harmony-service-library-py to ~1.0.21

[version 0.1.1] 2022-07-27

- **DAS-1445**
   - Update to allow photon subsetting to correctly handle padded segmented data.

[version 0.1.0] 2022-07-18

- **DAS-1530**
   - Update item processing to include all required variables in a request.
   - Update harmony-service-library-py to 1.0.20

[version 0.0.5]
