#!/usr/bin/env python3
import os
import sys

module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", ".."))

from data.base import get_version
from data.base import setup

setup(
    __file__,
    "styles",
    [
        "drules_proto.bin",
        "drules_proto_walking_light.bin",
        "drules_proto_walking_outdoor_light.bin",
        "drules_proto_walking_light.txt",
        "drules_proto_walking_outdoor_light.txt",
        "drules_proto_walking_dark.bin",
        "drules_proto_walking_outdoor_dark.bin",
        "drules_proto_walking_dark.txt",
        "drules_proto_walking_outdoor_dark.txt",
        "drules_proto_cycling_light.bin",
        "drules_proto_cycling_outdoor_light.bin",
        "drules_proto_cycling_light.txt",
        "drules_proto_cycling_outdoor_light.txt",
        "drules_proto_cycling_dark.bin",
        "drules_proto_cycling_outdoor_dark.bin",
        "drules_proto_cycling_dark.txt",
        "drules_proto_cycling_outdoor_dark.txt",
        "drules_proto_driving_light.bin",
        "drules_proto_driving_outdoor_light.bin",
        "drules_proto_driving_light.txt",
        "drules_proto_driving_outdoor_light.txt",
        "drules_proto_driving_dark.bin",
        "drules_proto_driving_outdoor_dark.bin",
        "drules_proto_driving_dark.txt",
        "drules_proto_driving_outdoor_dark.txt",
        "drules_proto_public-transport_light.bin",
        "drules_proto_public-transport_outdoor_light.bin",
        "drules_proto_public-transport_light.txt",
        "drules_proto_public-transport_outdoor_light.txt",
        "drules_proto_public-transport_dark.bin",
        "drules_proto_public-transport_outdoor_dark.bin",
        "drules_proto_public-transport_dark.txt",
        "drules_proto_public-transport_outdoor_dark.txt",
        "drules_proto_vehicle_light.bin",
        "drules_proto_vehicle_light.txt",
        "drules_proto_vehicle_dark.bin",
        "drules_proto_vehicle_dark.txt",
    ],
    install_requires=["omim-data-files=={}".format(get_version())]
)
