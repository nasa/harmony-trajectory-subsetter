from os.path import basename
from unittest import TestCase
from unittest.mock import patch

from harmony.message import Message
from harmony.util import config, HarmonyException

from harmony_service.adapter import HarmonyAdapter, main


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
                       'url': 'path/to/file.h5'}
        cls.mimetype = 'application/x-hdf'
        cls.staging_location = 's3://example-bucket'
        cls.user = 'tglennan'

    def test_subsetter_request(self, mock_download, mock_stage,
                               mock_get_mimetype):
        """ Ensure a simple request will call the expected Harmony functions,
            including to retrieve and stage files.

        """
        mock_download.return_value = self.granule['url']
        mock_get_mimetype.return_value = (self.mimetype, None)
        mock_stage.return_value = 'path/to/staged/output'

        message = Message({'accessToken': self.access_token,
                           'callback': self.callback,
                           'sources': [{'collection': self.collection,
                                        'granules': [self.granule],
                                        'variables': None}],
                           'stagingLocation': self.staging_location,
                           'user': self.user})

        subsetter = HarmonyAdapter(message, config=self.config)
        subsetter.invoke()

        mock_stage.assert_called_once_with(message.granules[0].url,
                                           basename(message.granules[0].url),
                                           self.mimetype,
                                           location=self.staging_location,
                                           logger=subsetter.logger)
        mock_get_mimetype.assert_called_once_with(message.granules[0].url)

    def test_exception_raised(self, mock_download, mock_stage,
                              mock_get_mimetype):
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

        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    def test_missing_granules(self, mock_download, mock_stage,
                              mock_get_mimetype):
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
                             'No granules specified for the Trajectory subsetter')

        mock_download.assert_not_called()
        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    def test_synchronous_multiple_granules(self, mock_download, mock_stage,
                                           mock_get_mimetype):
        """ Ensure a  synchronous request with more than one granule raises an
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

        mock_download.assert_not_called()
        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    @patch('harmony_service.adapter.run_cli')
    def test_main(self, mock_run_cli, mock_download, mock_stage,
                  mock_get_mimetype):
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
