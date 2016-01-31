#include "bluez_dbus.h"

#include <glib.h>
#include <gio/gio.h>

#include <vector>
#include <iostream>

namespace RpiEvtMon { namespace BluezDBus {

    typedef struct t {
        std::vector<const char *> mac_addresses;
    } t;

    t* create()
    {
        t* bt = new t;
        return bt;
    }

    void destory(t* t)
    {
        delete t;
    }

    bool init(t* t)
    {
        GError* err = NULL;
        GDBusConnection system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
        GDBusObjectManager* bluez = g_dbus_object_manager_client_new_for_bus_sync(
                G_BUS_TYPE_SYSTEM,
                G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                "org.bluez",
                "/",
                NULL,
                NULL,
                NULL,
                NULL,
                &err);
        GList* objects = g_dbus_object_manager_get_objects(bluez);
        for(GList* p = objects; p != NULL; p = p->next) {
            GDBusObject* obj = (GDBusObject*)p->data;
            GDBusInterface* idevice = g_dbus_object_get_interface(obj, "org.bluez.Device1");
            if(idevice != NULL) {
                g_dbus_connection_signal_subscribe(

            }

            g_object_unref(obj);
        }
        g_list_free(objects);
    }

    void add_device_mac_address(t* t, const char* mac);
    void set_on_connected_command(t* t, const char* command);
    void set_on_disconnected_command(t* t, const char* command);

} } // namespace RpiEvtMon
