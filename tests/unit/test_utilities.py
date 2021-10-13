from unittest import TestCase

from harmony_service.utilities import get_file_mimetype


class TestUtilities(TestCase):
    """ Test the helper functions in the harmony_service.utilities module. """

    def test_get_file_mimetype(self):
        """ Ensure the expected MIME type is returned for a variety of file
            paths. Initially, the in-built Python `mimetypes.guess_mime_type`
            function should be used. If that is unable to identify the MIME
            type, a dictionary of known Earth Observation file extensions
            should be used.

        """
        test_args = [['file.nc4', 'application/x-netcdf4'],
                     ['file.nc', 'application/x-netcdf'],
                     ['file.h5', 'application/x-hdf'],
                     ['file.hdf5', 'application/x-hdf'],
                     ['file.tif', 'image/tiff'],
                     ['file.tiff', 'image/tiff'],
                     ['file.csv', 'text/csv'],
                     ['file.unknown', None]]

        for file_path, expected_mime_type in test_args:
            with self.subTest(f'MIME type for {file_path}'):
                self.assertEqual(get_file_mimetype(file_path),
                                 expected_mime_type)
