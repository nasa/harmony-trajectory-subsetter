"""This module contains functions for updating or creating a history attribute
to maintain file provenance.

"""

import json
import os
from typing import Dict, List

import h5py

from harmony_service_lib.provenance import get_updated_history_metadata

from harmony_service.utilities import get_file_mimetype


PROGRAM = "Harmony Trajectory Subsetter"


def update_history_metadata(
    binary_parameters: Dict[str, str], variables: List[str]
) -> None:
    """Update the history global attribute of the output file.

    If either an existing History or history global attribute exists, append
    information as a new line to the existing value. Otherwise create a new
    attribute called "history".

    A history attribute can only be created/ammended in HDF5 and NetCDF4 files
    (i.e., not GeoTIFF).
    """
    if needs_history_metadata(binary_parameters["--outfile"]):
        with h5py.File(binary_parameters["--outfile"], "a") as output_file:
            history_attribute_name, existing_history = read_history_attrs(output_file)

            write_history_attrs(
                output_file,
                binary_parameters,
                history_attribute_name,
                existing_history,
                variables,
            )


def read_history_attrs(output_file: h5py.File) -> tuple[str, str | None]:
    """Read history attribute."""
    if "History" in output_file.attrs:
        history_attribute_name = "History"
        existing_history = output_file.attrs["History"]
        # Convert bytes to string if needed
        if isinstance(existing_history, bytes):
            existing_history = existing_history.decode("utf-8")
    elif "history" in output_file.attrs:
        history_attribute_name = "history"
        existing_history = output_file.attrs["history"]
        # Convert bytes to string if needed
        if isinstance(existing_history, bytes):
            existing_history = existing_history.decode("utf-8")
    else:
        history_attribute_name = "history"
        existing_history = None
    return history_attribute_name, existing_history


def write_history_attrs(
    output_file: h5py.File,
    binary_parameters: Dict[str, str],
    history_attribute_name: str,
    existing_history: str,
    variables: List[str],
) -> None:
    """Write history attribute.

    The following is written to the history attribute:

    * Prior history attribute (if it exists)
    * Date of execution
    * Service name
    * Service version
    * Subset parameters (if they exist):
        * Requested variables
        * Bounding box
        * Bounding shape
        * Temporal range

    """
    new_history = get_updated_history_metadata(
        PROGRAM, get_semantic_version(), existing_history
    )

    subset_parameters = get_subset_parameters(binary_parameters, variables)
    new_history_with_subset = " ".join(filter(None, [new_history, subset_parameters]))

    output_file.attrs[history_attribute_name] = new_history_with_subset


def get_semantic_version() -> str:
    """Parse the service_version.txt to get the semantic version number."""
    current_directory = os.path.dirname(os.path.abspath("__file__"))
    path = os.path.join(current_directory, "docker/service_version.txt")
    with open(path, encoding="utf-8") as file_handler:
        semantic_version = file_handler.read().strip()
        if not semantic_version:
            return "[version not found]"
        return semantic_version


def needs_history_metadata(output_file_name: str) -> bool:
    """Checks if history should be added to the output file.

    The history attribute should be created/updated if the output is either
    HDF5 or NetCDF4 formatted. The only available format that is not HDF5 or
    NetCDF4 formatted is GeoTIFF. If the mime type is not recognized, it
    defaults to NetCDF4.

    """
    file_format = get_file_mimetype(output_file_name)
    return file_format not in ("image/tiff", "image/tif")


def get_subset_parameters(
    binary_parameters: Dict[str, str], variables: List[str]
) -> str:
    """Return the subset parameters in the request.

    This returns the subset parameters found in the request (spatial, temporal,
    and variable). The spatial and temporal values were already extracted from
    the Harmony Message, so the resulting parameter dictionary can just be
    accessed.

    Extracting the requested variables must be done by accessing the Harmony
    Message directly, since the `--includedataset` parameter includes both
    the requested and required variables.
    """
    subset_parameters = []

    if "--bbox" in binary_parameters:
        subset_parameters.append(binary_parameters["--bbox"])

    if "--boundingshape" in binary_parameters:
        subset_parameters.append(
            json.loads(binary_parameters["--boundingshape"].strip("'"))
        )

    if "--start" in binary_parameters and "--end" in binary_parameters:
        subset_parameters.append(
            f"{binary_parameters['--start']}, {binary_parameters['--end']}"
        )

    if variables:
        subset_parameters.append(",".join(variables))
    else:
        subset_parameters.append("All variables.")

    return json.dumps(subset_parameters)
