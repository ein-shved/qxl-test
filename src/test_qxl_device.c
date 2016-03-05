#include <string.h>
#include <stdlib.h>

#include <spice.h>
#include <assert.h>

#include "test_qxl_device.h"
#include "test_basic_event_loop.h"
#include "test_display_base.h"

#define PR_WIDTH 640
#define PR_HEIGHT 480

#define STRIDE 1

#define OBJ_WIDTH 322
#define OBJ_HEIGHT 242

#define OBJ_POS_X 160
#define OBJ_POS_Y 120

#define check_argument(arg,long_str,short_str) \
    !strcmp((arg),"--"long_str) || !strcmp((arg),"-"short_str)

static int32_t obj_x = PR_WIDTH/2;
static int32_t obj_y = PR_HEIGHT + OBJ_HEIGHT;
static double obj_width = OBJ_WIDTH;
static uint8_t one_bitmap [OBJ_WIDTH*OBJ_HEIGHT*4];

static QXLRect global_rect = {
        .left   = PR_WIDTH/2 - OBJ_WIDTH/2,
        .right  = PR_WIDTH/2 + OBJ_WIDTH/2,
        .top    = PR_HEIGHT/2 - OBJ_HEIGHT/2,
        .bottom = PR_HEIGHT/2 + OBJ_HEIGHT/2,
};

static void fill_bitmap (uint8_t *bitmap) {
    uint32_t *dst = (uint32_t *) bitmap;
    int r=256;
    int stride = 8;
    int i;

    for (i=0; i<OBJ_WIDTH*OBJ_HEIGHT; ++i, ++dst) {
        r = (r-stride);
        if ( r <= 0 || r >= 256 ) {
            stride = -stride;
        }
        if ( r >=256) {
            r=255;
        }
        if ( r<=0) {
            r=0;
        }
        *dst = COLOR_RGB(r,r,r);
    }
}

static void cg_1 (void *opaque, TestCommand *command)
{
    QXLRect rect = {
        .left   = obj_x - obj_width/2,
        .right  = obj_x + obj_width/2,
        .top    = obj_y - OBJ_HEIGHT/2,
        .bottom = obj_y + OBJ_HEIGHT/2,
    };
    command->draw.rect = rect;
    command->draw.clip_rects.ptr            = &global_rect;
    command->draw.clip_rects.destroyable    = FALSE;
    command->draw.clip_rects.num_rects      = 0;

/*    obj_y += STRIDE;
    if (obj_y > PR_HEIGHT+OBJ_HEIGHT/2) {
        obj_y = -OBJ_HEIGHT/2;
    }*/
}
static void cg_2 (void *opaque, TestCommand *command)
{
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

static void cg_clip_3 (void *opaque, TestCommand *command)
{
    QXLRect rect = {
        .left   = obj_x - OBJ_WIDTH/2,
        .right  = obj_x + OBJ_WIDTH/2,
        .top    = obj_y - OBJ_HEIGHT/2 - STRIDE,
        .bottom = obj_y - OBJ_HEIGHT/2,
    };
    global_rect = rect;

    command->draw.clip_rects.num_rects = 1;
    command->draw.clip_rects.ptr = &global_rect;
    command->draw.clip_rects.destroyable = FALSE;

    obj_y += STRIDE;
    if (obj_y > PR_HEIGHT+OBJ_HEIGHT/2) {
        obj_y = -OBJ_HEIGHT/2;
    }
}
static void do_wakeup (void *opaque)
{

    test_qxl_t *qxl = opaque;
    qxl->produce_command(qxl);
    // Depricated
    //qxl->worker->wakeup(qxl->worker);
    qxl->core->timer_start (qxl->wakeup_timer, 1);
    spice_qxl_wakeup(&qxl->display_sin);
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

    dprint (1, "spice server created. Addres: '%s' Port: '%d'",
        ops->addr, ops->port);

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
        .left = 0 - OBJ_WIDTH/2,
        .right = 0 + OBJ_WIDTH/2,
        .top = 0 - OBJ_HEIGHT/2,
        .bottom = 0 + OBJ_HEIGHT/2,
    };

    cmd.type    = COMMAND_CREATE_SURFACE;
    cmd.create_surface.rect = prim_rect;
    cmd.times   = 1;
    add_command(&cmd);

    draw_command_init (&cmd);
    cmd.draw.type           = COMMAND_DRAW_FILL;
    cmd.draw.rect           = prim_rect;
    cmd.draw.color          = COLOR_RGB (255,255,255);
    cmd.draw.dst_surface    = 1;
    cmd.times               = 0;
    add_command(&cmd);

    draw_command_init (&cmd);
    cmd.draw.type           = COMMAND_DRAW_SURFACE;
    cmd.draw.src_surface    = 1;
    cmd.draw.dst_surface    = 0;
    cmd.times               = 0;
    cmd.draw.rect           = rect;
    add_command(&cmd);

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
    cmd.draw.color  = COLOR_RGB(200,127,0);
    cmd.draw.cg     = cg_clip_3;
    cmd.times       = 0;
//    add_command (&cmd);

    fill_bitmap (one_bitmap);
    draw_command_init (&cmd);
    cmd.draw.type   = COMMAND_DRAW_FROM_BITMAP;
    cmd.draw.rect   = rect;
    cmd.draw.bitmap.ptr         = one_bitmap;
    cmd.draw.bitmap.destroyable = FALSE;
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
int parse_args(int argc, char *argv[], TestServerOpts *ops ) {
    int i;
    int ret = 0;
    for (i=1; i < argc; ++i) {
        if (check_argument(argv[i],"port","p")) {
            if ( i+1 >= argc ) {
                ret=-1;
                goto port_error;
            }
            char *endptr = argv[++i];
            ops->port = strtoul(argv[i], &endptr, 0);
            if (*endptr != '\0') {
                --i;
                ret=-1;
                goto port_error;
            }
        } else
        if (check_argument(argv[i],"addr","a")) {
            if ( i+1 >=argc ) {
                goto addr_error;
            }
            ops->addr = argv[++i];
        } else
        if (check_argument(argv[i],"help","h")) {
            printf (
                "Usage: qxl-test [OPTIONS]..\n"
                "Test programm for spice server\n"
                "\n"
                "Options is:\n"
                "\t-p\t--port=PORT\tuse specify port for server [5912]\n"
                "\t-a\t--addr=ADDRESS\tuse specify address for server [localhost]\n"
                "\t-h\t--help\t\tshow this message and exit\n"
                );
            ret=1;
        }
    }

    return ret;
port_error:
    fprintf (stderr, "Need port number after '%s' option\n", argv[i] );
    return ret;
addr_error:
    fprintf (stderr, "Need addr after '%s' option\n", argv[i] );
    return ret;
}

int main (int argc, char *argv[])
{
    test_qxl_t *qxl;
    SpiceCoreInterface *core;
    const char *host = "localhost";
    int port = 5912;
    int ret;
    TestServerOpts ops = {
        .addr = host,
        .flags = 0,
        .port = port,
        .no_auth = TRUE,
    };

    ret = parse_args (argc, argv, &ops);

    switch (ret){
    case 1:
        return 0;
        break;
    case 0:
        break;
    default:
        fprintf(stderr,"\tTry '--help' for more information\n");
        return ret;
        break;
    }

    core = basic_event_loop_init();
    qxl = new_test (core);
    test_qxl_display_init(qxl);
    test_init_qxl_interface(qxl);
    test_qxl_spice_server_new(qxl, &ops);
    test_qxl_spice_server_init(qxl);

    fill_commands(qxl);

    //Deprecated
    //Launch worker
    //qxl->worker->start (qxl->worker);
    //dprint (3, "worker launched");

    basic_event_loop_mainloop();
    free_commands();
}

