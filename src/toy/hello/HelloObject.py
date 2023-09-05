#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# 2023-08-29 10:40
from m5.params import *
from m5.SimObject import SimObject


class HelloObject(SimObject):
    type = "HelloObject"
    cxx_header = "toy/hello/hello_object.hh"
    cxx_class = "gem5::HelloObject"
    latency = Param.Latency("hello event latency")
    times = Param.Int(1, "hello event times")
    goodbye = Param.GoodbyeObject("goodbye object")


class GoodbyeObject(SimObject):
    type = "GoodbyeObject"
    cxx_header = "toy/hello/goodbye_object.hh"
    cxx_class = "gem5::GoodbyeObject"
    buffer_size = Param.MemorySize("1kB", "size of goodbye buffer")
    bandwidth = Param.MemoryBandwidth("100MB/s", "bandwith of goodbye buffer")
