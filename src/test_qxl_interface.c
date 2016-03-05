#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static struct {
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
            dprint (2, "command que is full");
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
        dprint(1, "ignored");
        return;
    }
    dprint(3, "called");
    qxl->worker = qxl_worker;
    // Deprecated
    //qxl_worker->add_memslot(qxl_worker, &slot);
    spice_qxl_add_memslot(&qxl->display_sin, &slot);

    create_primary_surface(qxl, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    spice_server_vm_start(qxl->spice_server);
    qxl->worker_running = TRUE;
}
static void test_interface_set_compression_level (QXLInstance *sin, int level)
{
//    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "called");

    //FIXME implement
}
static void test_interface_set_mm_time(QXLInstance *sin, uint32_t mm_time)
{
//    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "called");

    //FIXME implement
}
static void test_interface_get_init_info(QXLInstance *sin, QXLDevInitInfo *info)
{
//    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "called");

    memset (info,0,sizeof(*info));

    info->num_memslots = NUM_MEMSLOTS;
    info->num_memslots_groups = NUM_MEMSLOTS_GROUPS;
    info->memslot_id_bits = MEMSLOT_ID_BITS;
    info->memslot_gen_bits = MEMSLOT_GEN_BITS;
    info->n_surfaces = NUM_SURFACES;
}
static int test_interface_get_command(QXLInstance *sin, struct QXLCommandExt *ext)
{
//    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);
    int count = commands.end - commands.start;

    memset (ext,0,sizeof(*ext));

    dprint(3, "called");

    if (count > 0) {
        *ext = *commands.vector[commands.start];
        ++commands.start;
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

    dprint(3, "called");

    qxl->core->timer_start (qxl->wakeup_timer, 10);

    /* This and req_cursor_notification needed for
     * client showing
     */
    return TRUE;
}
static void test_interface_release_resource(QXLInstance *sin,
                                       struct QXLReleaseInfoExt info)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);
    QXLCommandExt *ext;

    dprint(3, "called");

    assert (info.group_id == MEMSLOT_GROUP);
    ext = (QXLCommandExt*)(unsigned long)info.info->id;

    qxl->release_resource(qxl, ext);
}
static int test_interface_get_cursor_command(QXLInstance *sin, struct QXLCommandExt *ext)
{
//    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "called");

    //FIXME implemet

    return FALSE;
}
static int test_interface_req_cursor_notification(QXLInstance *sin)
{
//    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "called");

    //FIXME implemet

    /* This and req_cmd_notification needed for
     * client showing
     */
    return TRUE;
}
static void test_interface_notify_update(QXLInstance *sin, uint32_t update_id)
{
//    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "called");

    //FIXME implemet

}
static int test_interface_flush_resources(QXLInstance *sin)
{
//    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "called");

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

void test_init_qxl_interface (test_qxl_t *qxl)
{
    qxl->display_sin.base.sif = &test_qxl_interface.base;
    qxl->display_sin.id = 0;
    qxl->display_sin.st = (struct QXLState*)qxl;
    qxl->push_command = push_command;
}
