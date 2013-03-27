#include <spice.h>
#include <assert.h>

#include "test_qxl_device.h"
#include "test_basic_event_loop.h"
#include "test_display_base.h"

#define PR_WIDTH 640
#define PR_HEIGHT 480

#define STRIDE 1

#define OBJ_WIDTH 320
#define OBJ_HEIGHT 240

#define OBJ_POS_X 160
#define OBJ_POS_Y 120

static uint32_t image_id;
static int32_t obj_x = PR_WIDTH/2;
static int32_t obj_y = PR_HEIGHT/2;
uint8_t buffer[OBJ_HEIGHT*OBJ_WIDTH*4 ];

static void cg_1 (void *opaque, TestCommand *command)
{
    test_qxl_t *qxl = (test_qxl_t *)opaque;
    QXLRect rect = {
        .left   = obj_x - OBJ_WIDTH/2,
        .right  = obj_x + OBJ_WIDTH/2,
        .top    = obj_y - OBJ_HEIGHT/2,
        .bottom = obj_y + OBJ_HEIGHT/2,
    };
    command->draw.rect = rect;
    /*obj_y += STRIDE;
    if (obj_y > PR_HEIGHT+OBJ_HEIGHT/2) {
        obj_y = -OBJ_HEIGHT/2;
    }*/
}
static void cg_2 (void *opaque, TestCommand *command)
{
    test_qxl_t *qxl = (test_qxl_t *)opaque;
    QXLRect rect = {
        .left   = obj_x - OBJ_WIDTH/2,
        .right  = obj_x + OBJ_WIDTH/2,
        .top    = obj_y - OBJ_HEIGHT/2 - STRIDE,
        .bottom = obj_y - OBJ_HEIGHT/2,
    };

    command->draw.rect = rect;
    
    obj_y += STRIDE;
    if (obj_y > PR_HEIGHT+OBJ_HEIGHT/2) {
        obj_y = -OBJ_HEIGHT/2;
    }
}
static void do_wakeup (void *opaque)
{

    test_qxl_t *qxl = opaque;
    qxl->produce_command(qxl);
    qxl->worker->wakeup(qxl->worker);
    qxl->core->timer_start (qxl->wakeup_timer, 1);
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

    dprint (2, "server init done");
}

static void test_qxl_spice_server_init ( test_qxl_t *qxl )
{
    spice_server_add_interface (qxl->spice_server, &qxl->display_sin.base);

    dprint (2, "interface added");
}

static void fill_commands(test_qxl_t *qxl)
{
    TestCommand cmd;

    QXLRect prim_rect = {
        .left = 0,
        .right = PR_WIDTH,
        .top = 0,
        .bottom = PR_HEIGHT,
    };
    QXLRect rect = {
        .left = OBJ_POS_X - OBJ_WIDTH/2,
        .right = OBJ_POS_X + OBJ_WIDTH/2,
        .top = OBJ_POS_Y - OBJ_HEIGHT/2,
        .bottom = OBJ_POS_Y + OBJ_HEIGHT/2,
    };
    
    cmd.type    = COMMAND_DESTROY_PRIMARY;
    cmd.times   = 1;
    add_command(&cmd);

    cmd.type                = COMMAND_CREATE_PRIMARY;
    cmd.times               = 1;
    cmd.create_primary.rect = prim_rect;
    add_command(&cmd);

    draw_command_init (&cmd);
    cmd.draw.type   = COMMAND_DRAW_FILL;
    cmd.draw.rect   = prim_rect;
    cmd.draw.color  = COLOR_RGB(0,0,0);
    cmd.times       = 0;
//    add_command (&cmd);

    draw_command_init (&cmd);
    cmd.draw.type   = COMMAND_DRAW_SOLID;
    cmd.draw.rect   = rect;
    cmd.draw.color  = COLOR_RGB(200,127,0);
    cmd.draw.cg     = cg_1;
    cmd.draw.opaque = qxl;
    cmd.times       = 0;
    add_command (&cmd);

    draw_command_init (&cmd);
    cmd.draw.type   = COMMAND_DRAW_SOLID;
    cmd.draw.rect   = rect;
    cmd.draw.color  = COLOR_RGB(0,0,0); 
    cmd.draw.cg     = cg_2;
    cmd.draw.opaque = qxl;
    cmd.times       = 0;
    add_command (&cmd);

    cmd.type    = COMMAND_UPDATE;
    cmd.times   = 0;
    add_command (&cmd);
    
    cmd.type = COMMAND_SLEEP;
    cmd.sleep.secs = 1;
    cmd.times = 0;
    //add_command(&cmd);
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

    fill_commands(qxl);

    //Launch worker
    qxl->worker->start (qxl->worker);
    dprint (3, "worker launched");
    qxl->worker_running = TRUE;

    basic_event_loop_mainloop();
    free_commands();
}

