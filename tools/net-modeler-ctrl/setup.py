from setuptools import setup, find_packages
import version
setup(
    name = "net-modeler-ctrl",
    version = version.version,
    package = find_packages(),

    author = "Chris Wacek",
    author_email = "cwacek@cs.georgetown.edu",
    description = "Userspace tools for loading and controlling net-modeler",
    license = "GPLv2",

    entry_points = {
      'console_scripts': [
           "net-modeler-ctrl = net_modeler:main",
         ]
    },
  )
