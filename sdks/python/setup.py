from setuptools import setup, find_packages

setup(
    name="decenlicense",
    version="1.0.0",
    description="DecentriLicense Python Binding",
    packages=find_packages(),
    install_requires=[
        # Add any dependencies here
    ],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
)
