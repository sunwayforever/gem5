#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# 2023-08-31 16:26
from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject


class SimpleMemoryObject(SimObject):
    type = "SimpleMemoryObject"
    cxx_header = "toy/hello_memory/simple_memory_object.hh"
    cxx_class = "gem5::SimpleMemoryObject"

    iport = SlavePort("iport")
    dport = SlavePort("dport")
    memport = MasterPort("mem side port")
