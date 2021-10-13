""" This module contains lower-level functionality that can be abstracted into
    functions. Primarily this improves readability of the source code, and
    allows finer-grained unit testing of each function.

"""
from mimetypes import guess_type as guess_mime_type
from os.path import splitext
from typing import Optional


KNOWN_MIME_TYPES = {'.nc4': 'application/x-netcdf4',
                    '.nc': 'application/x-netcdf',
                    '.h5': 'application/x-hdf',
                    '.hdf5': 'application/x-hdf',
                    '.tif': 'image/tiff',
                    '.tiff': 'image/tiff'}


def get_file_mimetype(file_name: str) -> Optional[str]:
    """ This function tries to infer the MIME type of a file string. If the
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
