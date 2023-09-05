// 2023-08-31 16:42
#ifndef SIMPLE_MEMORY_OBJECT_HH
#define SIMPLE_MEMORY_OBJECT_HH

#include "base/trace.hh"
#include "debug/Toy.hh"
#include "mem/port.hh"
#include "params/SimpleMemoryObject.hh"
#include "sim/sim_object.hh"
// cpu     ->   recvTimingReq(sendRetryReq)            -> iport
//         <-   sendTimingResp(recvRespRetry):cached   <-
//
// iport   ->   recvReq(sendRetryReq)                  -> owner
//         <-   sendResp
//
// owner   ->   sendTimingReq(recvReqRetry):cached     -> memport
//         <-   recvTimingResp
namespace gem5 {

class SimpleMemoryObject : public SimObject {
   private:
    class CPUSidePort : public SlavePort {
       private:
        SimpleMemoryObject *owner;
        bool retry;

       public:
        CPUSidePort(const std::string &if_name, SimpleMemoryObject *owner)
            : SlavePort(if_name, owner), owner(owner), retry(false) {}

        AddrRangeList getAddrRanges() const override {
            return owner->getAddrRanges();
        }

        // NOTE: a.6 cpuport send reponse to cpu, by sendTimingResp
        void sendPacket(PacketPtr pkt) {
            DPRINTF(Toy, "cpuport:sendPacket: 0x%x\n", pkt->getAddr());
            if (!sendTimingResp(pkt)) {
                DPRINTF(
                    Toy, "cpuport:sendPacket:blocked: 0x%x\n", pkt->getAddr());
                owner->blockedPkt = pkt;
            }
        }

        // NOTE: b.3 memobj ask cpuport to retry blocked `b`, hence `c` emerged
        void sendRetryPacket() {
            if (retry && owner->blockedPkt == nullptr) {
                retry = false;
                sendRetryReq();
            }
        }

       protected:
        Tick recvAtomic(PacketPtr pkt) override { return curTick(); };
        void recvFunctional(PacketPtr pkt) override {
            owner->handleFunctional(pkt);
        }
        // NOTE: a.1 request from cpu
        // NOTE: b.1 another request from cpu
        // NOTE: c.1 restarted request from cpu (of b.1)
        bool recvTimingReq(PacketPtr pkt) override {
            DPRINTF(Toy, "cpuport:recvTimingReq: 0x%x\n", pkt->getAddr());
            if (!owner->recvReq(pkt)) {
                DPRINTF(
                    Toy, "cpu port:recvTimingReq:blocked: 0x%x\n",
                    pkt->getAddr());
                retry = true;
                return false;
            }
            return true;
        }

        void recvRespRetry() override {
            assert(owner->blockedPkt != nullptr);
            PacketPtr pkt = owner->blockedPkt;
            owner->blockedPkt = nullptr;
            sendPacket(pkt);
        }
    };

    class MemSidePort : public MasterPort {
       private:
        SimpleMemoryObject *owner;

       public:
        MemSidePort(const std::string &if_name, SimpleMemoryObject *owner)
            : MasterPort(if_name, owner), owner(owner) {}

        // NOTE: a.3 memport handle request from memobj, sendTimingReq to membus
        // NOTE: now another request of `b` emerged
        void sendPacket(PacketPtr pkt) {
            DPRINTF(Toy, "memport:sendPacket: 0x%x\n", pkt->getAddr());
            if (!sendTimingReq(pkt)) {
                DPRINTF(
                    Toy, "memport:sendPacket:blocked: 0x%x\n", pkt->getAddr());
                owner->blockedPkt = pkt;
            }
        }

       protected:
        // NOTE: a.4 memport receive response from membus
        bool recvTimingResp(PacketPtr pkt) override {
            DPRINTF(Toy, "memport:recvTimingResp: 0x%x\n", pkt->getAddr());
            owner->sendResp(pkt);
            return true;
        }
        void recvReqRetry() override {
            assert(owner->blockedPkt != nullptr);
            PacketPtr pkt = owner->blockedPkt;
            owner->blockedPkt = nullptr;
            sendPacket(pkt);
        }
        void recvRangeChange() override { owner->handleRangeChange(); }
    };

    CPUSidePort iport, dport;
    MemSidePort memport;
    bool blocked;
    EventFunctionWrapper delay;

   public:
    PacketPtr blockedPkt;
    PacketPtr curPkt;
    SimpleMemoryObject(const SimpleMemoryObjectParams &p)
        : SimObject(p),
          iport("iport", this),
          dport("dport", this),
          memport("memport", this),
          blocked(false),
          delay([this] { sendPacketDelayed(); }, "hello"),
          blockedPkt(nullptr),
          curPkt(nullptr) {
        DPRINTF(Toy, "simple memory object init\n");
    }

    Port &getPort(const std::string &if_name, PortID idx) {
        if (if_name == "memport") {
            return memport;
        } else if (if_name == "iport") {
            return iport;
        } else if (if_name == "dport") {
            return dport;
        } else {
            return SimObject::getPort(if_name, idx);
        }
    }

    void handleFunctional(PacketPtr pkt) { memport.sendFunctional(pkt); }
    void handleRangeChange() {
        iport.sendRangeChange();
        dport.sendRangeChange();
    }
    // NOTE: a.2 memobj handles request from cpuport
    // NOTE: b.2 memobj blocked `b`
    void sendPacketDelayed() { memport.sendPacket(curPkt); }
    bool recvReq(PacketPtr pkt) {
        DPRINTF(Toy, "main:recvReq: 0x%x\n", pkt->getAddr());
        if (blocked) {
            DPRINTF(Toy, "main:recvReq:blocked: 0x%x\n", pkt->getAddr());
            return false;
        }
        DPRINTF(Toy, "main:recvReq:go: 0x%x\n", pkt->getAddr());
        blocked = true;
        curPkt = pkt;
        schedule(delay, curTick() + 1000);
        // memport.sendPacket(pkt);
        return true;
    }

    void sendRetryReq() {
        iport.sendRetryPacket();
        dport.sendRetryPacket();
    }
    // NOTE: a.5 memobj receive reponse from memport, send it to cpuport
    void sendResp(PacketPtr pkt) {
        DPRINTF(Toy, "main:sendResp: 0x%x\n", pkt->getAddr());
        assert(blocked);
        blocked = false;
        if (pkt->req->isInstFetch()) {
            iport.sendPacket(pkt);
        } else {
            dport.sendPacket(pkt);
        }
        sendRetryReq();
    }
    AddrRangeList getAddrRanges() const {
        DPRINTF(Toy, "get addr range");
        return memport.getAddrRanges();
    }
};
}  // namespace gem5

#endif  // SIMPLE_MEMORY_OBJECT_HH
