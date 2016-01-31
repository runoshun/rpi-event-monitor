namespace RpiEvtMon { namespace Cec {

    typedef struct wrapper t;

    t* create();
    void destory(t* wrapper);
    bool init(t* wrapper);

    bool activate(t* wrapper);
    bool is_activated(t* wrapper);

    void set_on_activated_command(t* wrapper, const char * command);
    void set_on_deactivated_command(t* wrapper, const char * command);
    void set_log_level(t* wrapper, int log_level);

} } // namespace RpiEvtMon:: Cec
