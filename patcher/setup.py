#!/usr/bin/env python3

from setuptools import setup, find_packages

setup(
    name='CRISPR',
    version='3.0',
    description='CRISPR patcher component',
    author='Filippo Cremonese (rev.ng SRLs)',
    author_email='filippocremonese@rev.ng',
    # TODO
    url='https://rev.ng/gitlab/',
    packages=find_packages(),
    package_data={"orchestra": ["support/*"]},
    install_requires=open("requirements.txt").readlines(),
    entry_points={
        "console_scripts": [
            "crispr=patcher:cmdline_main",
        ]
    },
    zip_safe=False,
)
