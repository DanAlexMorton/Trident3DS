#pragma once

#include "../memory/memory.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace Trident {

class ServiceManager {
public:
    using Handler = std::function<void(uint32_t* cmdBuf)>;

    void init(Memory& memory);
    void reset();

    // Register a service port
    void registerService(const std::string& name, Handler handler);

    // Handle a service call (called from SVC SendSyncRequest)
    void handleSyncRequest(uint32_t sessionHandle, uint32_t* cmdBuf);

    // Connect to a service by name (called from SVC ConnectToPort / srv:GetServiceHandle)
    uint32_t connectToService(const std::string& name);

private:
    Memory* mem = nullptr;
    uint32_t nextHandle = 0x1000;

    struct Service {
        std::string name;
        Handler handler;
    };

    std::unordered_map<std::string, Service> services;
    std::unordered_map<uint32_t, std::string> handleToService;

    void registerDefaultServices();

    // Default HLE service stubs
    void handleSrv(uint32_t* cmdBuf);          // srv: (service manager)
    void handleAPT(uint32_t* cmdBuf);          // APT:U / APT:S (applet manager)
    void handleGSP(uint32_t* cmdBuf);          // gsp::Gpu
    void handleHID(uint32_t* cmdBuf);          // hid:USER (input)
    void handleFS(uint32_t* cmdBuf);           // fs:USER (filesystem)
    void handleCFG(uint32_t* cmdBuf);          // cfg:u (system config)
    void handleDSP(uint32_t* cmdBuf);          // dsp::DSP
    void handleCSND(uint32_t* cmdBuf);         // csnd:SND
    void handleAM(uint32_t* cmdBuf);           // am:net (title management)
    void handlePTM(uint32_t* cmdBuf);          // ptm:sysm (power/battery)
    void handleNDM(uint32_t* cmdBuf);          // ndm:u (network daemon)
    void handleACT(uint32_t* cmdBuf);          // act:u (NNID)
    void handleNIM(uint32_t* cmdBuf);          // nim:aoc (DLC)
    void handleNWM(uint32_t* cmdBuf);          // nwm::UDS (local wireless)
    void handleSOC(uint32_t* cmdBuf);          // soc:U (BSD sockets)
    void handleMIC(uint32_t* cmdBuf);          // mic:u (microphone)
    void handleCAM(uint32_t* cmdBuf);          // cam:u (camera)
    void handleCECD(uint32_t* cmdBuf);         // cecd:u (StreetPass)
    void handleIR(uint32_t* cmdBuf);           // ir:USER (infrared)
    void handleY2R(uint32_t* cmdBuf);          // y2r:u (YUV to RGB)
    void handleLDR(uint32_t* cmdBuf);          // ldr:ro (dynamic linking)
    void handleHTTP(uint32_t* cmdBuf);         // http:C (HTTP requests)
    void handleSSL(uint32_t* cmdBuf);          // ssl:C (SSL)
    void handlePS(uint32_t* cmdBuf);           // ps:ps (process services)
    void handleERR(uint32_t* cmdBuf);          // err:f (error display)
    void handleNS(uint32_t* cmdBuf);           // ns:s (reboot/launch)
    void handlePM(uint32_t* cmdBuf);           // pm:app (process manager)
};

} // namespace Trident
