#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# 2023-08-29 11:12
import m5
from m5.objects import *

root = Root(full_system=False)

root.hello = HelloObject(latency="1ns", times=10)
root.hello.goodbye = GoodbyeObject()

m5.instantiate()
exit_event = m5.simulate()
print(f"exiting simulation at {m5.curTick()}, cause: {exit_event.getCause()}")
