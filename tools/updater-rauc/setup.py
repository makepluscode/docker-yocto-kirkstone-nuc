"""Setup script for RAUC Updater."""

from setuptools import setup, find_packages
from pathlib import Path

# Read long description from README
long_description = (Path(__file__).parent / "README.md").read_text(encoding="utf-8")

setup(
    name="rauc-updater",
    version="0.1.0",
    description="RAUC Update Tool with Qt6 QML GUI",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="RAUC Team",
    author_email="support@example.com",
    url="https://github.com/rauc/rauc-updater",
    license="MIT",
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Operating System :: POSIX :: Linux",
        "Topic :: System :: Systems Administration",
        "Topic :: System :: Networking",
    ],
    keywords="rauc ota embedded update",
    package_dir={"": "src"},
    packages=find_packages(where="src"),
    python_requires=">=3.9",
    install_requires=[
        "paramiko>=3.0.0",
        "click>=8.0.0", 
        "rich>=13.0.0",
        "pydantic>=2.0.0",
    ],
    extras_require={
        "gui": ["PyQt6>=6.5.0"],
        "advanced": [
            "babel>=2.12.0",
            "watchdog>=3.0.0", 
            "keyring>=24.0.0",
        ],
        "dev": [
            "pytest>=7.0.0",
            "black>=23.0.0",
            "isort>=5.0.0",
            "mypy>=1.0.0",
        ],
    },
    entry_points={
        "console_scripts": [
            "rauc-updater=rauc_updater.cli:main",
            "rauc-updater-gui=rauc_updater.gui:main",
        ],
    },
    include_package_data=True,
    package_data={
        "rauc_updater.gui": ["qml/*.qml"],
    },
    zip_safe=False,
)