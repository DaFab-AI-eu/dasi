# Copyright 2023 European Centre for Medium-Range Weather Forecasts (ECMWF)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import pytest
from utils import version


def test_version_format():
    """Ensure version follows PEP 440."""
    from packaging.version import Version
    assert Version(version.__version__)  # Should not raise


def test_version_compatibility():
    """Test version comparison logic."""
    assert version.is_compatible(version.__version__)  # Same
    assert version.is_compatible("0.3.0")  # Newer
    assert not version.is_compatible("0.1.0")  # Older should fail


def test_library_version_check(tmp_path):
    """Integration test for library version validation."""
    # TODO: Mock library with old version
    # Should raise CFFIModuleLoadFailed
    pass
