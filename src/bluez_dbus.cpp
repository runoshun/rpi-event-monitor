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

    static bool is_registered_device(GDBusProxy* proxy, std::vector<const char *> &addrs)
    {
        GVariant* variant = g_dbus_proxy_get_cached_property(proxy,"Address");
        gsize len;
        std::string dev_addr = g_variant_get_string(variant, &len);

        g_debug("BluezDBus: is_registered_device(), device = %s", dev_addr.c_str());
        bool found = false;
        for(std::vector<const char*>::iterator it = addrs.begin(); it != addrs.end(); ++it)
        {
            if(dev_addr.find(*it) != std::string::npos) {
                found = true;
                break;
            }
        }

        g_variant_unref(variant);

        if(!found) {
            g_info("connected unregistered device : %s", dev_addr.c_str());
        }
        return found;
    }

    static void run_command(t* t, gboolean is_connected)
    {
        const char* command;
        g_debug("BluezDBus: run_command(), is_connected = %b", is_connected);
        if(is_connected) {
            command = t->on_connect_command;
        }
        else {
            command = t->on_disconnect_command;
        }
        if(command != NULL) {
            GError *err;
            gboolean ret = g_spawn_command_line_async(command, &err);
            if(!ret) {
                g_error("can't spawn command '%s', ", command, err->message);
                g_error_free(err);
            }
        }
    }

    static void on_properties_changed(
            GDBusProxy *proxy,
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

            g_debug("BluezDBus: on_properties_changed()");
            g_variant_iter_init (&iter, changed_properties);
            while (g_variant_iter_loop (&iter, "{sv}", &key, &value)) {
                if(g_strcmp0("Connected", key) != 0) {
                    continue;
                }
                bool registered = is_registered_device(proxy, bt->mac_addresses);
                if(registered) {
                    run_command(bt, g_variant_get_boolean(value));
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
        
        if(err != NULL) {
            
        }

        GList* objects = g_dbus_object_manager_get_objects(bluez);
        for(GList* p = objects; p != NULL; p = p->next) {
            GDBusObject* obj = static_cast<GDBusObject*>(p->data);
            GDBusInterface* idevice = g_dbus_object_get_interface(obj, "org.bluez.Device1");
            if(idevice != NULL) {
                g_signal_connect(idevice, "g-properties-changed", G_CALLBACK(on_properties_changed), t);
            }
            g_object_unref(obj);
        }
        g_list_free(objects);
        
        g_object_unref(system_bus);
        return true;
    }

    void add_device_mac_address(t* t, const char* mac)
    {
        g_debug("BluezDBus: mac address '%s' is added", mac);
        t->mac_addresses.push_back(mac);
    }

    void set_on_connected_command(t* t, const char* command)
    {
        t->on_connect_command = command;
    }

    void set_on_disconnected_command(t* t, const char* command)
    {
        t->on_disconnect_command = command;
    }

} } // namespace RpiEvtMon
