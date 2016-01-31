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

    void on_device_connected(t* t)
    {
        std::cout << "on_device_connect" << std::endl;
    }

    void on_device_disconnected(t* t)
    {
        std::cout << "on_device_disconnect" << std::endl;
    }

    bool is_registered_device(GDBusProxy* proxy, std::vector<const char *> &addrs)
    {
        GVariant* variant = g_dbus_proxy_get_cached_property(proxy,"Address");
        gsize len;
        std::string dev_addr = g_variant_get_string(variant, &len);
        std::cout << "proxy addr = " << dev_addr << std::endl;

        bool found = false;
        for(std::vector<const char*>::iterator it = addrs.begin(); it != addrs.end(); ++it)
        {
            if(dev_addr.find(*it) != std::string::npos) {
                found = true;
                break;
            }
        }

        g_variant_unref(variant);
        return found;
    }

    void on_properties_changed(GDBusProxy *proxy,
                               GVariant   *changed_properties,
                               GStrv       invalidated_properties,
                               gpointer    user_data)
    {
        t* bt = (t*) user_data;

        if (g_variant_n_children (changed_properties) > 0)
        {
            GVariantIter iter;
            const gchar *key;
            GVariant *value;

            g_variant_iter_init (&iter, changed_properties);
            while (g_variant_iter_loop (&iter, "{sv}", &key, &value)) {
                if(g_strcmp0("Connected", key) != 0) {
                    continue;
                }

                bool registered = is_registered_device(proxy, bt->mac_addresses);
                if(registered) {
                    if(g_variant_get_boolean(value)) {
                        on_device_connected(bt);
                    }
                    else {
                        on_device_disconnected(bt);
                    }
                }
            }
        }
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
                g_signal_connect(idevice, "g-properties-changed", G_CALLBACK(on_properties_changed), t);
            }
            g_object_unref(obj);
        }
        g_list_free(objects);
    }

    void add_device_mac_address(t* t, const char* mac)
    {
        t->mac_addresses.push_back(mac);
    }

    void set_on_connected_command(t* t, const char* command);
    void set_on_disconnected_command(t* t, const char* command);

} } // namespace RpiEvtMon
