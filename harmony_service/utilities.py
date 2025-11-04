"""This module contains lower-level functionality that can be abstracted into
functions. Primarily this improves readability of the source code, and
allows finer-grained unit testing of each function.

"""

import json
from logging import Logger
from mimetypes import guess_type as guess_mime_type
from os.path import abspath, dirname, join, splitext
from subprocess import PIPE, Popen
from typing import List, Optional

from dateutil.parser import parse as parse_datetime
from harmony_service_lib.message import Message
from harmony_service_lib.message import Variable as HarmonyVariable
from varinfo import VarInfoFromNetCDF4

from harmony_service.exceptions import (
    CustomError,
    InternalError,
    InvalidParameter,
    MissingParameter,
    NoMatchingData,
    NoPolygonFound,
)

KNOWN_MIME_TYPES = {
    ".nc4": "application/x-netcdf4",
    ".nc": "application/x-netcdf",
    ".h5": "application/x-hdf",
    ".hdf5": "application/x-hdf",
    ".tif": "image/tiff",
    ".tiff": "image/tiff",
}


KNOWN_EXIT_STATUSES = {
    1: InvalidParameter,
    2: MissingParameter,
    3: NoMatchingData,
    6: NoPolygonFound,
}

TRAJECTORY_SUBSETTER_VARINFO_CONFIG = join(
    dirname(abspath(__file__)), "config", "trajectorysubsetter_varinfo_config.json"
)


def get_file_mimetype(file_name: str) -> Optional[str]:
    """This function tries to infer the MIME type of a file string. If the
    `mimetype.guess_type` function cannot guess the MIME type of the
    granule, a dictionary of known MIME type mappings is checked before a
    default value is returned.

    """
    mimetype = guess_mime_type(file_name, False)

    if not mimetype or mimetype[0] is None:
        _, file_extension = splitext(file_name)
        mimetype = KNOWN_MIME_TYPES.get(file_extension)
    else:
        mimetype = mimetype[0]

    return mimetype


def get_binary_exception(exit_status: int) -> CustomError:
    """Given an exit code from the Trajectory Subsetter binary,
    return a `CustomError` that can be raised. If the exit status does not
    match one of the known types, return an `InternalError` exception.

    """
    binary_exception = KNOWN_EXIT_STATUSES.get(exit_status)

    if binary_exception is not None:
        binary_exception = binary_exception()
    else:
        binary_exception = InternalError(exit_status)

    return binary_exception


def execute_command(command: str, logger: Logger) -> None:
    """This function invokes the Trajectory Subsetter binary. It
    will continue to poll the process output until there is an exit status.
    While doing so, it will retrieve any input from STDOUT and STDERR, and
    log those with the supplied `logging.Logger` instance associated with
    the main `HarmonyAdapter` class.

    The on-premises invocation checks for variables that cannot be
    subsetted, if a temporal or spatial subset is requested. If there are
    variables that cannot be subsetted, the on-premises response message
    contains an extra string stating such (although not specifically which
    variables). At present, Harmony does not return success with additional
    information, so that check is omitted from this function.

    """
    logger.info(f"Running command: {command}")

    with Popen(command, shell=True, stdout=PIPE, stderr=PIPE) as process:
        exit_status = process.poll()

        while exit_status is None:
            exit_status = process.poll()

            for stdout_line in process.stdout.readlines():
                logger.info(stdout_line.decode("utf-8"))

            for stderr_line in process.stderr.readlines():
                logger.error(stderr_line.decode("utf-8"))

    if exit_status != 0:
        raise get_binary_exception(exit_status)


def is_temporal_subset(message: Message) -> bool:
    """Check the content of a `harmony.message.Message` instance to determine
    if the request has asked for a temporal subset.

    """
    return message.temporal is not None


def is_bbox_spatial_subset(message: Message) -> bool:
    """Check the content of a `harmony.message.Message` instance to determine
    if the request has asked for a bounding box spatial subset.

    """
    return message.subset is not None and message.subset.bbox is not None


def is_polygon_spatial_subset(message: Message) -> bool:
    """Check the content of a `harmony.message.Message` instance to determine
    if the request has asked for a polygon spatial subset.

    """
    return message.subset is not None and message.subset.shape is not None


def is_variable_subset(variables: Optional[List[HarmonyVariable]]) -> bool:
    """Check the content of a `harmony.message.Message` instance to determine
    if the request has asked for a variable subset.

    """
    return variables is not None and len(variables) > 0


def is_harmony_subset(
    message: Message, variables: Optional[List[HarmonyVariable]]
) -> bool:
    """Check the content of a `harmony.message.Message` instance to determine
    if the request has asked for a subset operation. This includes a
    temporal, bounding box spatial or polygon spatial subset, but omits
    variable subsets.

    """
    return (
        is_temporal_subset(message)
        or is_bbox_spatial_subset(message)
        or is_polygon_spatial_subset(message)
        or is_variable_subset(variables)
    )


def convert_harmony_datetime(harmony_datetime_str: str) -> str:
    """Take an input Harmony datetime, as a string, and ensure it conforms to
    the required format of the Trajectory Subsetter binary. This function
    assumes that both the binary arguments and Harmony datetimes for start
    and end are in UTC. The Harmony message contains a trailing "Z", which
    the subsetter binary regular expression does not currently handle.

    """
    return parse_datetime(harmony_datetime_str).strftime("%Y-%m-%dT%H:%M:%S")


def include_support_variables(
    variables: list[str], short_name: str, filename: str
) -> set[str]:
    """Get support variables needed for a viable subset.

    Parse the variable list, ensuring all variable names have a leading
    slash, even when variable names supplied by Harmony do not. Then update
    that list to include all supporting variables necessary to use a
    subsetted file.

    """
    var_info = VarInfoFromNetCDF4(
        filename, short_name=short_name, config_file=TRAJECTORY_SUBSETTER_VARINFO_CONFIG
    )
    requested_vars = set(
        requested_var if requested_var.startswith("/") else f"/{requested_var}"
        for requested_var in variables
    )
    updated_vars = var_info.get_required_variables(requested_vars)
    return updated_vars


def write_source_variables_to_file(
    variables: set[str],
    working_dir: str,
) -> str:
    """
    Writes a list of variables to a temporary file.

    Args:
        variables: List of variable names.
        working_dir: Directory where the temp file will be created.

    Returns:
        str: URI of the temporary file containing the variables.
    """
    data = {"data_set_layers": [var.strip() for var in variables]}
    with open(
        join(working_dir, "source_vars.json"), "w+", encoding="utf-8"
    ) as variables_file:
        json.dump(data, variables_file)
    return variables_file.name
