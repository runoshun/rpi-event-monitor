#include "cec_wrapper.h"
#include "bluez_dbus.h"

#include <cec.h>
#include <glib.h>

#include <iostream>

using namespace RpiEvtMon;

typedef struct options {
    gchar* on_cec_activated;
    gchar* on_cec_deactivated;
    gchar* on_bt_connected;
    gchar* on_bt_disconnected;
    gchar** bt_devices;
    gboolean  verbose;
} options_t;

gboolean parse_options(int* argc, char* argv[], options_t* options)
{
    GOptionEntry option_entries[] = {
        { "on-cec-activated",   'a', 0, G_OPTION_ARG_STRING,      &(options->on_cec_activated),   "Execute COMMAND on cec source activated event", "COMMAND" },
        { "on-cec-deactivated", 'z', 0, G_OPTION_ARG_STRING,      &(options->on_cec_deactivated), "Execute COMMAND on cec source deactivated event", "COMMAND" },
        { "on-bt-connected",    'c', 0, G_OPTION_ARG_STRING,      &(options->on_bt_connected),    "Execute COMMAND on bluetooth device connected", "COMMAND" },
        { "on-bt-disconnected", 'x', 0, G_OPTION_ARG_STRING,      &(options->on_bt_disconnected), "Execute COMMAND on bluetooth device disconnected", "COMMAND" },
        { "bt-device",          'b', 0, G_OPTION_ARG_STRING_ARRAY,&(options->bt_devices),         "Add bluetooth device mac address to monitoring", "MAC_ADDR" },
        { "verbose",            0,   0, G_OPTION_ARG_NONE,        &(options->verbose),            "Enable verbose mode", NULL },
        { NULL }
    };

    options->on_cec_activated = NULL;
    options->on_cec_deactivated = NULL;
    options->on_bt_connected = NULL;
    options->on_bt_disconnected = NULL;
    options->verbose = FALSE;

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new("monitor cec and bluetooth event and execute command");
    g_option_context_add_main_entries(context, option_entries, NULL);
    if (!g_option_context_parse (context, argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        return FALSE;
    }

    return TRUE;
}

void desotoy_options(options_t* options)
{
    g_strfreev(options->bt_devices);
}

int main(int argc, char* argv[])
{
    options_t options;
    parse_options(&argc, argv, &options);

    Cec::t* cec_wrapper = Cec::create();
    Cec::set_on_activated_command(cec_wrapper, options.on_cec_activated);
    Cec::set_on_deactivated_command(cec_wrapper, options.on_cec_deactivated);
    if(options.verbose) {
        Cec::set_log_level(cec_wrapper, CEC::CEC_LOG_ALL);
    }

    if(!Cec::init(cec_wrapper)) {
        return 1;
    }

    BluezDBus::t* dbus = BluezDBus::create();
    BluezDBus::init(dbus);

    GMainLoop* loop;
    loop = g_main_loop_new(NULL, FALSE);

    g_main_loop_run(loop);

    desotoy_options(&options);
    g_main_loop_unref(loop);
    Cec::destory(cec_wrapper);

    return 0;
}
