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

        long on_device_connected_handler_id;
        GDBusObjectManager* bluez;
    } t;

    typedef void (*object_proc)(t* bt, GDBusObject* obj);
    typedef void (*key_val_proc)(t* bt, GDBusProxy* proxy, const gchar *key, GVariant *value);

    // prototype decl
    static void connect_to_device_signal(t* bt, GDBusObject* obj);
    static void foreach_objects(t* bt, object_proc proc);

    t* create()
    {
        t* bt = new t;
        return bt;
    }

    void destory(t* t)
    {
        if(t->bluez) g_object_unref(t->bluez);
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
        g_debug("BluezDBus: run_command(), is_connected = %d", is_connected);
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

    static void foreach_variant_key_value(GVariant* properties, t* bt, GDBusProxy* proxy, key_val_proc proc)
    {
        g_debug("BluezDBus: foreach_variant_key_value();");
        if (g_variant_n_children (properties) > 0)
        {
            GVariantIter iter;
            const gchar *key;
            GVariant *value;

            g_variant_iter_init (&iter, properties);
            while (g_variant_iter_loop (&iter, "{sv}", &key, &value)) {
                proc(bt, proxy, key, value);
            }
        }
    }

    static void run_command_if_connected(t* bt, GDBusProxy* proxy, const gchar* key, GVariant* value)
    {
        g_debug("BluezDBus: run_command_if_connected();");
        if(g_strcmp0("Connected", key) != 0) {
            return;
        }
        bool registered = is_registered_device(proxy, bt->mac_addresses);
        if(registered) {
            run_command(bt, g_variant_get_boolean(value));
        }
    }

    static void on_device_properties_changed(
            GDBusProxy *proxy,
            GVariant   *changed_properties,
            GStrv       invalidated_properties,
            gpointer    user_data)
    {
        g_debug("BluezDBus: on_device_properties_changed()");
        t* bt = (t*) user_data;
        foreach_variant_key_value(changed_properties, bt, proxy, run_command_if_connected);
    }


    static void connect_to_device_if_powered(t* bt, GDBusProxy* proxy, const gchar* key, GVariant* value)
    {
        g_debug("BluezDBus: connect_to_device_if_powered();");
        if(g_strcmp0("Powered", key) != 0) {
            return;
        }
        if(g_variant_get_boolean(value)) {
            foreach_objects(bt, connect_to_device_signal);
        }
    }

    static void on_adapter_properties_changed(
            GDBusProxy *proxy,
            GVariant   *changed_properties,
            GStrv       invalidated_properties,
            gpointer    user_data)
    {
        g_debug("BluezDBus: on_adapter_properties_changed()");
        t* bt = (t*)user_data;
        foreach_variant_key_value(changed_properties, bt, proxy, connect_to_device_if_powered);
    }

    static void foreach_objects(t* bt, object_proc proc) {
        GList* objects = g_dbus_object_manager_get_objects(bt->bluez);

        for(GList* p = objects; p != NULL; p = p->next) {
            GDBusObject* obj = static_cast<GDBusObject*>(p->data);
            proc(bt, obj);
            g_object_unref(obj);
        }

        if(objects) g_list_free(objects);
    }

    static void connect_to_device_signal(t* t, GDBusObject* obj) {
        GDBusInterface* idevice = g_dbus_object_get_interface(obj, "org.bluez.Device1");
        if(idevice != NULL) {
            t->on_device_connected_handler_id =
                    g_signal_connect(idevice, "g-properties-changed", G_CALLBACK(on_device_properties_changed), t);
        }
    }

    static void connect_to_adapter_signal(t* t, GDBusObject* obj) {
        GDBusInterface* idevice = g_dbus_object_get_interface(obj, "org.bluez.Adapter1");
        if(idevice != NULL) {
            g_signal_connect(idevice, "g-properties-changed", G_CALLBACK(on_adapter_properties_changed), t);
        }
    }

    bool init(t* t)
    {
        GError* err = NULL;
        t->bluez = g_dbus_object_manager_client_new_for_bus_sync(
                    G_BUS_TYPE_SYSTEM,
                    G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                    "org.bluez", "/",
                    NULL, NULL, NULL, NULL, &err);
        if(err != NULL) {
            g_error("BluezDBus: can't get bleuz object manager, %s", err->message);
            g_object_unref(err);
            return false;
        }

        foreach_objects(t, connect_to_device_signal);
        foreach_objects(t, connect_to_adapter_signal);

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
