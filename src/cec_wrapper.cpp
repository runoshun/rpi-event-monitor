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
        const char * on_activated_command;
        const char * on_deactivated_command;

        ICECAdapter* adapter;
        const ICECCallbacks* callbacks;
        const libcec_configuration* config;

        bool is_adapter_opened;
        bool is_activated;
    } t;


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
                g_error("spawning command '%s' is failed, %s", err->message);
                g_error_free(err);
            }
        }
    }

    int cec_log_message(void *cbParam, const cec_log_message message)
    {
        std::string strLevel;
        t* wrapper = (t*)cbParam;

        switch (message.level)
        {
            case CEC_LOG_ERROR:
                g_error(message.message);
                break;
            case CEC_LOG_WARNING:
                g_warning(message.message);
                break;
            case CEC_LOG_NOTICE:
                g_info(message.message);
                break;
            case CEC_LOG_TRAFFIC:
                g_debug("TRAFFIC:  %s", message.message);
                break;
            case CEC_LOG_DEBUG:
                g_debug("DEBUG:    %s", message.message);
                break;
            default:
                break;
        }
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
            g_error("can't initialize libcec.");
            destory(wrapper);
            return false;
        }

        wrapper->adapter->InitVideoStandalone();

        cec_adapter devices[10];
        std::string port;
        uint8_t devicesFound = wrapper->adapter->FindAdapters(devices, 10, NULL);
        if(devicesFound <= 0) {
            g_error("no device found.");
            destory(wrapper);
            return false;
        }
        port = devices[0].comm;

        if(!wrapper->adapter->Open(port.c_str())) {
            g_error("can't open cec adapter.");
            destory(wrapper);
            return false;
        }
        wrapper->is_adapter_opened = true;

        return true;
    }

    t* create()
    {
        t* wrapper = new t;
        wrapper->on_activated_command = NULL;
        wrapper->on_deactivated_command = NULL;
        wrapper->callbacks = NULL;
        wrapper->config = NULL;
        wrapper->is_activated = false;
        wrapper->is_adapter_opened = false;

        return wrapper;
    }

    void destory(t* wrapper) {
        if(wrapper->adapter) {
            if(wrapper->is_adapter_opened) {
                wrapper->adapter->Close();
            }
            UnloadLibCec(wrapper->adapter);
            wrapper->adapter = NULL;
        }
        delete wrapper->callbacks;
        delete wrapper->config;
        delete wrapper;
    }


} }// namespace RpiEventMonitor::Cec
