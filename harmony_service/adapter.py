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
from itertools import chain
from os.path import basename, join as join_path
from shutil import rmtree
from sys import argv
from tempfile import mkdtemp
from typing import Any, Dict, List, Optional

from harmony import BaseHarmonyAdapter, run_cli, setup_cli
from harmony.message import Source
from harmony.util import (Config, download, generate_output_filename,
                          HarmonyException, stage)
from pystac import Asset, Item

from harmony_service.utilities import (execute_command, get_file_mimetype,
                                       is_bbox_spatial_subset,
                                       is_harmony_subset,
                                       is_polygon_spatial_subset,
                                       is_temporal_subset)


# DAS-1263: Updated path to binary to correct value
SUBSETTER_BINARY_PATH = 'subset'
SUBSETTER_CONFIG = 'harmony_service/subsetter_config.json'


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

            binary_parameters = self.parse_binary_parameters(working_directory,
                                                             asset, source)

            # DAS-1263 - Uncomment the following line. It will need extensive
            # local testing once the Docker image is being built to contain the
            # subsetter binary.
            # self.transform(binary_parameters)

            staged_file_name = basename(binary_parameters['--outfile'])

            # stage the output results
            mime = get_file_mimetype(binary_parameters['--outfile'])
            url = stage(binary_parameters['--outfile'], staged_file_name, mime,
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

    def transform(self, binary_parameters: Dict[str, str]) -> None:
        """ This class method takes the parsed arguments to be sent to the
            L2 segmented Trajectory Subsetter binary and constructs a command
            to execute the specified transformation. This command takes the
            form of a string, which is a space-delimited concatenation of the
            list of parameters:

            ```Python
            binary_command = ['path/to/binary', '--filename', 'input.h5']
            execute_command('path/to/binary --filename input.h5', self.logger)
            ```

            If the binary returns a non-zero exit status, this will cause a
            `CustomError` to be raised, which in turn will raise a
            `HarmonyException`.

        """
        binary_command = [SUBSETTER_BINARY_PATH]
        binary_command.extend(chain(*(binary_parameters.items())))
        execute_command(' '.join(binary_command), self.logger)

    def validate_message(self):
        """ Check the service was triggered by a valid message. This includes
            standard Harmony validation for the number of granules specified,
            as well as validation rules from the on-premises invocation.
            Currently that is limited to ensuring temporal subsetting
            specifies both a start and end time.

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

        if (
                is_temporal_subset(self.message) and
                (self.message.temporal.start is None
                 or self.message.temporal.end is None)
        ):
            raise Exception('Invalid temporal range, both start and end '
                            'required.')

        if (
                is_polygon_spatial_subset(self.message) and
                self.message.subset.shape.type != 'application/geo+json'
        ):
            raise Exception('Invalid shape file format. Must be GeoJSON.')

    def parse_binary_parameters(self, working_directory: str,
                                input_asset: Asset,
                                source: Source) -> Dict[str, str]:
        """ Retrieve parameters for the Trajectory Subsetter binary from the
            Harmony message. These will be stored in a dictionary, that
            formats the key as the binary parameter flag, e.g.:

            ```
            binary_parameters = {'--end': 'YYYY-MM-DDTHH:mm:ss'
                                 '--filename': '/path/to/local/file.nc',
                                 '--start': 'YYYY-MM-DDTHH:mm:ss'}
            ```

            Parameters that will always be included:

            * `--configfile` - the path to the subsetter configuration JSON
              file, as hosted in the Docker image. This value is hard-coded at
              the top of this module.
            * `--filename` - the local file path of the downloaded granule to
              be transformed.
            * `--outfile` - the local file path of the transformed output.

            Optional parameters:

            * `--includedataset` - A comma separated list of variables, if the
              request specifies a subset of variables to include in the output.
              Otherwise, all variables will be returned.
            * `--start` - The start time of a temporal range, in ISO-8601
              format. This can be a date or a datetime.
            * `--end` - The end time of a temporal range, in ISO-8601 format.
              This can be a date or a datetime. `--start` and `--end` must
              either both be provided, or both be omitted from a request.
            * `--bbox` - A bounding box of format "[W,S,E,N]".
            * `--boundingshape` - A local file path for a shape file to be used
              for polygon spatial subsetting.

            Other binary parameters currently not used:

            * `--crs` - Specifies a coordinate system to project the output,
              including EPSG codes.
            * `--reformat` - Specifies the output format, such a "GeoTIFF". A
              future feature.
            * `--subsettype` - Can be "ICESAT", "SMAP" or "GLAS". Omitted as
              not included in the `ICESat2ToolAdapter.py`.

        """
        binary_parameters = {
            '--configfile': SUBSETTER_CONFIG,
            '--filename': download(input_asset.href, working_directory,
                                   logger=self.logger,
                                   access_token=self.message.accessToken,
                                   cfg=self.config),
        }

        if source.variables != []:
            variables = [variable.fullPath
                         for variable in source.process('variables')]
            binary_parameters['--includedataset'] = ','.join(variables)
        else:
            variables = None

        if is_temporal_subset(self.message):
            binary_parameters['--start'] = self.message.temporal.start
            binary_parameters['--end'] = self.message.temporal.end

        if is_bbox_spatial_subset(self.message):
            binary_parameters['--bbox'] = ','.join(str(extent) for extent
                                                   in self.message.subset.bbox)

        if is_polygon_spatial_subset(self.message):
            binary_parameters['--boundingshape'] = download(
                self.message.subset.shape.href, working_directory,
                logger=self.logger, access_token=self.message.accessToken,
                cfg=self.config
            )

        binary_parameters['--outfile'] = join_path(
            working_directory,
            generate_output_filename(
                input_asset.href, variable_subset=variables,
                is_subsetted=is_harmony_subset(self.message)
            )
        )

        return binary_parameters


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
