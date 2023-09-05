#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# 2023-08-31 16:26
from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject


class SimpleCache(SimObject):
    type = "SimpleCache"
    cxx_header = "toy/hello_cache/simple_cache.hh"
    cxx_class = "gem5::SimpleCache"

    cpu_side = SlavePort("cpu side port")
    mem_side = MasterPort("mem side port")

    latency = Param.Cycles(1, "cache latency")
