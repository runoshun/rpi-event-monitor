
namespace RpiEvtMon { namespace BluezDBus {

    typedef struct t t;

    t* create();
    void destory(t* t);

    bool init(t* t);

    void add_device_mac_address(t* t, const char* mac);
    void set_command(t* t, const char* command);

} } // namespace RpiEvtMon
