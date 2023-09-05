#include "toy/hello/hello_object.hh"

#include <iostream>

#include "base/trace.hh"
#include "debug/Toy.hh"

namespace gem5 {
HelloObject::HelloObject(const HelloObjectParams &p)
    : SimObject(p),
      event([this] { processEvent(); }, name()),
      latency(p.latency),
      times(p.times),
      goodbye(p.goodbye) {
    DPRINTF(Toy, "init hello object\n");
}

void HelloObject::processEvent() {
    times--;
    DPRINTF(Toy, "hello event: %d\n", times);
    if (times == 0) {
        DPRINTF(Toy, "hello event: done\n");
        goodbye->goodbye();
    } else {
        schedule(event, curTick() + latency);
    }
}

void HelloObject::startup() { schedule(event, latency); }
}  // namespace gem5
