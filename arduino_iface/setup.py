import os.path
from _project import NAME, VERSION, LICENSE, DESCRIPTION, URL, PACKAGES, REQUIRES, BINARIES
from setuptools import setup

setup(
    name = NAME,
    version = VERSION,
    license = LICENSE,
    description = DESCRIPTION,
    url = URL,
    scripts= BINARIES,
    packages = PACKAGES,
    install_requires = REQUIRES,
)
