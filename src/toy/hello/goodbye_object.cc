#include "toy/hello/goodbye_object.hh"

#include <iostream>

#include "base/trace.hh"
#include "debug/Toy.hh"
#include "sim/sim_exit.hh"

namespace gem5 {
GoodbyeObject::GoodbyeObject(const GoodbyeObjectParams &p)
    : SimObject(p),
      event([this] { processEvent(); }, name()),
      bandwidth(p.bandwidth),
      bufferSize(p.buffer_size),
      bufferUsed(0) {
    DPRINTF(Toy, "init goodbye object\n");
}

void GoodbyeObject::processEvent() {
    DPRINTF(Toy, "process event\n");
    bufferUsed += 100;
    if (bufferUsed >= bufferSize) {
        DPRINTF(Toy, "copy done\n");
        exitSimLoop("", 0, curTick() + bandwidth * 100);
    } else {
        schedule(event, curTick() + bandwidth * 100);
    }
}

void GoodbyeObject::goodbye() {
    DPRINTF(Toy, "goodbye");
    processEvent();
}
}  // namespace gem5
