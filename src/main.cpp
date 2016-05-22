#include "cec_wrapper.h"
#include "bluez_dbus.h"

#include <glib.h>
#include <iostream>

using namespace RpiEvtMon;

typedef struct options {
    gchar* on_cec_command;
    gchar* on_bt_command;
    gchar** bt_devices;
    gboolean  verbose;
} options_t;

gboolean parse_options(int* argc, char* argv[], options_t* options)
{
    GOptionEntry option_entries[] = {
        { "on-cec",          'c', 0, G_OPTION_ARG_STRING,      &(options->on_cec_command), "Execute COMMAND on cec source activated event", "COMMAND" },
        { "on-bluetooth",    'b', 0, G_OPTION_ARG_STRING,      &(options->on_bt_command),  "Execute COMMAND on bluetooth device connected", "COMMAND" },
        { "bt-device",       'd', 0, G_OPTION_ARG_STRING_ARRAY,&(options->bt_devices),     "Add bluetooth device mac address to monitoring", "MAC_ADDR" },
        { "verbose",         0,   0, G_OPTION_ARG_NONE,        &(options->verbose),        "Enable verbose mode", NULL },
        { NULL }
    };

    options->on_cec_command = NULL;
    options->on_bt_command = NULL;
    options->bt_devices = NULL;
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
    g_debug("Parsing options ...");
    options_t options;
    parse_options(&argc, argv, &options);

    if(options.verbose) {
        g_setenv("G_MESSAGES_DEBUG","all",true);
    }

    g_debug("Create Cec instance...");
    Cec::t* cec_wrapper = Cec::create();
    g_debug("Setting up Cec options ...");
    Cec::set_command(cec_wrapper, options.on_cec_command);
    g_debug("Initialize Cec ...");
    if(!Cec::init(cec_wrapper)) {
        return 1;
    }

    g_debug("Create BluezDBus instance...");
    BluezDBus::t* bluez_dbus = BluezDBus::create();
    g_debug("Setting up BluezDBus options ...");
    BluezDBus::set_command(bluez_dbus, options.on_bt_command);
    g_debug("Setting up BluezDBus bluetooth addresses ...");
    if(options.bt_devices != NULL) {
        for(int i = 0; options.bt_devices[i] != NULL; ++i)
        {
            BluezDBus::add_device_mac_address(bluez_dbus, options.bt_devices[i]);
        }
    }
    g_debug("Initialize BluezDBus ...");
    BluezDBus::init(bluez_dbus);

    g_debug("Enter main loop ...");
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    desotoy_options(&options);
    g_main_loop_unref(loop);
    Cec::destory(cec_wrapper);
    BluezDBus::destory(bluez_dbus);

    return 0;
}
