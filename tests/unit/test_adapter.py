from os import makedirs
from os.path import basename, exists
from shutil import rmtree
from unittest import TestCase
from unittest.mock import patch

from harmony.message import Message, Source
from harmony.util import config, HarmonyException
from pystac import Asset

from harmony_service.adapter import (HarmonyAdapter, main,
                                     SUBSETTER_BINARY_PATH, SUBSETTER_CONFIG)


@patch('harmony_service.adapter.mkdtemp', return_value='tests/temp')
@patch('harmony_service.adapter.get_file_mimetype')
@patch('harmony_service.adapter.stage')
@patch('harmony_service.adapter.download')
class TestAdapter(TestCase):
    """ Test the HarmonyAdapter class for basic functionality. """
    @classmethod
    def setUpClass(cls):
        """ Fixtures that only need to be instantiated once for all tests. """
        cls.access_token = 'access'
        cls.callback = 'https://example.com'
        cls.collection = 'C1234567890-TEST'
        cls.config = config(validate=False)
        cls.granule = {'id': 'G2345678901-TEST',
                       'temporal': {'start': '2021-01-01T00:00:00.000Z',
                                    'end': '2021-01-01T00:00:00.000Z'},
                       'url': 'file.h5'}
        cls.mimetype = 'application/x-hdf'
        cls.staging_location = 's3://example-bucket'
        cls.subsetted_filename = 'file_subsetted.h5'
        cls.temp_dir = 'tests/temp'
        cls.user = 'tglennan'

    def setUp(self):
        """ Fixtures that must be created individually for all tests. """
        makedirs(self.temp_dir, exist_ok=True)

    def tearDown(self):
        """ Clean-up after each test. """
        if exists(self.temp_dir):
            rmtree(self.temp_dir)

    def test_subsetter_request(self, mock_download, mock_stage,
                               mock_get_mimetype, mock_mkdtemp):
        """ Ensure a simple request will call the expected Harmony functions,
            including to retrieve and stage files.

        """
        local_input_path = f'{self.temp_dir}/{self.granule["url"]}'
        mock_download.return_value = local_input_path
        mock_get_mimetype.return_value = (self.mimetype, None)
        mock_stage.return_value = 'tests/to/staged/output'

        message = Message({'accessToken': self.access_token,
                           'callback': self.callback,
                           'sources': [{'collection': self.collection,
                                        'granules': [self.granule],
                                        'variables': None}],
                           'stagingLocation': self.staging_location,
                           'user': self.user})

        subsetter = HarmonyAdapter(message, config=self.config)
        subsetter.invoke()

        mock_mkdtemp.assert_called_once()
        mock_stage.assert_called_once_with(local_input_path,
                                           basename(message.granules[0].url),
                                           self.mimetype,
                                           location=self.staging_location,
                                           logger=subsetter.logger)
        mock_get_mimetype.assert_called_once_with(local_input_path)

    def test_exception_raised(self, mock_download, mock_stage,
                              mock_get_mimetype, mock_mkdtemp):
        """ Ensure a request that raises an exception ends with a
            HarmonyException being raised.

        """
        mock_download.side_effect = Exception('Failed to download')

        message = Message({'accessToken': self.access_token,
                           'callback': self.callback,
                           'sources': [{'collection': self.collection,
                                        'granules': [self.granule],
                                        'variables': None}],
                           'stagingLocation': self.staging_location,
                           'user': self.user})

        subsetter = HarmonyAdapter(message, config=self.config)

        with self.assertRaises(HarmonyException) as context_manager:
            subsetter.invoke()

            self.assertEqual(str(context_manager.exception),
                             ('L2 Trajectory Subsetter failed with error: '
                              'Failed to download'))

        mock_mkdtemp.assert_called_once()
        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    def test_missing_granules(self, mock_download, mock_stage,
                              mock_get_mimetype, mock_mkdtemp):
        """ A request with no specified granules in an inbound message should
            raise an exception.

        """
        message = Message({'accessToken': self.access_token,
                           'callback': self.callback,
                           'sources': [{'collection': self.collection}],
                           'stagingLocation': self.staging_location,
                           'user': self.user})

        subsetter = HarmonyAdapter(message, config=self.config)

        with self.assertRaises(Exception) as context_manager:
            subsetter.invoke()
            self.assertEqual(str(context_manager.exception),
                             'No granules specified for the Trajectory '
                             'subsetter')

        mock_mkdtemp.assert_not_called()
        mock_download.assert_not_called()
        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    def test_synchronous_multiple_granules(self, mock_download, mock_stage,
                                           mock_get_mimetype, mock_mkdtemp):
        """ Ensure asynchronous request with more than one granule raises an
            exception.

        """
        message = Message({'accessToken': self.access_token,
                           'callback': self.callback,
                           'isSynchronous': True,
                           'sources': [{'collection': self.collection,
                                        'granules': [self.granule,
                                                     self.granule]}],
                           'stagingLocation': self.staging_location,
                           'user': self.user})

        subsetter = HarmonyAdapter(message, config=self.config)

        with self.assertRaises(Exception) as context_manager:
            subsetter.invoke()
            self.assertEqual(str(context_manager.exception),
                             'Synchronous requests accept only one granule.')

        mock_mkdtemp.assert_not_called()
        mock_download.assert_not_called()
        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    def test_validation_temporal(self, mock_download, mock_stage,
                                 mock_get_mimetype, mock_mkdtemp):
        """ Ensure a Harmony message containing temporal subset information
            only passes validation if it has both a start and end time for the
            temporal range.

        """
        mock_download.return_value = self.granule['url']
        mock_get_mimetype.return_value = (self.mimetype, None)
        mock_stage.return_value = 'tests/to/staged/output'

        start_time = '2001-02-03T04:05:06'
        end_time = '2002-03-04T05:06:07'

        test_failure_args = [['No temporal end', start_time, None],
                             ['No temporal start', None, end_time]]

        for description, temporal_start, temporal_end in test_failure_args:
            with self.subTest(description):
                message = Message({'accessToken': self.access_token,
                                   'callback': self.callback,
                                   'sources': [{'collection': self.collection,
                                                'granules': [self.granule]}],
                                   'stagingLocation': self.staging_location,
                                   'temporal': {'start': temporal_start,
                                                'end': temporal_end},
                                   'user': self.user})

                subsetter = HarmonyAdapter(message, config=self.config)

                with self.assertRaises(Exception) as context_manager:
                    subsetter.invoke()
                    self.assertEqual(str(context_manager.exception),
                                     'Invalid temporal range, both start and '
                                     'end required.')

                mock_mkdtemp.assert_not_called()
                mock_download.assert_not_called()
                mock_get_mimetype.assert_not_called()
                mock_stage.assert_not_called()

        with self.subTest('Temporal subset with both start and end'):
            message = Message({'accessToken': self.access_token,
                               'callback': self.callback,
                               'sources': [{'collection': self.collection,
                                            'granules': [self.granule]}],
                               'stagingLocation': self.staging_location,
                               'temporal': {'start': start_time,
                                            'end': end_time},
                               'user': self.user})

            subsetter = HarmonyAdapter(message, config=self.config)
            subsetter.invoke()

            expected_local_out = f'{self.temp_dir}/{self.subsetted_filename}'
            mock_mkdtemp.assert_called_once()
            mock_stage.assert_called_once_with(expected_local_out,
                                               self.subsetted_filename,
                                               self.mimetype,
                                               location=self.staging_location,
                                               logger=subsetter.logger)
            mock_get_mimetype.assert_called_once_with(expected_local_out)

    def test_validation_shapefile(self, mock_download, mock_stage,
                                  mock_get_mimetype, mock_mkdtemp):
        """ Ensure only a shape file with the correct MIME type will pass
            validation. Any non-GeoJSON shape file should raise an exception.

        """
        mock_download.return_value = self.granule['url']
        mock_get_mimetype.return_value = (self.mimetype, None)
        mock_stage.return_value = 'tests/to/staged/output'

        with self.subTest('Non GeoJSON shape file'):
            message = Message({
                'accessToken': self.access_token,
                'callback': self.callback,
                'sources': [{'collection': self.collection,
                             'granules': [self.granule]}],
                'stagingLocation': self.staging_location,
                'subset': {'shape': {'href': 'url.notgeojson',
                                     'type': 'other MIME type'}},
                'user': self.user,
            })

            subsetter = HarmonyAdapter(message, config=self.config)

            with self.assertRaises(Exception) as context_manager:
                subsetter.invoke()
                self.assertEqual(str(context_manager.exception),
                                 'Invalid shape file format. Must be GeoJSON.')

            mock_mkdtemp.assert_not_called()
            mock_download.assert_not_called()
            mock_get_mimetype.assert_not_called()
            mock_stage.assert_not_called()

        with self.subTest('GeoJSON shape file'):
            message = Message({
                'accessToken': self.access_token,
                'callback': self.callback,
                'sources': [{'collection': self.collection,
                             'granules': [self.granule]}],
                'stagingLocation': self.staging_location,
                'subset': {'shape': {'href': 'url.geo.json',
                                     'type': 'application/geo+json'}},
                'user': self.user,
            })

            subsetter = HarmonyAdapter(message, config=self.config)
            subsetter.invoke()

            expected_local_out = f'{self.temp_dir}/{self.subsetted_filename}'
            mock_mkdtemp.assert_called_once()
            mock_stage.assert_called_once_with(expected_local_out,
                                               self.subsetted_filename,
                                               self.mimetype,
                                               location=self.staging_location,
                                               logger=subsetter.logger)
            mock_get_mimetype.assert_called_once_with(expected_local_out)

    def test_parse_binary_parameters(self, mock_download, mock_stage,
                                     mock_get_mimetype, mock_mkdtemp):
        """ Ensure Harmony messages for the Trajectory Subsetter that specify
            a variety of transformations (spatial, temporal, variable)
            correctly translate the message parameters into a dictionary of
            variables to be given to the Trajectory Subsetter binary file.

        """
        base_source = {'collection': self.collection,
                       'granules': [self.granule]}

        base_message = {'accessToken': self.access_token,
                        'callback': self.callback,
                        'stagingLocation': self.staging_location,
                        'user': self.user}

        asset = Asset(href=self.granule['url'])
        shape_file_url = 'https://shape.com/shape.geo.json'
        start_time = '2021-02-03T04:05:06'
        end_time = '2021-02-03T05:06:07'
        source_variables = [{'fullPath': 'var_one',
                             'id': 'V1234-EEDTEST',
                             'name': 'var_one'},
                            {'fullPath': '/nested/var_two',
                             'id': 'V2345-EEDTEST',
                             'name': 'var_two'}]

        # First result is input granule, second is a GeoJSON shape file.
        local_input_path = f'{self.temp_dir}/random_uuid.h5'
        local_shape_file_path = f'{self.temp_dir}/shape.geo.json'
        download_list = [local_input_path, local_shape_file_path]

        with self.subTest('No variables, temporal, bbox or polygon'):
            mock_download.side_effect = download_list
            expected_parameters = {
                '--configfile': SUBSETTER_CONFIG,
                '--filename': local_input_path,
                '--outfile': f'{self.temp_dir}/{self.granule["url"]}'
            }
            message_content = base_message.copy()
            message_content['sources'] = [base_source]
            message = Message(message_content)
            source = Source(base_source)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            mock_download.assert_called_once_with(
                asset.href, self.temp_dir, logger=subsetter.logger,
                access_token=self.access_token, cfg=self.config
            )
            mock_download.reset_mock()

        with self.subTest('Variable subset'):
            mock_download.side_effect = download_list
            expected_parameters = {
                '--configfile': SUBSETTER_CONFIG,
                '--filename': local_input_path,
                '--includedataset': 'var_one,/nested/var_two',
                '--outfile': f'{self.temp_dir}/{self.granule["url"]}'
            }
            message_content = base_message.copy()
            source_content = base_source.copy()
            source_content['variables'] = source_variables

            message_content['sources'] = [source_content]
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            mock_download.assert_called_once_with(
                asset.href, self.temp_dir, logger=subsetter.logger,
                access_token=self.access_token, cfg=self.config
            )
            mock_download.reset_mock()

        with self.subTest('Bounding box spatial subset'):
            mock_download.side_effect = download_list
            expected_parameters = {
                '--bbox': '[10,20,30,40]',
                '--configfile': SUBSETTER_CONFIG,
                '--filename': local_input_path,
                '--outfile': f'{self.temp_dir}/{self.subsetted_filename}'
            }
            message_content = base_message.copy()
            message_content.update({'sources': [base_source],
                                    'subset': {'bbox': [10, 20, 30, 40]}})
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            mock_download.assert_called_once_with(
                asset.href, self.temp_dir, logger=subsetter.logger,
                access_token=self.access_token, cfg=self.config
            )
            mock_download.reset_mock()

        with self.subTest('Polygon spatial subset'):
            mock_download.side_effect = download_list
            expected_parameters = {
                '--boundingshape': local_shape_file_path,
                '--configfile': SUBSETTER_CONFIG,
                '--filename': local_input_path,
                '--outfile': f'{self.temp_dir}/{self.subsetted_filename}'
            }
            message_content = base_message.copy()
            message_content.update({
                'sources': [base_source],
                'subset': {'shape': {'href': shape_file_url,
                                     'type': 'application/geo+json'}}
            })
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            self.assertEqual(mock_download.call_count, 2)
            mock_download.assert_any_call(
                asset.href, self.temp_dir, logger=subsetter.logger,
                access_token=self.access_token, cfg=self.config
            )
            mock_download.assert_any_call(
                shape_file_url, self.temp_dir, logger=subsetter.logger,
                access_token=self.access_token, cfg=self.config
            )
            mock_download.reset_mock()

        with self.subTest('Temporal subset'):
            mock_download.side_effect = download_list
            expected_parameters = {
                '--configfile': SUBSETTER_CONFIG,
                '--end': end_time,
                '--filename': local_input_path,
                '--outfile': f'{self.temp_dir}/{self.subsetted_filename}',
                '--start': start_time,
            }
            message_content = base_message.copy()
            message_content.update({
                'sources': [base_source],
                'temporal': {'start': start_time, 'end': end_time}
            })
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            mock_download.assert_called_once_with(
                asset.href, self.temp_dir, logger=subsetter.logger,
                access_token=self.access_token, cfg=self.config
            )
            mock_download.reset_mock()

        with self.subTest('Variables, bounding box, polygon and temporal'):
            mock_download.side_effect = download_list
            expected_parameters = {
                '--bbox': '[10,20,30,40]',
                '--boundingshape': local_shape_file_path,
                '--configfile': SUBSETTER_CONFIG,
                '--end': end_time,
                '--filename': local_input_path,
                '--includedataset': 'var_one,/nested/var_two',
                '--outfile': f'{self.temp_dir}/{self.subsetted_filename}',
                '--start': start_time,
            }
            source_content = base_source.copy()
            source_content['variables'] = source_variables

            message_content['sources'] = [source_content]
            message_content = base_message.copy()
            message_content.update({
                'sources': [source_content],
                'subset': {'bbox': [10, 20, 30, 40],
                           'shape': {'href': shape_file_url,
                                     'type': 'application/geo+json'}},
                'temporal': {'start': start_time, 'end': end_time}
            })
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            self.assertEqual(mock_download.call_count, 2)
            mock_download.assert_any_call(
                asset.href, self.temp_dir, logger=subsetter.logger,
                access_token=self.access_token, cfg=self.config
            )
            mock_download.assert_any_call(
                shape_file_url, self.temp_dir, logger=subsetter.logger,
                access_token=self.access_token, cfg=self.config
            )
            mock_download.reset_mock()

    @patch('harmony_service.adapter.execute_command')
    def test_transform(self, mock_execute_command, mock_download, mock_stage,
                       mock_get_mime_type, mock_mkdtemp):
        """ Ensure that a command to invoke the binary subsetter is correctly
            constructed and passed to the
            `harmony_service.utilities.execute_command` function.

        """
        binary_parameters = {'--par_one': 'val_one',
                             '--par_two': 'val_two'}

        expected_command = (f'{SUBSETTER_BINARY_PATH} --par_one val_one '
                            '--par_two val_two')

        message = Message({'accessToken': self.access_token,
                           'callback': self.callback,
                           'sources': [{'collection': self.collection,
                                        'granules': [self.granule]}],
                           'stagingLocation': self.staging_location,
                           'user': self.user})

        subsetter = HarmonyAdapter(message, config=self.config)
        subsetter.transform(binary_parameters)

        mock_execute_command.assert_called_once_with(expected_command,
                                                     subsetter.logger)

    @patch('harmony_service.adapter.run_cli')
    def test_main(self, mock_run_cli, mock_download, mock_stage,
                  mock_get_mimetype, mock_mkdtemp):
        """ Ensure expected arguments can be parsed, and that unexpected
            arguments result in an exception being raised.

        """
        with self.subTest('Parsing known arguments'):
            known_arguments = ['--harmony-action', 'invoke',
                               '--harmony-input', '{"accessToken": "..."}']
            main(known_arguments, config=self.config)
            namespace = mock_run_cli.call_args[0][1]
            self.assertEqual(namespace.harmony_action, 'invoke')
            self.assertEqual(namespace.harmony_input, '{"accessToken": "..."}')
            self.assertIn('cfg', mock_run_cli.call_args[1])
            self.assertEqual(mock_run_cli.call_args[1]['cfg'], self.config)

        mock_run_cli.reset_mock()

        with self.subTest('Ignoring unexpected arguments'):
            unknown_arguments = ['--something_wicked', 'this way comes']
            main(unknown_arguments)
            namespace = mock_run_cli.call_args[0][1]
            self.assertIsNone(namespace.harmony_action)
            self.assertIsNone(namespace.harmony_input)
            self.assertFalse(hasattr(namespace, 'something_wicked'))
            self.assertIn('cfg', mock_run_cli.call_args[1])
            self.assertIsNone(mock_run_cli.call_args[1]['cfg'])

        mock_run_cli.reset_mock()
