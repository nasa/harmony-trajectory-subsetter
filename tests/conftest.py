"""Set up fixtures for unit tests."""

import os
import tempfile
from typing import Dict

import h5py
import numpy as np
from pytest import fixture


@fixture(scope="function")
def sample_hdf5_file():
    """Create a temporary HDF5 file for testing."""
    with tempfile.NamedTemporaryFile(suffix=".h5", delete=False) as tmp_file:
        temp_filename = tmp_file.name

    # Create basic HDF5 file with some test data
    with h5py.File(temp_filename, "w") as f:
        f.create_dataset("dataset1", data=np.array([1, 2, 3]))
        f.create_dataset("dataset2", data=np.array([4, 5, 6]))
        f.attrs["attribute1"] = "attribute_value1"
        f.attrs["attribute2"] = "attribute_value2"

    yield temp_filename

    # Cleanup
    if os.path.exists(temp_filename):
        os.unlink(temp_filename)


@fixture(scope="function")
def sample_all_binary_parameters(sample_hdf5_file) -> Dict[str, str]:
    """Sample binary parameters for testing."""
    return {
        "--outfile": sample_hdf5_file,
        "--bbox": "10.0,20.0,30.0,40.0",
        "--start": "2000-01-02T03:04:05Z",
        "--end": "2000-02-02T03:04:05Z",
        "--boundingshape": '{"type": "Polygon", "coordinates": '
        "[[[1.0, 2.0], [3.0, 4.0], [-5.0, 6.0], [1.0, 2.0]]]}",
    }
