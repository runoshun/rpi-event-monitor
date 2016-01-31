#include "bluez_dbus.h"

#include <glib.h>
#include <gio/gio.h>

#include <vector>
#include <iostream>

namespace RpiEvtMon { namespace BluezDBus {

    typedef struct t {
        std::vector<const char *> mac_addresses;
        const char* on_connect_command;
        const char* on_disconnect_command;
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

    void on_device_connect(GDBusProxy *proxy,
                           GVariant   *changed_properties,
                           GStrv       invalidated_properties,
                           gpointer    user_data)
    {
        t* bt = (t*) user_data;

        std::cout << "on_device_connect(), invalidated_properties = " << invalidated_properties << std::endl;
        /*
    std::string path = object_path;
        for(auto it = t->mac_addresses.begin(); it != t->mac_addresses.end(); it++) {
            std::string mac = *it;
            if(path.find(mac) != std::string::npos) {
            }
        }
        */
    }

    bool init(t* t)
    {
        GError* err = NULL;
        GDBusConnection* system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
        GDBusObjectManager* bluez = g_dbus_object_manager_client_new_sync(
                system_bus,
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
                g_signal_connect(idevice, "g-properties-changed", G_CALLBACK(on_device_connect), t);
            }
            g_object_unref(obj);
        }
        g_list_free(objects);
    }

    void add_device_mac_address(t* t, const char* mac);
    void set_on_connected_command(t* t, const char* command);
    void set_on_disconnected_command(t* t, const char* command);

} } // namespace RpiEvtMon
