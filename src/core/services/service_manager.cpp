#include "service_manager.hpp"

namespace Trident {

void ServiceManager::init(Memory& memory) {
    mem = &memory;
    nextHandle = 0x1000;
    services.clear();
    handleToService.clear();
    registerDefaultServices();
}

void ServiceManager::reset() {
    nextHandle = 0x1000;
    services.clear();
    handleToService.clear();
    registerDefaultServices();
}

void ServiceManager::registerService(const std::string& name, Handler handler) {
    services[name] = {name, handler};
}

uint32_t ServiceManager::connectToService(const std::string& name) {
    auto it = services.find(name);
    if (it == services.end()) return 0;

    uint32_t handle = nextHandle++;
    handleToService[handle] = name;
    return handle;
}

void ServiceManager::handleSyncRequest(uint32_t sessionHandle, uint32_t* cmdBuf) {
    auto it = handleToService.find(sessionHandle);
    if (it == handleToService.end()) {
        // Unknown handle, return error
        cmdBuf[1] = 0xD8E007F7; // ResultCode: invalid handle
        return;
    }

    auto srvIt = services.find(it->second);
    if (srvIt != services.end() && srvIt->second.handler) {
        srvIt->second.handler(cmdBuf);
    } else {
        cmdBuf[1] = 0; // Stub: return success
    }
}

void ServiceManager::registerDefaultServices() {
    registerService("srv:", [this](uint32_t* cmd) { handleSrv(cmd); });
    registerService("APT:U", [this](uint32_t* cmd) { handleAPT(cmd); });
    registerService("APT:S", [this](uint32_t* cmd) { handleAPT(cmd); });
    registerService("gsp::Gpu", [this](uint32_t* cmd) { handleGSP(cmd); });
    registerService("hid:USER", [this](uint32_t* cmd) { handleHID(cmd); });
    registerService("fs:USER", [this](uint32_t* cmd) { handleFS(cmd); });
    registerService("cfg:u", [this](uint32_t* cmd) { handleCFG(cmd); });
    registerService("dsp::DSP", [this](uint32_t* cmd) { handleDSP(cmd); });
    registerService("csnd:SND", [this](uint32_t* cmd) { handleCSND(cmd); });
    registerService("am:net", [this](uint32_t* cmd) { handleAM(cmd); });
    registerService("ptm:sysm", [this](uint32_t* cmd) { handlePTM(cmd); });
    registerService("ndm:u", [this](uint32_t* cmd) { handleNDM(cmd); });
    registerService("act:u", [this](uint32_t* cmd) { handleACT(cmd); });
    registerService("nim:aoc", [this](uint32_t* cmd) { handleNIM(cmd); });
    registerService("nwm::UDS", [this](uint32_t* cmd) { handleNWM(cmd); });
    registerService("soc:U", [this](uint32_t* cmd) { handleSOC(cmd); });
    registerService("mic:u", [this](uint32_t* cmd) { handleMIC(cmd); });
    registerService("cam:u", [this](uint32_t* cmd) { handleCAM(cmd); });
    registerService("cecd:u", [this](uint32_t* cmd) { handleCECD(cmd); });
    registerService("ir:USER", [this](uint32_t* cmd) { handleIR(cmd); });
    registerService("y2r:u", [this](uint32_t* cmd) { handleY2R(cmd); });
    registerService("ldr:ro", [this](uint32_t* cmd) { handleLDR(cmd); });
    registerService("http:C", [this](uint32_t* cmd) { handleHTTP(cmd); });
    registerService("ssl:C", [this](uint32_t* cmd) { handleSSL(cmd); });
    registerService("ps:ps", [this](uint32_t* cmd) { handlePS(cmd); });
    registerService("err:f", [this](uint32_t* cmd) { handleERR(cmd); });
    registerService("ns:s", [this](uint32_t* cmd) { handleNS(cmd); });
    registerService("pm:app", [this](uint32_t* cmd) { handlePM(cmd); });
}

// All service stubs return success (0) for now
void ServiceManager::handleSrv(uint32_t* cmdBuf) {
    uint16_t cmdId = static_cast<uint16_t>(cmdBuf[0] >> 16);
    switch (cmdId) {
        case 0x0001: // RegisterClient
            cmdBuf[1] = 0; // success
            break;
        case 0x0002: // EnableNotification
            cmdBuf[1] = 0;
            cmdBuf[3] = 0; // notification semaphore handle (stub)
            break;
        case 0x0005: { // GetServiceHandle
            // Service name at cmdBuf[1..2], length at cmdBuf[3]
            char name[16] = {};
            std::memcpy(name, &cmdBuf[1], 8);
            uint32_t handle = connectToService(name);
            cmdBuf[1] = (handle != 0) ? 0 : 0xD8E06406;
            cmdBuf[3] = handle;
            break;
        }
        default:
            cmdBuf[1] = 0;
            break;
    }
}

void ServiceManager::handleAPT(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleGSP(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleHID(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleFS(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleCFG(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleDSP(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleCSND(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleAM(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handlePTM(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleNDM(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleACT(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleNIM(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleNWM(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleSOC(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleMIC(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleCAM(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleCECD(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleIR(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleY2R(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleLDR(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleHTTP(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleSSL(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handlePS(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleERR(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handleNS(uint32_t* cmdBuf) { cmdBuf[1] = 0; }
void ServiceManager::handlePM(uint32_t* cmdBuf) { cmdBuf[1] = 0; }

} // namespace Trident
