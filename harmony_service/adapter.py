""" This module contains the main HarmonyAdapter instance for calling the
    Data Services Level 2 Segmented Trajectory Subsetter. It will:

    * Receive an inbound Harmony message.
    * Validate and parse the contents of that Harmony message.
    * Retrieve a copy of the granule to be processed and associated shape file,
      if a shape file is specified.
    * Convert the Harmony message parameters into arguments compatible with the
      C++ binary for the Trajectory Subsetter.
    * Call the Trajectory Subsetter binary.
    * Receive the output file path from the Trajectory Subsetter binary.
    * Stage the output file via Harmony functionality to allow the end-user to
      retrieve their results.

"""
from argparse import ArgumentParser
from shutil import rmtree
from sys import argv
from tempfile import mkdtemp
from typing import Any, List, Optional

from harmony import BaseHarmonyAdapter, run_cli, setup_cli
from harmony.message import Source
from harmony.util import (Config, download, generate_output_filename,
                          HarmonyException, stage)
from pystac import Asset, Item

from harmony_service.utilities import get_file_mimetype


class HarmonyAdapter(BaseHarmonyAdapter):
    """ This class extends the BaseHarmonyAdapter class, to implement the
        `invoke` method, which performs L2 segmented trajectory subsetting.

    """
    def invoke(self):
        """ Adds validation to default process_item-based invocation. """
        self.validate_message()
        return super().invoke()

    def process_item(self, item: Item, source: Source):
        """ This method performs the standard Harmony methodology to process
            an input granule:

            * Retrieve the source granule.
            * Extract relevant parameters from the input Harmony message.
            * Invoke the transformation service.
            * Stage the results.
            * Create an updated STAC record and return it.

        """
        result = item.clone()
        result.asserts = {}

        working_directory = mkdtemp()

        try:
            asset = next(stac_asset
                         for stac_asset in item.assets.values()
                         if 'data' in (stac_asset.roles or []))

            input_filename = download(asset.href,
                                      working_directory,
                                      logger=self.logger,
                                      access_token=self.message.accessToken,
                                      cfg=self.config)

            # DAS-1262 - Parse request parameters from the Harmony message here

            output_file_path = self.transform(input_filename)

            # DAS-1262 - include requested variable, etc, when naming file
            staged_file_name = generate_output_filename(asset.href)

            # stage the output results
            mime, _ = get_file_mimetype(output_file_path)
            url = stage(output_file_path, staged_file_name, mime,
                        location=self.message.stagingLocation,
                        logger=self.logger)

            # Update the STAC record and return it.
            result.assets['data'] = Asset(url, title=staged_file_name,
                                          media_type=mime, roles=['data'])

            return result
        except Exception as exception:
            raise HarmonyException('L2 Trajectory Subsetter failed with '
                                   f'error: {str(exception)}') from exception
        finally:
            # Clean up any intermediate resources:
            rmtree(working_directory)

    def transform(self, input_file_path: str) -> str:
        """ DAS-1263 - This class method will contain a call to the trajectory
            subsetter.

        """
        return input_file_path

    def validate_message(self):
        """ Check the service was triggered by a valid message.
            DAS-1262 - add validation for specific options, if needed.

        """
        has_granules = (hasattr(self.message, 'granules') and
                        bool(self.message.granules))

        try:
            has_items = bool(self.catalog and
                             next(self.catalog.get_all_items()))
        except StopIteration:
            has_items = False

        if not has_granules and not has_items:
            raise Exception('No granules specified for trajectory subsetting.')
        elif self.message.isSynchronous and len(self.message.granules) > 1:
            raise Exception('Synchronous requests accept only one granule.')


def main(arguments: List[Any], config: Optional[Config] = None):
    """ Parse command line arguments and invoke the Harmony service. The known
        arguments are defined in `harmony-service-lib-py`, under:
        `harmony.cli.setup_cli`.

    """
    parser = ArgumentParser(
        prog='L2 Segmented Trajectory Subsetter',
        description='Run the Data Services L2 Segmented Trajectory Subsetter'
    )
    setup_cli(parser)
    args, _ = parser.parse_known_args(arguments)
    run_cli(parser, args, HarmonyAdapter, cfg=config)


if __name__ == '__main__':
    main(argv[1:])
