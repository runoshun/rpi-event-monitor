#include "cec_wrapper.h"
#include "bluez_dbus.h"

#include <cec.h>
#include <glib.h>

#include <iostream>

using namespace RpiEvtMon;

typedef struct options {
    gchar* on_cec_activated;
    gchar* on_cec_deactivated;
    gboolean  verbose;
} options_t;

gboolean parse_options(int* argc, char* argv[], options_t* options)
{
    GOptionEntry option_entries[] = {
        { "on-cec-activated",   'a', 0, G_OPTION_ARG_STRING, &(options->on_cec_activated),   "Execute COMMAND on cec source activated event", "COMMAND" },
        { "on-cec-deactivated", 'd', 0, G_OPTION_ARG_STRING, &(options->on_cec_deactivated), "Execute COMMAND on cec source deactivated event", "COMMAND" },
        { "verbose",            0,   0, G_OPTION_ARG_NONE,   &(options->verbose),            "Enable verbose mode", NULL },
        { NULL }
    };

    options->on_cec_activated = NULL;
    options->on_cec_deactivated = NULL;
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
    BluezDBus::add_device_mac_address(dbus, "04_98_F3_0D_0C_11");
    BluezDBus::init(dbus);

    GMainLoop* loop;
    loop = g_main_loop_new(NULL, FALSE);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    Cec::destory(cec_wrapper);

    return 0;
}
