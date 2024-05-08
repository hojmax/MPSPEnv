from setuptools import setup, Extension
import glob

ext_modules = Extension(
    "MPSPEnv.c_lib",
    sources=glob.glob("MPSPEnv/c/src/*.c"),
    extra_compile_args=["-O3", "-DNDEBUG"],
    extra_link_args=["-O3", "-DNDEBUG"],
    include_dirs=["MPSPEnv/c/src"],
    language="c",
)

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="MPSPEnv",
    version="0.0.34",
    author="Axel Højmark",
    author_email="axelhojmark@gmail.com",
    description="A reinforcement learning environment for the Multi Port Stowage Planning problem",
    long_description=long_description,
    long_description_content_type="text/markdown",
    ext_modules=[ext_modules],
    packages=["MPSPEnv"],
    project_urls={
        "Repository": "https://github.com/hojmax/MPSPEnv",
    },
    install_requires=["pygame", "gymnasium", "numpy", "seaborn"],
)
