#include <stdio.h>
#include <stddef.h>


#include "test_qxl_device.h"

//FIXME reuse from other source (no copy-paste)
#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - offsetof(type, member))
#endif //container_of


#define NUM_MEMSLOTS        1
#define NUM_MEMSLOTS_GROUPS 1
#define NUM_SURFACES        128
#define MEMSLOT_ID_BITS     1
#define MEMSLOT_GEN_BITS    1

#define DEFAULT_WIDTH   256
#define DEFAULT_HEIGHT  256

#define MEMSLOT_GROUP 0

#define NUM_MAX_COMMANDS 1024

static struct {
    TestCommand ring [NUM_MAX_COMMANDS];
    int start;
    int end;
} commands = {
    .start = 0,
    .end = 0,
};

#define ASSERT_COMMANDS assert (\
    commands.end - commands.start < NUM_MAX_COMMANDS &&\
    commands.end > commands.start );

static void add_command (const TestCommand *command)
{
    ++commands.end;
    ASSERT_COMMANDS;
    commands.ring[commands.end % NUM_MAX_COMMANDS] = *command;
}
static const TestCommand * get_command (int cmd_index)
{
    int cmd_num = commands.start + cmd_index;
    assert (cmd_num < commands.end);
    return commands.ring + (cmd_num) % NUM_MAX_COMMANDS;
}

//from server/tests/test_display_base.c
static void create_primary_surface( test_qxl_t* qxl,
                                    uint32_t width, uint32_t height )
{
    QXLWorker *qxl_worker = qxl->worker;
    QXLDevSurfaceCreate surface = { 0, };

    ASSERT(height <= MAX_HEIGHT);
    ASSERT(width <= MAX_WIDTH);
    ASSERT(height > 0);
    ASSERT(width > 0);

    surface.format     = SPICE_SURFACE_FMT_32_xRGB;
    surface.width      = qxl->primary_width = width;
    surface.height     = qxl->primary_height = height;
    surface.stride     = -width * 4; /* negative? */
    surface.mouse_mode = FALSE; /* unused by red_worker */
    surface.flags      = 0;
    surface.type       = 0;    /* unused by red_worker */
    surface.position   = 0;    /* unused by red_worker */
    surface.mem        = (uint64_t)&qxl->primary_surface;
    surface.group_id   = MEMSLOT_GROUP;

    //test->width = width;
    //test->height = height;

    qxl_worker->create_primary_surface(qxl_worker, 0, &surface);
}


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

static void test_interface_attache_worker (QXLInstance *sin, QXLWorker *qxl_worker)
{
    static int count = 0;
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    if (++count > 1) { //Only one worker per session
        dprint(2, "%s: ignored\n", __FUNCTION__);
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
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "%s:\n", __FUNCTION__);
 
    //FIXME implemet (make command ring)

    return FALSE;
}
static int test_interface_req_cmd_notification(QXLInstance *sin)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return 0;
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

    return 0;
}
static int test_interface_req_cursor_notification(QXLInstance *sin)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(3, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return 0;
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

void produce_command (test_qxl_t *qxl)
{
    int num_commands = commands.end - commands.start;
    const TestCommand *command;

    dprint (1, "%s:", __func__);

    if (num_commands == 0) {
        dprint (2, "%s: commands ring is empty", __func__);
        return;
    }
    command = get_command (qxl->cmd_index); 

    switch (command->type) {
    case COMMAND_SLEEP:
        dprint (2, "%s: sleeping", __func__);
        sleep (command->sleep.secs);
        break;
    case COMMAND_UPDATE:
        break;
    }
    qxl->cmd_index = (qxl->cmd_index + 1) % num_commands;

}

void test_init_qxl_interface (test_qxl_t *qxl)
{
    qxl->display_sin.base.sif = &test_qxl_interface.base;
    qxl->display_sin.id = 0;
    qxl->display_sin.st = (struct QXLState*)qxl;
    qxl->cmd_index = 0;

    qxl->produce_command = produce_command;
}
