#include "cec_wrapper.h"

#include <string>
#include <iostream>
#include <stdio.h>

#include <glib.h>

#include <cec.h>
#include <cecloader.h>

using namespace CEC;

namespace RpiEvtMon { namespace Cec {

    typedef struct route {
        bool is_activated;
    } route_t;

    route_t* create_route() {
        route_t* r = new route_t;

        r->is_activated = false;

        return r;
    }

    typedef struct wrapper {
        const char * on_activated_command;
        const char * on_deactivated_command;

        ICECAdapter* adapter;
        bool is_adapter_opened;
        const ICECCallbacks* callbacks;
        const libcec_configuration* config;

        GHashTable* routes;
    } t;

    void set_on_activated_command(t* wrapper, const char * command)
    {
        wrapper->on_activated_command = command;
    }

    void set_on_deactivated_command(t* wrapper, const char * command)
    {
        wrapper->on_deactivated_command = command;
    }

    void fire_command(t* wrapper, gint address, bool activated)
    {
        char const * base;
        char const * which;
        if(activated) {
            base = wrapper->on_activated_command;
            which = "activate";
        }
        else {
            base = wrapper->on_deactivated_command;
            which = "deactivate";
        }

        int len = strlen(base) + 20;
        char command[len];
        snprintf(command, len, "%s %s %04x", base, which, address);

        GError* err;
        if(!g_spawn_command_line_async(command, &err)) {
            g_error("spawning command '%s' is failed, %s", command, err->message);
            g_error_free(err);
        }
    }

    int cec_log_message(void *cbParam, const cec_log_message message)
    {
        std::string strLevel;
        t* wrapper = (t*)cbParam;

        switch (message.level)
        {
            case CEC_LOG_ERROR:
                g_error("%s", message.message);
                break;
            case CEC_LOG_WARNING:
                g_warning("%s", message.message);
                break;
            case CEC_LOG_NOTICE:
                g_info("%s", message.message);
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

    int cec_command(void *cbParam, const cec_command cmd) {
        if (cmd.opcode == CEC_OPCODE_ROUTING_CHANGE &&
            cmd.parameters.size == 4)
        {
            t* wrapper = (t*)cbParam;
            gint addr = (cmd.parameters[2] << 8 | (gint)cmd.parameters[3]);
            gpointer key = GINT_TO_POINTER(addr);

            route_t* route;
            if(g_hash_table_contains(wrapper->routes, key)) {
                route = (route_t*)g_hash_table_lookup(wrapper->routes, key);
            }
            else {
                route = create_route();
                g_hash_table_insert(wrapper->routes, key, route);
            }

            route->is_activated = true;

            GHashTableIter iter;
            gpointer k, v;
            g_hash_table_iter_init (&iter, wrapper->routes);
            while (g_hash_table_iter_next (&iter, &k, &v)) {
                route_t* r = (route_t*)v;
                if(k == key || !r->is_activated) {
                    continue;
                }
                else {
                    r->is_activated = false;

                }
            }
        }

        return 1;
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
        cec_callbacks->CBCecCommand = &cec_command;

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
        wrapper->is_adapter_opened = false;

        wrapper->routes = g_hash_table_new(g_int_hash, g_int_equal);

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
        g_hash_table_destroy(wrapper->routes);
        delete wrapper->callbacks;
        delete wrapper->config;
        delete wrapper;
    }


} }// namespace RpiEventMonitor::Cec
