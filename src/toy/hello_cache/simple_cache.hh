// 2023-08-31 16:42
#ifndef SIMPLE_CACHE_HH
#define SIMPLE_CACHE_HH

#include "base/trace.hh"
#include "debug/Toy.hh"
#include "mem/port.hh"
#include "params/SimpleCache.hh"
#include "sim/sim_object.hh"

#define BLOCK_SIZE 64

namespace gem5 {

class SimpleCache : public SimObject {
   private:
    class CPUSidePort : public SlavePort {
       private:
        SimpleCache *owner;
        bool retry;

       public:
        CPUSidePort(const std::string &if_name, SimpleCache *owner)
            : SlavePort(if_name, owner), owner(owner), retry(false) {}

        AddrRangeList getAddrRanges() const override {
            return owner->getAddrRanges();
        }

        void sendPacket(PacketPtr pkt) {
            DPRINTF(Toy, "cpuport:sendPacket: %s\n", pkt->print());
            if (!sendTimingResp(pkt)) {
                DPRINTF(Toy, "cpuport:sendPacket:blocked: %s\n", pkt->print());
                owner->blockedPkt = pkt;
            }
        }
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
        bool recvTimingReq(PacketPtr pkt) override {
            DPRINTF(Toy, "cpuport:recvTimingReq: %s\n", pkt->print());
            if (owner->blockedPkt || retry) {
                // The cache may not be able to send a reply if this is blocked
                retry = true;
                return false;
            }
            if (!owner->handleRequest(pkt)) {
                DPRINTF(
                    Toy, "cpu port:recvTimingReq:blocked: %s\n", pkt->print());
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
        SimpleCache *owner;

       public:
        MemSidePort(const std::string &if_name, SimpleCache *owner)
            : MasterPort(if_name, owner), owner(owner) {}

        void sendPacket(PacketPtr pkt) {
            DPRINTF(Toy, "mem_side:sendPacket: %s\n", pkt->print());
            if (!sendTimingReq(pkt)) {
                DPRINTF(Toy, "mem_side:sendPacket:blocked: %s\n", pkt->print());
                owner->blockedPkt = pkt;
            }
        }

       protected:
        bool recvTimingResp(PacketPtr pkt) override {
            DPRINTF(Toy, "mem_side:recvTimingResp: %s\n", pkt->print());
            owner->handleResponse(pkt);
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

    CPUSidePort cpu_side;
    MemSidePort mem_side;
    const Cycles latency;
    std::unordered_map<Addr, uint8_t *> cacheStore;

    bool blocked;
    PacketPtr originPkt;
    PacketPtr blockedPkt;

   public:
    SimpleCache(const SimpleCacheParams &p)
        : SimObject(p),
          cpu_side("cpu_side port", this),
          mem_side("mem_side port", this),
          latency(p.latency),
          blocked(false),
          originPkt(nullptr),
          blockedPkt(nullptr) {
        DPRINTF(Toy, "simple memory object init\n");
    }

    Port &getPort(const std::string &if_name, PortID idx) {
        if (if_name == "mem_side") {
            return mem_side;
        } else if (if_name == "cpu_side") {
            return cpu_side;
        } else {
            return SimObject::getPort(if_name, idx);
        }
    }

    static bool isCacheable(PacketPtr pkt) {
        return !(
            pkt->req->isLLSC() || pkt->req->isSwap() || pkt->req->isAtomic());
    }

    void handleFunctional(PacketPtr pkt) {
        if (!isCacheable(pkt)) {
            mem_side.sendFunctional(pkt);
            return;
        }
        if (accessFunctional(pkt)) {
            pkt->makeResponse();
        } else {
            mem_side.sendFunctional(pkt);
        }
    }
    void handleRangeChange() { cpu_side.sendRangeChange(); }
    bool handleRequest(PacketPtr pkt) {
        DPRINTF(Toy, "main:handleRequest: %s\n", pkt->print());
        if (blocked) {
            DPRINTF(Toy, "main:handleRequest:blocked: %s\n", pkt->print());
            return false;
        }
        DPRINTF(Toy, "main:handleRequest:go: %s\n", pkt->print());
        blocked = true;
        schedule(
            new EventFunctionWrapper(
                [this, pkt] { access(pkt); }, name() + ".accessEvent", true),
            curTick() + latency);

        return true;
    }

    bool accessFunctional(PacketPtr pkt) {
        // DPRINTF(Toy, "accessFunctional: %s\n", pkt->print());
        Addr block_addr = pkt->getBlockAddr(BLOCK_SIZE);
        auto it = cacheStore.find(block_addr);
        if (it != cacheStore.end()) {
            if (pkt->isWrite()) {
                pkt->writeDataToBlock(it->second, BLOCK_SIZE);
            } else if (pkt->isRead()) {
                pkt->setDataFromBlock(it->second, BLOCK_SIZE);
            } else {
                panic("Unknown packet type!");
            }
            return true;
        }
        return false;
    }

    void access(PacketPtr pkt) {
        if (!isCacheable(pkt)) {
            mem_side.sendPacket(pkt);
            return;
        }
        bool hit = accessFunctional(pkt);
        DPRINTF(
            Toy, "%s for packet: 0x%x\n", hit ? "Hit" : "Miss", pkt->print());
        if (hit) {
            pkt->makeResponse();
            sendResponse(pkt);
        } else {
            MemCmd cmd = MemCmd::ReadReq;
            PacketPtr new_pkt = new Packet(pkt->req, cmd, BLOCK_SIZE);
            new_pkt->allocate();
            originPkt = pkt;
            mem_side.sendPacket(new_pkt);
        }
    }

    void sendRetryReq() { cpu_side.sendRetryPacket(); }

    void handleResponse(PacketPtr pkt) {
        if (originPkt != nullptr) {
            uint8_t *data = new uint8_t[BLOCK_SIZE];
            cacheStore[pkt->getAddr()] = data;
            pkt->writeDataToBlock(data, BLOCK_SIZE);
            bool hit = accessFunctional(originPkt);
            panic_if(!hit, "Should always hit after inserting");
            originPkt->makeResponse();
            sendResponse(originPkt);
            originPkt = nullptr;
        } else {
            sendResponse(pkt);
        }
    }

    void sendResponse(PacketPtr pkt) {
        DPRINTF(Toy, "main:sendResponse: %s\n", pkt->print());
        assert(blocked);
        blocked = false;
        cpu_side.sendPacket(pkt);
        sendRetryReq();
    }

    AddrRangeList getAddrRanges() const {
        DPRINTF(Toy, "get addr range");
        return mem_side.getAddrRanges();
    }
};  // namespace gem5
}  // namespace gem5

#endif  // SIMPLE_CACHE_HH
