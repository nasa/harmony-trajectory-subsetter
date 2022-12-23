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
