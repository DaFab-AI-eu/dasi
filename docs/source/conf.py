# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

import sys
from pathlib import Path

sys.path.insert(0, str(Path('..', '..', 'pydasi', 'src').resolve()))

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'DASI'
copyright = '2024, European Centre for Medium-Range Weather Forecasts (ECMWF)'
author = 'ECMWF'
release = "0.2.6"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.duration',
    'sphinx.ext.doctest',
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.intersphinx',
    'sphinx_rtd_theme',
]

autosummary_generate = True

autodoc_mock_imports = ['dasi']

intersphinx_mapping = {
    'dasi': ('https://dasi.readthedocs.io/en/latest/', None),
    'rtd': ('https://docs.readthedocs.io/en/stable/', None),
    'python': ('https://docs.python.org/3/', None),
    'sphinx': ('https://www.sphinx-doc.org/en/master/', None),
}
intersphinx_disabled_domains = ['std', 'cffi']

templates_path = ['_templates']
exclude_patterns = ["build", "Thumbs.db", ".DS_Store", ".venv", ".tox"]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_title = 'DASI'
html_static_path = ['_static']

html_show_sphinx = False
todo_include_todos = False
