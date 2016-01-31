#include "cec_wrapper.h"

#include <string>
#include <iostream>
#include <stdio.h>

#include <glib.h>

#include <cec.h>
#include <cecloader.h>

using namespace CEC;

namespace RpiEvtMon { namespace Cec {

    typedef struct wrapper {
        bool is_activated;
        int log_level;
        const char * on_activated_command;
        const char * on_deactivated_command;

        ICECAdapter* adapter;
        const ICECCallbacks* callbacks;
        const libcec_configuration* config;
    } t;


    bool activate(t* wrapper) {
        wrapper->adapter->SetActiveSource(CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
    }

    bool is_activated(t* wrapper)
    {
        return wrapper->is_activated;
    }

    void set_on_activated_command(t* wrapper, const char * command)
    {
        wrapper->on_activated_command = command;
    }

    void set_on_deactivated_command(t* wrapper, const char * command)
    {
        wrapper->on_deactivated_command = command;
    }

    void set_log_level(t* wrapper, int log_level)
    {
        wrapper->log_level = log_level;
    }

    void cec_source_activated(void *cbParam, const cec_logical_address address, const uint8_t activated)
    {
        t* wrapper = (t*)cbParam;

        if(wrapper->is_activated != activated) {
            wrapper->is_activated = activated;

            char const * command;
            if(wrapper->is_activated) {
                command = wrapper->on_activated_command;
            }
            else {
                command = wrapper->on_deactivated_command;
            }

            GError* err;
            if(!g_spawn_command_line_async(command, &err)) {
                std::cout << "spawning command '" << command << "' is failed." << std::endl;
            }
        }
    }

    int cec_log_message(void *cbParam, const cec_log_message message)
    {
        std::string strLevel;
        t* wrapper = (t*)cbParam;

        if(!(wrapper->log_level & message.level)) {
            return 0;
        }

        switch (message.level)
        {
            case CEC_LOG_ERROR:
                strLevel = "ERROR:   ";
                break;
            case CEC_LOG_WARNING:
                strLevel = "WARNING: ";
                break;
            case CEC_LOG_NOTICE:
                strLevel = "NOTICE:  ";
                break;
            case CEC_LOG_TRAFFIC:
                strLevel = "TRAFFIC: ";
                break;
            case CEC_LOG_DEBUG:
                strLevel = "DEBUG:   ";
                break;
            default:
                break;
        }
        std::cout << strLevel << message.message << std::endl;
        return 0;
    }

    bool init(t* wrapper)
    {
        ICECCallbacks* cec_callbacks = new ICECCallbacks;
        libcec_configuration* cec_config = new libcec_configuration;

        wrapper->callbacks = cec_callbacks;
        wrapper->config = cec_config;

        cec_config->Clear();
        cec_callbacks->Clear();

        cec_callbacks->CBCecLogMessage = &cec_log_message;
        cec_callbacks->CBCecSourceActivated = &cec_source_activated;

        snprintf(cec_config->strDeviceName, 13, "RpiEvtMon");
        cec_config->clientVersion   = LIBCEC_VERSION_CURRENT;
        cec_config->bActivateSource = 0;
        cec_config->callbacks       = cec_callbacks;
        cec_config->callbackParam   = (void *)wrapper;
        cec_config->deviceTypes.Add(CEC_DEVICE_TYPE_PLAYBACK_DEVICE);

        wrapper->adapter = LibCecInitialise(cec_config);
        if(!wrapper->adapter) {
            std::cout << "can't initialize libcec." << std::endl;
            return false;
        }

        wrapper->adapter->InitVideoStandalone();

        cec_adapter devices[10];
        std::string port;
        uint8_t devicesFound = wrapper->adapter->FindAdapters(devices, 10, NULL);
        if(devicesFound <= 0) {
            std::cout << "no device found." << std::endl;
            UnloadLibCec(wrapper->adapter);
            return false;
        }
        port = devices[0].comm;

        if(!wrapper->adapter->Open(port.c_str())) {
            std::cout << "can't open cec adapter." << std::endl;
            UnloadLibCec(wrapper->adapter);
            return false;
        }

        return true;
    }

    t* create()
    {
        t* wrapper = new t;
        wrapper->on_activated_command = NULL;
        wrapper->on_deactivated_command = NULL;
        wrapper->callbacks = NULL;
        wrapper->config = NULL;
        wrapper->log_level = CEC_LOG_ERROR | CEC_LOG_WARNING;

        return wrapper;
    }

    void destory(t* wrapper) {
        if(wrapper->adapter) {
            wrapper->adapter->Close();
            UnloadLibCec(wrapper->adapter);
            wrapper->adapter = NULL;
        }
        delete wrapper->callbacks;
        delete wrapper->config;
        delete wrapper;
    }


} }// namespace RpiEventMonitor::Cec
