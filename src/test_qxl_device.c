#include <spice.h>
#include <assert.h>
//#include <qxl.h>
//#include <spiceqxl_spice_server.h>

#include "test_qxl_device.h"
#include "test_basic_event_loop.h"
#include "test_display_base.h"

static void do_wakeup (void *opaque)
{

    test_qxl_t *qxl = opaque;
    qxl->produce_command(qxl);
    qxl->worker->wakeup(qxl->worker);
    qxl->core->timer_start (qxl->wakeup_timer, 10);
}

static test_qxl_t *new_test ( SpiceCoreInterface *core)
{
    static int qxl_count = 0;
    SpiceTimer* wakeup_timer;
    static test_qxl_t _qxl;
    test_qxl_t *qxl = &_qxl;

    assert (++qxl_count<=1);

    qxl->core = core;

    wakeup_timer = core->timer_add(do_wakeup, qxl);
    qxl->wakeup_timer=wakeup_timer;
    
    return qxl;
}


struct TestServerOptsStr {
    const char* addr;
    int flags;
    int port;
    int no_auth;
};
typedef struct TestServerOptsStr TestServerOpts, *TestServerOptsPtr;

static void test_qxl_spice_server_new ( test_qxl_t *qxl, 
                                        const TestServerOptsPtr ops)
{
    //Init spice server
    qxl->spice_server = spice_server_new();
    
    spice_server_set_addr ( qxl->spice_server,
                            ops->addr, ops->flags );
    spice_server_set_port ( qxl->spice_server,
                            ops->port );
    if (ops->no_auth ) {
        spice_server_set_noauth ( qxl->spice_server );
    }

    //TODO set another spice server options here   

    spice_server_init (qxl->spice_server, qxl->core);

    dprint (2, "%s: server init done\n", __func__);
}

static void test_qxl_spice_server_init ( test_qxl_t *qxl )
{

    spice_server_add_interface (qxl->spice_server, &qxl->display_sin.base);

    dprint (2, "%s: interface added\n", __func__);

}

int main (int argc, const char *argv[])
{
    test_qxl_t *qxl;
    SpiceCoreInterface *core;
    const char *host = "localhost";
    int port = 5912;
    TestServerOpts ops = {
        .addr = host,
        .flags = 0,
        .port = port,
        .no_auth = TRUE,
    };

    core = basic_event_loop_init();
    qxl = new_test (core);
    test_qxl_display_init(qxl);
    test_init_qxl_interface(qxl);
    test_qxl_spice_server_new(qxl, &ops);
    test_qxl_spice_server_init(qxl);

    //Launch worker
    qxl->worker->start (qxl->worker);
    dprint (3, "%s: worker launched\n", __func__);
    qxl->worker_running = TRUE;

    basic_event_loop_mainloop();
    free_commands();
}

