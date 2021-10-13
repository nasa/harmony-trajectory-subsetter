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

* `bin` - To be added, DAS-1259.
* `DataAccess` - To be added, DAS-1252.
* `docker` - To be added, DAS-1259.
* `harmony_service` - A directory containing the Python code to invoke the
  Trajectory Subsetter within a Harmony service.
* `tests` - Python unit tests for the Harmony service functionality.

### Harmony service conda environment:

The Harmony service for the trajectory subsetter runs within a conda
environment, itself within a Docker container. To recreate the same conda
environment in a local terminal, use the following commands:

```bash
conda create --name trajectory python=3.9 --channel conda-forge --channel defaults
conda activate trajectory
pip install -r ./harmony_service/pip_requirements.txt
```
