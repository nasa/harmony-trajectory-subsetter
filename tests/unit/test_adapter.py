from datetime import datetime
from os import makedirs
from os.path import basename, exists, join
from shutil import rmtree
from unittest import TestCase
from unittest.mock import patch

from harmony_service_lib.message import Message, Source
from harmony_service_lib.exceptions import NoDataException
from harmony_service_lib.util import HarmonyException, bbox_to_geometry, config
from pystac import Asset, Catalog, Item

from harmony_service.adapter import (
    SUBSETTER_BINARY_PATH,
    SUBSETTER_CONFIG,
    HarmonyAdapter,
    main,
)
from harmony_service.exceptions import NoMatchingData


@patch("harmony_service.adapter.include_support_variables")
@patch("harmony_service.adapter.mkdtemp", return_value="tests/temp")
@patch("harmony_service.adapter.get_file_mimetype")
@patch("harmony_service.adapter.stage")
@patch("harmony_service.adapter.download")
class TestAdapter(TestCase):
    """Test the HarmonyAdapter class for basic functionality."""

    @classmethod
    def setUpClass(cls):
        """Fixtures that only need to be instantiated once for all tests."""
        cls.access_token = "access"
        cls.bounding_shape = (
            '\'{"type": "FeatureCollection", "features": '
            '[{"type": "Feature", "properties": {}, '
            '"geometry": {"type": "Polygon", "coordinates": '
            "[[[20, 15], [50, 15], [50, 40], [20, 40], "
            "[20, 15]]]}}]}'"
        )
        cls.callback = "https://example.com"
        cls.collection = "C1234567890-TEST"
        cls.config = config(validate=False)
        cls.granule = {
            "id": "G2345678901-TEST",
            "temporal": {
                "start": "2021-01-01T00:00:00.000Z",
                "end": "2021-01-01T00:00:00.000Z",
            },
            "url": "file.h5",
        }
        cls.local_shape_path = "tests/data/box.geo.json"
        cls.mimetype = "application/x-hdf"
        cls.staging_location = "s3://example-bucket"
        cls.subsetted_filename = "file_subsetted.h5"
        cls.temp_dir = "tests/temp"
        cls.user = "tglennan"
        cls.shortname = "ATL24"
        cls.loglevel = "INFO"

    @staticmethod
    def side_effect_fxn(input, short_name):
        return input

    def setUp(self):
        """Fixtures that must be created individually for all tests."""
        makedirs(self.temp_dir, exist_ok=True)

        self.input_catalog = Catalog(id="input", description="test input")
        input_item = Item(
            id="input_granule",
            geometry=bbox_to_geometry([-180, -90, 180, 90]),
            bbox=[-180, -90, 180, 90],
            datetime=datetime(2001, 1, 1),
            properties=None,
        )
        input_item.add_asset(
            "input",
            Asset(
                "https://www.example.com/file.h5",
                media_type="application/x-hdf",
                roles=["data"],
            ),
        )
        self.input_catalog.add_item(input_item)

    def tearDown(self):
        """Clean-up after each test."""
        if exists(self.temp_dir):
            rmtree(self.temp_dir)

    @patch("harmony_service.adapter.execute_command")
    def test_subsetter_request(
        self,
        mock_execute_command,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure a simple request will call the expected Harmony functions,
        including to retrieve and stage files.

        """
        local_input_path = f"{self.temp_dir}/{self.granule['url']}"
        staged_output_url = "tests/to/staged/output_subsetted.h5"
        mock_download.return_value = local_input_path
        mock_get_mimetype.return_value = self.mimetype
        mock_stage.return_value = staged_output_url
        mock_support_variables.side_effect = self.side_effect_fxn

        message = Message(
            {
                "accessToken": self.access_token,
                "callback": self.callback,
                "sources": [
                    {
                        "collection": self.collection,
                        "granules": [self.granule],
                        "variables": None,
                        "shortName": self.shortname,
                    }
                ],
                "stagingLocation": self.staging_location,
                "user": self.user,
            }
        )

        subsetter = HarmonyAdapter(
            message, config=self.config, catalog=self.input_catalog
        )
        _, output_catalog = subsetter.invoke()

        expected_command = (
            f"{SUBSETTER_BINARY_PATH} "
            f"--configfile {SUBSETTER_CONFIG} "
            f"--filename {local_input_path} "
            f"--shortname {self.shortname} "
            f"--loglevel {self.loglevel} "
            f"--outfile {local_input_path}"
        )

        mock_mkdtemp.assert_called_once()
        mock_execute_command.assert_called_once_with(expected_command, subsetter.logger)
        mock_stage.assert_called_once_with(
            local_input_path,
            basename(message.granules[0].url),
            self.mimetype,
            location=self.staging_location,
            logger=subsetter.logger,
        )
        mock_get_mimetype.assert_called_once_with(local_input_path)

        # Confirm output catalog only contains an item for the subsetter
        # output, and that there is a single asset pointing to the subsetted
        # file
        output_items = list(output_catalog.get_all_items())
        self.assertEqual(len(output_items), 1)
        self.assertListEqual(list(output_items[0].assets.keys()), ["data"])
        self.assertEqual(output_items[0].assets["data"].href, staged_output_url)

    def test_exception_raised(
        self,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure a request that raises an exception ends with a
        HarmonyException being raised.

        """
        mock_download.side_effect = Exception("Failed to download")

        message = Message(
            {
                "accessToken": self.access_token,
                "callback": self.callback,
                "sources": [
                    {
                        "collection": self.collection,
                        "granules": [self.granule],
                        "variables": None,
                        "shortName": self.shortname,
                    }
                ],
                "stagingLocation": self.staging_location,
                "user": self.user,
            }
        )

        subsetter = HarmonyAdapter(
            message, config=self.config, catalog=self.input_catalog
        )

        with self.assertRaises(HarmonyException) as context_manager:
            subsetter.invoke()

        self.assertEqual(
            context_manager.exception.message,
            ("Trajectory Subsetter failed with error: Failed to download"),
        )

        mock_mkdtemp.assert_called_once()
        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    @patch("harmony_service.adapter.execute_command")
    def test_no_matching_data_exception(
        self,
        mock_execute_command,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure a request that has no data found matching the subset
        constraints raises the harmony-defined NoDataException

        """
        mock_execute_command.side_effect = NoMatchingData()
        local_input_path = f"{self.temp_dir}/{self.granule['url']}"
        staged_output_url = "tests/to/staged/output_subsetted.h5"
        mock_download.return_value = local_input_path
        mock_get_mimetype.return_value = self.mimetype
        mock_stage.return_value = staged_output_url
        mock_support_variables.side_effect = self.side_effect_fxn

        message = Message(
            {
                "accessToken": self.access_token,
                "callback": self.callback,
                "sources": [
                    {
                        "collection": self.collection,
                        "granules": [self.granule],
                        "variables": None,
                        "shortName": self.shortname,
                    }
                ],
                "stagingLocation": self.staging_location,
                "user": self.user,
            }
        )

        subsetter = HarmonyAdapter(
            message, config=self.config, catalog=self.input_catalog
        )

        with self.assertRaises(NoDataException) as context_manager:
            subsetter.invoke()

        self.assertEqual(
            context_manager.exception.message, ("No data found to process")
        )

    def test_missing_granules(
        self,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """A request with no specified granules in an inbound message should
        raise an exception.

        """
        message = Message(
            {
                "accessToken": self.access_token,
                "callback": self.callback,
                "sources": [{"collection": self.collection}],
                "stagingLocation": self.staging_location,
                "user": self.user,
            }
        )

        subsetter = HarmonyAdapter(message, config=self.config)

        with self.assertRaises(Exception) as context_manager:
            subsetter.invoke()

        self.assertEqual(
            str(context_manager.exception),
            "No granules specified for trajectory subsetting.",
        )

        mock_mkdtemp.assert_not_called()
        mock_download.assert_not_called()
        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    def test_synchronous_multiple_granules(
        self,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure asynchronous request with more than one granule raises an
        exception.

        """
        message = Message(
            {
                "accessToken": self.access_token,
                "callback": self.callback,
                "isSynchronous": True,
                "sources": [
                    {
                        "collection": self.collection,
                        "granules": [self.granule, self.granule],
                        "shortName": self.shortname,
                    }
                ],
                "stagingLocation": self.staging_location,
                "user": self.user,
            }
        )

        subsetter = HarmonyAdapter(
            message, config=self.config, catalog=self.input_catalog
        )

        with self.assertRaises(Exception) as context_manager:
            subsetter.invoke()

        self.assertEqual(
            str(context_manager.exception),
            "Synchronous requests accept only one granule.",
        )

        mock_mkdtemp.assert_not_called()
        mock_download.assert_not_called()
        mock_get_mimetype.assert_not_called()
        mock_stage.assert_not_called()

    @patch("harmony_service.adapter.execute_command")
    def test_validation_temporal(
        self,
        mock_execute_command,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure a Harmony message containing temporal subset information
        only passes validation if it has both a start and end time for the
        temporal range.

        """
        mock_download.return_value = self.granule["url"]
        mock_get_mimetype.return_value = self.mimetype
        mock_stage.return_value = "tests/to/staged/output"
        mock_support_variables.side_effect = self.side_effect_fxn

        start_time = "2001-02-03T04:05:06"
        end_time = "2002-03-04T05:06:07"

        test_failure_args = [
            ["No temporal end", start_time, None],
            ["No temporal start", None, end_time],
        ]

        for description, temporal_start, temporal_end in test_failure_args:
            with self.subTest(description):
                message = Message(
                    {
                        "accessToken": self.access_token,
                        "callback": self.callback,
                        "sources": [
                            {
                                "collection": self.collection,
                                "granules": [self.granule],
                                "shortName": self.shortname,
                            }
                        ],
                        "stagingLocation": self.staging_location,
                        "temporal": {"start": temporal_start, "end": temporal_end},
                        "user": self.user,
                    }
                )

                subsetter = HarmonyAdapter(
                    message, config=self.config, catalog=self.input_catalog
                )

                with self.assertRaises(Exception) as context_manager:
                    subsetter.invoke()

                self.assertEqual(
                    str(context_manager.exception),
                    "Invalid temporal range, both start and end required.",
                )

                mock_mkdtemp.assert_not_called()
                mock_download.assert_not_called()
                mock_execute_command.assert_not_called()
                mock_get_mimetype.assert_not_called()
                mock_stage.assert_not_called()

        with self.subTest("Temporal subset with both start and end"):
            message = Message(
                {
                    "accessToken": self.access_token,
                    "callback": self.callback,
                    "sources": [
                        {
                            "collection": self.collection,
                            "granules": [self.granule],
                            "shortName": self.shortname,
                        }
                    ],
                    "stagingLocation": self.staging_location,
                    "temporal": {"start": start_time, "end": end_time},
                    "user": self.user,
                }
            )

            subsetter = HarmonyAdapter(
                message, config=self.config, catalog=self.input_catalog
            )
            subsetter.invoke()

            expected_local_out = f"{self.temp_dir}/{self.subsetted_filename}"
            expected_command = (
                f"{SUBSETTER_BINARY_PATH} "
                f"--configfile {SUBSETTER_CONFIG} "
                f"--filename {self.granule['url']} "
                f"--start {start_time} "
                f"--end {end_time} "
                f"--shortname {self.shortname} "
                f"--loglevel {self.loglevel} "
                f"--outfile {expected_local_out}"
            )

            mock_mkdtemp.assert_called_once()
            mock_execute_command.assert_called_once_with(
                expected_command, subsetter.logger
            )
            mock_stage.assert_called_once_with(
                expected_local_out,
                self.subsetted_filename,
                self.mimetype,
                location=self.staging_location,
                logger=subsetter.logger,
            )
            mock_get_mimetype.assert_called_once_with(expected_local_out)

    @patch("harmony_service.adapter.execute_command")
    def test_validation_shapefile(
        self,
        mock_execute_command,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure only a shape file with the correct MIME type will pass
        validation. Any non-GeoJSON shape file should raise an exception.

        """
        mock_download.side_effect = [self.granule["url"], self.local_shape_path]
        mock_get_mimetype.return_value = self.mimetype
        mock_stage.return_value = "tests/to/staged/output"
        mock_support_variables.side_effect = self.side_effect_fxn

        with self.subTest("Non GeoJSON shape file"):
            message = Message(
                {
                    "accessToken": self.access_token,
                    "callback": self.callback,
                    "sources": [
                        {
                            "collection": self.collection,
                            "granules": [self.granule],
                            "shortName": self.shortname,
                        }
                    ],
                    "stagingLocation": self.staging_location,
                    "subset": {
                        "shape": {"href": "url.notgeojson", "type": "other MIME type"}
                    },
                    "user": self.user,
                }
            )

            subsetter = HarmonyAdapter(
                message, config=self.config, catalog=self.input_catalog
            )

            with self.assertRaises(Exception) as context_manager:
                subsetter.invoke()

            self.assertEqual(
                str(context_manager.exception),
                "Invalid shape file format. Must be GeoJSON.",
            )

            mock_mkdtemp.assert_not_called()
            mock_download.assert_not_called()
            mock_execute_command.assert_not_called()
            mock_get_mimetype.assert_not_called()
            mock_stage.assert_not_called()

        with self.subTest("GeoJSON shape file"):
            message = Message(
                {
                    "accessToken": self.access_token,
                    "callback": self.callback,
                    "sources": [
                        {
                            "collection": self.collection,
                            "granules": [self.granule],
                            "shortName": self.shortname,
                        }
                    ],
                    "stagingLocation": self.staging_location,
                    "subset": {
                        "shape": {
                            "href": "url.geo.json",
                            "type": "application/geo+json",
                        }
                    },
                    "user": self.user,
                }
            )

            subsetter = HarmonyAdapter(
                message, config=self.config, catalog=self.input_catalog
            )
            subsetter.invoke()

            expected_local_out = f"{self.temp_dir}/{self.subsetted_filename}"
            expected_command = (
                f"{SUBSETTER_BINARY_PATH} "
                f"--configfile {SUBSETTER_CONFIG} "
                f"--filename {self.granule['url']} "
                f"--boundingshape {self.bounding_shape} "
                f"--shortname {self.shortname} "
                f"--loglevel {self.loglevel} "
                f"--outfile {expected_local_out}"
            )

            mock_mkdtemp.assert_called_once()
            mock_execute_command.assert_called_once_with(
                expected_command, subsetter.logger
            )
            mock_stage.assert_called_once_with(
                expected_local_out,
                self.subsetted_filename,
                self.mimetype,
                location=self.staging_location,
                logger=subsetter.logger,
            )
            mock_get_mimetype.assert_called_once_with(expected_local_out)

    def test_parse_binary_parameters(
        self,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure Harmony messages for the Trajectory Subsetter that specify
        a variety of transformations (spatial, temporal, variable)
        correctly translate the message parameters into a dictionary of
        variables to be given to the Trajectory Subsetter binary file.

        """
        base_source = {
            "collection": self.collection,
            "granules": [self.granule],
            "variables": [],
            "shortName": self.shortname,
        }

        base_message = {
            "accessToken": self.access_token,
            "callback": self.callback,
            "stagingLocation": self.staging_location,
            "user": self.user,
        }

        asset = Asset(href=self.granule["url"])
        shape_file_url = "https://shape.com/shape.geo.json"
        start_time = "2021-02-03T04:05:06"
        end_time = "2021-02-03T05:06:07"
        source_variables = [
            {"fullPath": "var_one", "id": "V1234-EEDTEST", "name": "var_one"},
            {"fullPath": "/nested/var_two", "id": "V2345-EEDTEST", "name": "var_two"},
        ]

        # First result is input granule, second is a GeoJSON shape file.
        local_input_path = f"{self.temp_dir}/random_uuid.h5"
        download_list = [local_input_path, self.local_shape_path]

        with self.subTest("No variables, temporal, bbox or polygon"):
            mock_download.side_effect = download_list
            expected_parameters = {
                "--configfile": SUBSETTER_CONFIG,
                "--filename": local_input_path,
                "--shortname": self.shortname,
                "--loglevel": self.loglevel,
                "--outfile": f"{self.temp_dir}/{self.granule['url']}",
            }
            message_content = base_message.copy()
            message_content["sources"] = [base_source]
            message = Message(message_content)
            source = Source(base_source)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            mock_download.assert_called_once_with(
                asset.href,
                self.temp_dir,
                logger=subsetter.logger,
                access_token=self.access_token,
                cfg=self.config,
            )
            mock_download.reset_mock()

        with self.subTest("Variable subset"):
            mock_download.side_effect = download_list

            expected_parameters = {
                "--configfile": SUBSETTER_CONFIG,
                "--filename": local_input_path,
                "--includedataset": join(self.temp_dir, "source_vars.json"),
                "--shortname": self.shortname,
                "--loglevel": self.loglevel,
                "--outfile": f"{self.temp_dir}/{self.subsetted_filename}",
            }
            message_content = base_message.copy()
            source_content = base_source.copy()
            source_content["variables"] = source_variables
            mock_support_variables.return_value = ["var_one", "/nested/var_two"]

            message_content["sources"] = [source_content]
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            source_vars_filename = join(self.temp_dir, "source_vars.json")
            with open(source_vars_filename, "r", encoding="utf-8") as var_file:
                self.assertEqual(
                    var_file.read(),
                    '{"data_set_layers": ["var_one", "/nested/var_two"]}',
                )

            self.assertDictEqual(binary_parameters, expected_parameters)
            mock_download.assert_called_once_with(
                asset.href,
                self.temp_dir,
                logger=subsetter.logger,
                access_token=self.access_token,
                cfg=self.config,
            )
            mock_download.reset_mock()

        with self.subTest("Bounding box spatial subset"):
            mock_download.side_effect = download_list
            expected_parameters = {
                "--bbox": "10,20,30,40",
                "--configfile": SUBSETTER_CONFIG,
                "--filename": local_input_path,
                "--shortname": self.shortname,
                "--loglevel": self.loglevel,
                "--outfile": f"{self.temp_dir}/{self.subsetted_filename}",
            }
            message_content = base_message.copy()
            message_content.update(
                {"sources": [base_source], "subset": {"bbox": [10, 20, 30, 40]}}
            )
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            mock_download.assert_called_once_with(
                asset.href,
                self.temp_dir,
                logger=subsetter.logger,
                access_token=self.access_token,
                cfg=self.config,
            )
            mock_download.reset_mock()

        with self.subTest("Polygon spatial subset"):
            mock_download.side_effect = download_list
            expected_parameters = {
                "--boundingshape": self.bounding_shape,
                "--configfile": SUBSETTER_CONFIG,
                "--filename": local_input_path,
                "--shortname": self.shortname,
                "--loglevel": self.loglevel,
                "--outfile": f"{self.temp_dir}/{self.subsetted_filename}",
            }
            message_content = base_message.copy()
            message_content.update(
                {
                    "sources": [base_source],
                    "subset": {
                        "shape": {
                            "href": shape_file_url,
                            "type": "application/geo+json",
                        }
                    },
                }
            )
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            self.assertEqual(mock_download.call_count, 2)
            mock_download.assert_any_call(
                asset.href,
                self.temp_dir,
                logger=subsetter.logger,
                access_token=self.access_token,
                cfg=self.config,
            )
            mock_download.assert_any_call(
                shape_file_url,
                self.temp_dir,
                logger=subsetter.logger,
                access_token=self.access_token,
                cfg=self.config,
            )
            mock_download.reset_mock()

        with self.subTest("Temporal subset"):
            mock_download.side_effect = download_list
            expected_parameters = {
                "--configfile": SUBSETTER_CONFIG,
                "--end": end_time,
                "--filename": local_input_path,
                "--shortname": self.shortname,
                "--loglevel": self.loglevel,
                "--outfile": f"{self.temp_dir}/{self.subsetted_filename}",
                "--start": start_time,
            }
            message_content = base_message.copy()
            message_content.update(
                {
                    "sources": [base_source],
                    "temporal": {"start": start_time, "end": end_time},
                }
            )
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            mock_download.assert_called_once_with(
                asset.href,
                self.temp_dir,
                logger=subsetter.logger,
                access_token=self.access_token,
                cfg=self.config,
            )
            mock_download.reset_mock()

        with self.subTest("Variables, bounding box, polygon and temporal"):
            mock_download.side_effect = download_list
            expected_parameters = {
                "--bbox": "10,20,30,40",
                "--boundingshape": self.bounding_shape,
                "--configfile": SUBSETTER_CONFIG,
                "--end": end_time,
                "--filename": local_input_path,
                "--includedataset": join(self.temp_dir, "source_vars.json"),
                "--shortname": self.shortname,
                "--loglevel": self.loglevel,
                "--outfile": f"{self.temp_dir}/{self.subsetted_filename}",
                "--start": start_time,
            }
            source_content = base_source.copy()
            source_content["variables"] = source_variables

            message_content["sources"] = [source_content]
            message_content = base_message.copy()
            message_content.update(
                {
                    "sources": [source_content],
                    "subset": {
                        "bbox": [10, 20, 30, 40],
                        "shape": {
                            "href": shape_file_url,
                            "type": "application/geo+json",
                        },
                    },
                    "temporal": {"start": start_time, "end": end_time},
                }
            )
            message = Message(message_content)
            source = Source(source_content)

            subsetter = HarmonyAdapter(message, config=self.config)
            binary_parameters = subsetter.parse_binary_parameters(
                self.temp_dir, asset, source
            )
            self.assertDictEqual(binary_parameters, expected_parameters)
            self.assertEqual(mock_download.call_count, 2)
            mock_download.assert_any_call(
                asset.href,
                self.temp_dir,
                logger=subsetter.logger,
                access_token=self.access_token,
                cfg=self.config,
            )
            mock_download.assert_any_call(
                shape_file_url,
                self.temp_dir,
                logger=subsetter.logger,
                access_token=self.access_token,
                cfg=self.config,
            )
            mock_download.reset_mock()

    @patch("harmony_service.adapter.execute_command")
    def test_transform(
        self,
        mock_execute_command,
        mock_download,
        mock_stage,
        mock_get_mime_type,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure that a command to invoke the binary subsetter is correctly
        constructed and passed to the
        `harmony_service.utilities.execute_command` function.

        """
        binary_parameters = {"--par_one": "val_one", "--par_two": "val_two"}

        expected_command = (
            f"{SUBSETTER_BINARY_PATH} --par_one val_one --par_two val_two"
        )

        message = Message(
            {
                "accessToken": self.access_token,
                "callback": self.callback,
                "sources": [
                    {
                        "collection": self.collection,
                        "granules": [self.granule],
                        "shortName": self.shortname,
                    }
                ],
                "stagingLocation": self.staging_location,
                "user": self.user,
            }
        )

        subsetter = HarmonyAdapter(message, config=self.config)
        subsetter.transform(binary_parameters)

        mock_execute_command.assert_called_once_with(expected_command, subsetter.logger)

    @patch("harmony_service.adapter.run_cli")
    def test_main(
        self,
        mock_run_cli,
        mock_download,
        mock_stage,
        mock_get_mimetype,
        mock_mkdtemp,
        mock_support_variables,
    ):
        """Ensure expected arguments can be parsed, and that unexpected
        arguments result in an exception being raised.

        """
        with self.subTest("Parsing known arguments"):
            known_arguments = [
                "--harmony-action",
                "invoke",
                "--harmony-input",
                '{"accessToken": "..."}',
            ]
            main(known_arguments, config=self.config)
            namespace = mock_run_cli.call_args[0][1]
            self.assertEqual(namespace.harmony_action, "invoke")
            self.assertEqual(namespace.harmony_input, '{"accessToken": "..."}')
            self.assertIn("cfg", mock_run_cli.call_args[1])
            self.assertEqual(mock_run_cli.call_args[1]["cfg"], self.config)

        mock_run_cli.reset_mock()

        with self.subTest("Ignoring unexpected arguments"):
            unknown_arguments = ["--something_wicked", "this way comes"]
            main(unknown_arguments)
            namespace = mock_run_cli.call_args[0][1]
            self.assertIsNone(namespace.harmony_action)
            self.assertIsNone(namespace.harmony_input)
            self.assertFalse(hasattr(namespace, "something_wicked"))
            self.assertIn("cfg", mock_run_cli.call_args[1])
            self.assertIsNone(mock_run_cli.call_args[1]["cfg"])

        mock_run_cli.reset_mock()
