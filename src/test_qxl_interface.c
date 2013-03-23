#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "test_qxl_device.h"
#include "common/ring.h"
#include "test_display_base.h"

//Not actually need.
QXLDevMemSlot slot = {
.slot_group_id = MEMSLOT_GROUP,
.slot_id = 0,
.generation = 0,
.virt_start = 0,
.virt_end = ~0,
.addr_delta = 0,
.qxl_ram_size = ~0, //TODO: learn what this: ~
};

#define MAX_COMMAND_NUM 1024
#define MAX_WAIT_ITERATIONS 10
#define WAIT_ITERATION_TIME 1

struct {
    QXLCommandExt *vector [MAX_COMMAND_NUM];
    int start;
    int end;
} commands = { 
    .start = 0,
    .end = 0,
};

#define ASSERT_COMMANDS assert (\
    (commands.end - commands.start <= MAX_COMMAND_NUM) && \
    (commands.end >= commands.start) )

static int push_command (test_qxl_t *qxl, QXLCommandExt *cmd)
{
    int i = 0;
    int count;

    ASSERT_COMMANDS;
    
    while ( (count  = commands.end - commands.start) >= MAX_COMMAND_NUM) {
        //may be decremented from worker thread.
        if (i >= MAX_WAIT_ITERATIONS) {
            dprint (2, "%s: command que is full\n", __func__);
            return FALSE;
        }
        ++i;
        sleep(WAIT_ITERATION_TIME);
    }
    commands.vector[commands.end % MAX_COMMAND_NUM] = cmd;
    ++commands.end;
    return TRUE;
}

static void test_interface_attache_worker (QXLInstance *sin, QXLWorker *qxl_worker)
{
    static int count = 0;
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    if (++count > 1) { //Only one worker per session
        dprint(1, "%s: ignored\n", __FUNCTION__);
        return;
    }
    qxl_worker->add_memslot(qxl_worker, &slot);

    dprint(2, "%s:\n", __FUNCTION__);
    qxl->worker = qxl_worker;
    create_primary_surface(qxl, DEFAULT_WIDTH, DEFAULT_HEIGHT);
}
static void test_interface_set_compression_level (QXLInstance *sin, int level)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(2, "%s:\n", __FUNCTION__);
 
    //FIXME implement
}
static void test_interface_set_mm_time(QXLInstance *sin, uint32_t mm_time)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "%s:\n", __FUNCTION__);

    //FIXME implement 
}
static void test_interface_get_init_info(QXLInstance *sin, QXLDevInitInfo *info)
{
    memset (info,0,sizeof(*info));
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(2, "%s:\n", __FUNCTION__);
    
    info->num_memslots = NUM_MEMSLOTS;
    info->num_memslots_groups = NUM_MEMSLOTS_GROUPS;
    info->memslot_id_bits = MEMSLOT_ID_BITS;
    info->memslot_gen_bits = MEMSLOT_GEN_BITS;
    info->n_surfaces = NUM_SURFACES;

    //FIXME implement
}
static int test_interface_get_command(QXLInstance *sin, struct QXLCommandExt *ext)
{
    memset (ext,0,sizeof(*ext));
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);
    int count = commands.end - commands.start;

    dprint(3, "%s:\n", __FUNCTION__);
    
    if (count > 0) {
        *ext = *commands.vector[commands.start];
        ++commands.start;
        free (commands.vector[commands.start - 1]);
        if ( commands.start >= MAX_COMMAND_NUM ) {
            commands.start %= MAX_COMMAND_NUM;
            commands.end %= MAX_COMMAND_NUM;
        }
        ASSERT_COMMANDS;
        return TRUE;
    }

    return FALSE;
}
static int test_interface_req_cmd_notification(QXLInstance *sin)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(2, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return TRUE;
}
static void test_interface_release_resource(QXLInstance *sin,
                                       struct QXLReleaseInfoExt ext)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

}
static int test_interface_get_cursor_command(QXLInstance *sin, struct QXLCommandExt *ext)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return FALSE;
}
static int test_interface_req_cursor_notification(QXLInstance *sin)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(2, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return TRUE;
}
static void test_interface_notify_update(QXLInstance *sin, uint32_t update_id)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

}
static int test_interface_flush_resources(QXLInstance *sin)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return 0;
}
static QXLInterface test_qxl_interface = {
    .base.type                  = SPICE_INTERFACE_QXL,
    .base.description           = "test qxl gpu",
    .base.major_version         = SPICE_INTERFACE_QXL_MAJOR,
    .base.minor_version         = SPICE_INTERFACE_QXL_MINOR,

    .attache_worker             = test_interface_attache_worker,
    .set_compression_level      = test_interface_set_compression_level,
    .set_mm_time                = test_interface_set_mm_time,
    .get_init_info              = test_interface_get_init_info,

    .get_command                = test_interface_get_command,
    .req_cmd_notification       = test_interface_req_cmd_notification,
    .release_resource           = test_interface_release_resource,
    .get_cursor_command         = test_interface_get_cursor_command,
    .req_cursor_notification    = test_interface_req_cursor_notification,
    .notify_update              = test_interface_notify_update,
    .flush_resources            = test_interface_flush_resources,
};

//TODO implement worker functions



static void fill_commands()
{
    TestCommand cmd;

    QXLRect rect = {
        .left = 0,
        .right = 640,
        .top = 0,
        .bottom = 480,
    };
    
    cmd.type = COMMAND_DESTROY_PRIMARY;
    cmd.times = 1;
    add_command(&cmd);

    cmd.type = COMMAND_CREATE_PRIMARY;
    cmd.times = 1;
    cmd.create_primary.rect = rect;
    add_command(&cmd);

    rect.right = 320;
    rect.bottom = 240;
    cmd.type = COMMAND_CREATE_SURFACE;
    cmd.create_surface.rect = rect;
    cmd.times = 1;
    add_command (&cmd);

    cmd.type = COMMAND_UPDATE;
    cmd.times = 1;
    add_command (&cmd);

    cmd.type = COMMAND_SLEEP;
    cmd.sleep.secs = 1;
    cmd.times = 0;
    add_command(&cmd);
}

void test_init_qxl_interface (test_qxl_t *qxl)
{
    qxl->display_sin.base.sif = &test_qxl_interface.base;
    qxl->display_sin.id = 0;
    qxl->display_sin.st = (struct QXLState*)qxl;
    qxl->push_command = push_command;    

    fill_commands();
}
