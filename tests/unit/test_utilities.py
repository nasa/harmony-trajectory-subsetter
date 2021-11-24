from logging import Logger
from unittest import TestCase
from unittest.mock import Mock, patch

from harmony.message import Message

from harmony_service.exceptions import (InternalError, InvalidParameter,
                                        MissingParameter, NoMatchingData,
                                        NoPolygonFound)
from harmony_service.utilities import (convert_harmony_datetime,
                                       get_binary_exception, get_file_mimetype,
                                       execute_command, is_bbox_spatial_subset,
                                       is_harmony_subset,
                                       is_polygon_spatial_subset,
                                       is_temporal_subset, is_variable_subset)


class TestUtilities(TestCase):
    """ Test the helper functions in the harmony_service.utilities module. """
    @classmethod
    def setUpClass(cls):
        """ Test fixtures that can be reused between tests. """
        cls.base_message_content = {'accessToken': 'access',
                                    'callback': 'a callback',
                                    'sources': [{'collection': 'C123-EEDTEST',
                                                 'granules': []}],
                                    'stagingLocation': 'staging location',
                                    'user': 'cbolden'}

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

    def test_get_binary_exception(self):
        """ Ensure that the correct type of `CustomError` is returned for each
            type of exit status (and not raised). If an exit status is not
            known, an `InternalError` should be returned.

        """
        test_args = [['InvalidParameter', 1, InvalidParameter],
                     ['MissingParameter', 2, MissingParameter],
                     ['NoMatchingData', 3, NoMatchingData],
                     ['NoPolygonFound', 6, NoPolygonFound],
                     ['InternalError', 9999, InternalError]]

        for description, exit_status, expected_exception_class in test_args:
            with self.subTest(description):
                self.assertIsInstance(get_binary_exception(exit_status),
                                      expected_exception_class)

    @patch('harmony_service.utilities.get_binary_exception')
    def test_execute_command(self, mock_get_binary_exception):
        """ Ensure a shell command is invoked within a new process. It should
            return all logging to the supplied logger and if the exit status
            indicates failure, it should call `get_binary_exception`.

        """
        echo_value = 'string_value'
        test_command = f'echo "{echo_value}"'
        test_error_command = f'>&2 echo "{echo_value}" && exit 1'

        mock_logger = Mock(spec=Logger)
        mock_get_binary_exception.return_value = InvalidParameter()

        with self.subTest('A process runs a successful command'):
            self.assertIsNone(execute_command(test_command, mock_logger))
            self.assertEqual(mock_logger.info.call_count, 2)
            mock_logger.info.assert_any_call(
                'Running command: echo "string_value"'
            )
            mock_logger.info.assert_any_call(f'{echo_value}\n')

        mock_logger.reset_mock()

        with self.subTest('A process returns a non-zero exit status'):
            with self.assertRaises(InvalidParameter):
                execute_command(test_error_command, mock_logger)

            mock_logger.error.assert_called_once_with(f'{echo_value}\n')

    def test_is_temporal_subset(self):
        """ Ensure that a Harmony message will be correctly recognised as
            requesting a temporal subset or not.

        """
        with self.subTest('Temporal subset'):
            message_content = self.base_message_content.copy()
            message_content['temporal'] = {'start': '2021-02-03T04:05:06',
                                           'end': '2021-03-04T05:06:07'}
            message = Message(message_content)
            self.assertTrue(is_temporal_subset(message))

        with self.subTest('Non temporal subset'):
            message = Message(self.base_message_content)
            self.assertFalse(is_temporal_subset(message))

    def test_is_bbox_spatial_subset(self):
        """ Ensure that a Harmony message will be correctly recognised as
            requesting a bounding box spatial subset or not.

        """
        with self.subTest('Bounding box spatial subset'):
            message_content = self.base_message_content.copy()
            message_content['subset'] = {'bbox': [10, 20, 30, 40]}
            message = Message(message_content)
            self.assertTrue(is_bbox_spatial_subset(message))

        with self.subTest('Polygon spatial subset'):
            message_content = self.base_message_content.copy()
            message_content['subset'] = {
                'shape': {'href': 'url.com/file.h5',
                          'type': 'application/geo+json'}
            }
            message = Message(message_content)
            self.assertFalse(is_bbox_spatial_subset(message))

        with self.subTest('No spatial subset'):
            message = Message(self.base_message_content)
            self.assertFalse(is_bbox_spatial_subset(message))

    def test_is_polygon_spatial_subset(self):
        """ Ensure that a Harmony message will be correctly recognised as
            requesting a polygon spatial subset or not.

        """
        with self.subTest('Polygon spatial subset'):
            message_content = self.base_message_content.copy()
            message_content['subset'] = {
                'shape': {'href': 'url.com/file.h5',
                          'type': 'application/geo+json'}
            }
            message = Message(message_content)
            self.assertTrue(is_polygon_spatial_subset(message))

        with self.subTest('Bounding box spatial subset'):
            message_content = self.base_message_content.copy()
            message_content['subset'] = {'bbox': [10, 20, 30, 40]}
            message = Message(message_content)
            self.assertFalse(is_polygon_spatial_subset(message))

        with self.subTest('No spatial subset'):
            message = Message(self.base_message_content)
            self.assertFalse(is_polygon_spatial_subset(message))

    def test_is_variable_subset(self):
        """ Ensure that the function can recognise a list with element, an
            empty list, or a `None` type variable, and correctly determine if
            a variable subset has been requested.

        """
        with self.subTest('Empty list is not a variable subset'):
            self.assertFalse(is_variable_subset([]))

        with self.subTest('None type input is not a variable subset'):
            self.assertFalse(is_variable_subset([]))

        with self.subTest('A list of variables is a variable subset'):
            variables = [{
                'fullPath': 'var_one', 'id': 'V123-EEDTEST', 'name': 'var_one'
            }]
            self.assertTrue(is_variable_subset(variables))

    def test_is_harmony_subset(self):
        """ Ensure that a Harmony message will be correctly recognised as
            requesting a subset operation. Harmony currently lists these as
            spatial (bounding box or shape file), temporal or variable.

        """
        no_variables = []

        with self.subTest('Temporal subset'):
            message_content = self.base_message_content.copy()
            message_content['temporal'] = {'start': '2021-02-03T04:05:06',
                                           'end': '2021-03-04T05:06:07'}
            message = Message(message_content)
            self.assertTrue(is_harmony_subset(message, no_variables))

        with self.subTest('Bounding box spatial subset'):
            message_content = self.base_message_content.copy()
            message_content['subset'] = {'bbox': [10, 20, 30, 40]}
            message = Message(message_content)
            self.assertTrue(is_harmony_subset(message, no_variables))

        with self.subTest('Polygon spatial subset'):
            message_content = self.base_message_content.copy()
            message_content['subset'] = {
                'shape': {'href': 'url.com/file.h5',
                          'type': 'application/geo+json'}
            }
            message = Message(message_content)
            self.assertTrue(is_harmony_subset(message, no_variables))

        with self.subTest('Temporal and spatial subsets'):
            message_content = self.base_message_content.copy()
            message_content.update({
                'subset': {'bbox': [10, 20, 30, 40],
                           'shape': {'href': 'url.com/file.h5',
                                     'type': 'application/geo+json'}},
                'temporal': {'start': '2021-02-03T04:05:06',
                             'end': '2021-03-04T05:06:07'},
            })
            message = Message(message_content)
            self.assertTrue(is_harmony_subset(message, no_variables))

        with self.subTest('No spatial or temporal subsets'):
            message = Message(self.base_message_content)
            self.assertFalse(is_harmony_subset(message, no_variables))

        with self.subTest('Variable subset'):
            message_content = Message(self.base_message_content)
            variables = [{
                'fullPath': 'var_one', 'id': 'V123-EEDTEST', 'name': 'var_one'
            }]
            message = Message(self.base_message_content)
            self.assertTrue(is_harmony_subset(message, variables))

        with self.subTest('No subset operation requested'):
            message = Message(self.base_message_content)
            self.assertFalse(is_harmony_subset(message, no_variables))

    def test_convert_harmony_datetime(self):
        """ Ensure a datetime string, as included in a Harmony message, can be
            correctly converted to a format that is compatible with the
            Trajectory Subsetter. Mainly, this means omitting a trailing "Z"
            from the content of the Harmony message.

        """
        start_time = '2021-02-03T04:05:06'
        end_time = '2021-03-04T05:06:07'
        message_content = self.base_message_content.copy()
        message_content['temporal'] = {'start': start_time, 'end': end_time}
        message = Message(message_content)

        self.assertEqual(convert_harmony_datetime(message.temporal.start),
                         start_time)

        self.assertEqual(convert_harmony_datetime(message.temporal.end),
                         end_time)
