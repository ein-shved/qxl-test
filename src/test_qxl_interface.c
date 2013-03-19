#include <stdio.h>
#include <stddef.h>

#include "test_qxl_device.h"

//FIXME reuse from other source (no copy-paste)
#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - offsetof(type, member))
#endif //container_of

static void test_interface_attache_worker (QXLInstance *sin, QXLWorker *qxl_worker)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);


    //TODO learn for what next line?
    //qxl_worker->add_memslot(qxl_worker, &my_slot);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
    qxl->worker = qxl_worker;
}
static void test_interface_set_compression_level (QXLInstance *sin, int level)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
    //FIXME implement
}
static void test_interface_set_mm_time(QXLInstance *sin, uint32_t mm_time)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
    //FIXME implement 
}
static void test_interface_get_init_info(QXLInstance *sin, QXLDevInitInfo *info)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
    
    //FIXME implement
}
static int test_interface_get_command(QXLInstance *sin, struct QXLCommandExt *ext)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return 0;
}
static int test_interface_req_cmd_notification(QXLInstance *sin)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return 0;
}
static void test_interface_release_resource(QXLInstance *sin,
                                       struct QXLReleaseInfoExt ext)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

}
static int test_interface_get_cursor_command(QXLInstance *sin, struct QXLCommandExt *ext)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return 0;
}
static int test_interface_req_cursor_notification(QXLInstance *sin)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

    return 0;
}
static void test_interface_notify_update(QXLInstance *sin, uint32_t update_id)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
    //FIXME implemet

}
static int test_interface_flush_resources(QXLInstance *sin)
{
    test_qxl_t *qxl = container_of(sin, test_qxl_t, display_sin);

    dprint(qxl, 1, "%s:\n", __FUNCTION__);
 
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

QXLInterface *test_init_qxl (void)
{
    return &test_qxl_interface;
}
