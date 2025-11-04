"""This module contains custom exceptions specific to the Harmony Trajectory
Subsetter. These exceptions are intended to allow for easier debugging of
the expected errors that may occur during an invocation of the service.

The initial list of exceptions was drawn from the `ICESat2ToolAdapter.py`
module.

"""


class CustomError(Exception):
    """Base class for exceptions in the Harmony Trajectory Subsetter."""

    def __init__(self, exception_type: str, message: str, exit_status: int):
        self.exception_type = exception_type
        self.exit_status = exit_status
        self.message = message
        super().__init__(self.message)


class InvalidParameter(CustomError):
    """Raised if the binary receives an unexpected parameter argument."""

    def __init__(self):
        super().__init__(
            "InvalidParameter", "Incorrect parameter specified for given dataset(s).", 1
        )


class MissingParameter(CustomError):
    """Raised if the binary does not receive an expected parameter."""

    def __init__(self):
        super().__init__(
            "MissingParameter",
            "No parameter value(s) specified for given dataset(s).",
            2,
        )


class NoMatchingData(CustomError):
    """Raised if the granule doesn't contain any data matching the user
    specifed constraints.

    """

    def __init__(self):
        super().__init__(
            "NoMatchingData", "No data found that matched subset constraints.", 3
        )


class NoPolygonFound(CustomError):
    """Raised if the supplied shape file in the `--boundingshape` argument
    does not contain valid GeoJSON content.

    """

    def __init__(self):
        super().__init__(
            "NoPolygonFound", "No polygon found in the given GeoJSON shapefile.", 6
        )


class InternalError(CustomError):
    """A default error from the binary, if the exit status code does not
    match one of the other `CustomError` exceptions.

    """

    def __init__(self, exit_status):
        super().__init__(
            "InternalError",
            f"An internal error occurred, code: {exit_status}",
            exit_status,
        )
