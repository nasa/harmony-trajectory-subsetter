"""Unit tests for history.py."""

import json
import os

from freezegun import freeze_time
import h5py

from harmony_service.history import (
    PROGRAM,
    get_semantic_version,
    get_subset_parameters,
    needs_history_metadata,
    update_history_metadata,
    write_history_attrs,
)


@freeze_time("2000-01-02T03:04:05")
def test_update_history_metadata_no_input_history(sample_hdf5_file):
    """No existing history attribute so one should be created."""
    binary_parameters = {"--outfile": sample_hdf5_file}
    variables = []

    # Invoke the function under test.
    update_history_metadata(binary_parameters, variables)

    with h5py.File(sample_hdf5_file) as output_file:
        # Check function output.
        assert "history" in output_file.attrs
        assert "History" not in output_file.attrs
        assert (
            output_file.attrs["history"]
            == f'2000-01-02T03:04:05+00:00 {PROGRAM} {get_semantic_version()} ["All variables."]'
        )


@freeze_time("2000-01-02T03:04:05")
def test_update_history_metadata_existing_history(sample_hdf5_file):
    """Attribute named `history` exists, new history should be appended."""
    binary_parameters = {"--outfile": sample_hdf5_file}
    variables = []

    # Create existing history attribute.
    with h5py.File(sample_hdf5_file, "r+") as output_file:
        # Adding existing history attribute.
        output_file.attrs["history"] = "old history"

    # Invoke function under test.
    update_history_metadata(binary_parameters, variables)

    # Check function output.
    with h5py.File(sample_hdf5_file, "r") as output_file:
        # Check function output.
        assert "history" in output_file.attrs
        assert "History" not in output_file.attrs
        assert (
            output_file.attrs["history"]
            == f"old history\n2000-01-02T03:04:05+00:00 {PROGRAM} "
            f'{get_semantic_version()} ["All variables."]'
        )


@freeze_time("2000-01-02T03:04:05")
def test_update_history_metadata_existing_history_uppercase(sample_hdf5_file):
    """Attribute named `History` exists, new history should be appended."""
    binary_parameters = {"--outfile": sample_hdf5_file}
    variables = []

    # Create existing history attribute.
    with h5py.File(sample_hdf5_file, "r+") as output_file:
        # Adding existing history attribute.
        output_file.attrs["History"] = "old history"

    # Invoke function under test.
    update_history_metadata(binary_parameters, variables)

    # Check function output.
    with h5py.File(sample_hdf5_file, "r") as output_file:
        # Check function output.
        assert "history" not in output_file.attrs
        assert "History" in output_file.attrs
        assert (
            output_file.attrs["History"]
            == f"old history\n2000-01-02T03:04:05+00:00 {PROGRAM} "
            f'{get_semantic_version()} ["All variables."]'
        )


def test_needs_history_metadata():
    """A history attribute should only be created for HDF5 and NetCDF4 files.

    If the file type is not recognized, it defaults to `.nc4`.
    """
    assert needs_history_metadata("file.nc4") is True
    assert needs_history_metadata("file.nc") is True
    assert needs_history_metadata("file.h5") is True
    assert needs_history_metadata("file.hdf5") is True
    assert needs_history_metadata("file.tif") is False
    assert needs_history_metadata("file.tiff") is False
    assert needs_history_metadata("file.unknown") is True


def test_get_semantic_version_found(tmp_path, monkeypatch):
    """Test successful reading of service version."""
    # Create temporary service_version.txt
    version_file = tmp_path / "docker" / "service_version.txt"
    version_file.parent.mkdir(parents=True, exist_ok=True)
    version_file.write_text("1.2.3")

    # Mock the current directory to point to tmp_path
    monkeypatch.setattr(os, "getcwd", lambda: str(tmp_path))

    result = get_semantic_version()
    assert result == "1.2.3"


def test_get_semantic_version_empty(tmp_path, monkeypatch):
    """Test successful reading of service version, but file is empty."""
    # Create temporary service_version.txt
    version_file = tmp_path / "docker" / "service_version.txt"
    version_file.parent.mkdir(parents=True, exist_ok=True)
    version_file.write_text("")

    # Mock the current directory to point to tmp_path
    monkeypatch.setattr(os, "getcwd", lambda: str(tmp_path))

    result = get_semantic_version()
    assert result == "[version not found]"


def test_write_history_attrs_with_parameters(
    mocker, sample_all_binary_parameters, sample_hdf5_file
):
    """Ensure history attribute includes all subset parameters."""
    mock_get_updated_history_metadata = mocker.patch(
        "harmony_service.history.get_updated_history_metadata",
        return_value="Time Service Version",
    )
    mock_get_subset_parameters = mocker.patch(
        "harmony_service.history.get_subset_parameters",
        return_value='[["1,2,3,4"], "variable1,variable2"]',
    )
    variables = ["variable1, variable2"]
    expected_history_attribute = (
        f"{mock_get_updated_history_metadata.return_value} "
        f"{mock_get_subset_parameters.return_value}"
    )

    # Invoke function under test.
    with h5py.File(sample_hdf5_file, "r+") as output_file:
        write_history_attrs(
            output_file, sample_all_binary_parameters, "history", "", variables
        )

        # Checkout function output.
        mock_get_updated_history_metadata.assert_called_once()
        mock_get_subset_parameters.assert_called_once()
        assert output_file.attrs["history"] == expected_history_attribute


def test_write_history_attrs_no_parameters(mocker, sample_hdf5_file):
    """Ensure that no subset parameters beyond are written to the history attribute when none are given.

    Only the time, service name, and service version should be included,
    along with an indication that all variables are to be returned.

    """
    mock_get_updated_history_metadata = mocker.patch(
        "harmony_service.history.get_updated_history_metadata",
        return_value="Time Service Version",
    )
    expected_history_attribute = (
        f'{mock_get_updated_history_metadata.return_value} ["All variables."]'
    )

    # Invoke function under test.
    with h5py.File(sample_hdf5_file, "r+") as output_file:
        write_history_attrs(
            output_file,
            {},  # No binary parameters
            "history",  # Attribute name
            "",  # No existing history
            [],
        )  # No requested variable

        # Checkout function output.
        mock_get_updated_history_metadata.assert_called_once()
        assert output_file.attrs["history"] == expected_history_attribute


def test_get_subset_parameters_all(sample_all_binary_parameters):
    """Ensure that all subset parameters are returned when all are requested."""
    variables = ["variable1", "variable2"]
    binary_parameters_list = [
        f"{sample_all_binary_parameters['--bbox']}",
        json.loads(sample_all_binary_parameters["--boundingshape"]),
        f"{sample_all_binary_parameters['--start']}, "
        f"{sample_all_binary_parameters['--end']}",
        f"{variables[0]},{variables[1]}",
    ]
    expected_output = json.dumps(binary_parameters_list)

    # Invoke the function under test.
    parameters = get_subset_parameters(sample_all_binary_parameters, variables)
    assert parameters == expected_output


def test_get_subset_parameters_all_variables():
    """Ensure all variables are returned when there are no input parameters."""
    variables = []
    binary_parameters = {}

    expected_output = '["All variables."]'

    # Invoke the function under test.
    parameters = get_subset_parameters(binary_parameters, variables)
    assert parameters == expected_output
