// 2023-08-29 10:48
#ifndef HELLO_OBJECT_H
#define HELLO_OBJECT_H

#include "toy/hello/goodbye_object.hh"
#include "params/HelloObject.hh"
#include "sim/sim_object.hh"

namespace gem5 {
class HelloObject : public SimObject {
   private:
    void processEvent();
    EventFunctionWrapper event;
    const Tick latency;
    int times;

    GoodbyeObject *goodbye;

   public:
    HelloObject(const HelloObjectParams &p);
    void startup() override;
};

}  // namespace gem5

#endif  // HELLO_OBJECT_H
