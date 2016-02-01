#include "cec_wrapper.h"
#include "bluez_dbus.h"

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

    if(options.verbose) {
        g_setenv("G_MESSAGES_DEBUG","1",true);
    }

    Cec::t* cec_wrapper = Cec::create();
    Cec::set_on_activated_command(cec_wrapper, options.on_cec_activated);
    Cec::set_on_deactivated_command(cec_wrapper, options.on_cec_deactivated);
    if(!Cec::init(cec_wrapper)) {
        return 1;
    }

    BluezDBus::t* bluez_dbus = BluezDBus::create();
    BluezDBus::set_on_connected_command(bluez_dbus, options.on_bt_connected);
    BluezDBus::set_on_disconnected_command(bluez_dbus, options.on_bt_disconnected);
    for(int i = 0; options.bt_devices[i] != NULL; ++i)
    {
        BluezDBus::add_device_mac_address(bluez_dbus, options.bt_devices[i]);
    }
    BluezDBus::init(bluez_dbus);

    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    desotoy_options(&options);
    g_main_loop_unref(loop);
    Cec::destory(cec_wrapper);
    BluezDBus::destory(bluez_dbus);

    return 0;
}
