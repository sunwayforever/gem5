// 2023-08-29 10:48
#ifndef GOODBYE_OBJECT_H
#define GOODBYE_OBJECT_H

#include "params/GoodbyeObject.hh"
#include "sim/sim_object.hh"

namespace gem5 {
class GoodbyeObject : public SimObject {
   private:
    void processEvent();
    EventFunctionWrapper event;
    float bandwidth;
    int bufferSize;
    int bufferUsed;

   public:
    GoodbyeObject(const GoodbyeObjectParams &p);
    void goodbye();
};

}  // namespace gem5

#endif  // GOODBYE_OBJECT_H
