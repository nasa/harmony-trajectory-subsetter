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
  - Bug fix: Added the required format output argument to an instantiation of the Subsetter class.


[version 0.1.11] 2023-07-20

- **DAS-1887**
  - Bug fix: Updated ForwardRefCoordinates.h to use forward and backward scanning methods to ensure that the fill values in the index begin dataset are skipped.

[version 0.1.10] 2023-06-19

- **DAS-1731**
  - Bug fix: Updated FwdRefBeginDataset.h to correctly calculate index begin datasets for ATL03.

[version 0.1.9] 2023-04-17

- **DAS-1806**
  - Refactoring: Removed global namespaces and updated Subset.cpp and Subsetter.h to incorporate modern coding standards.

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
