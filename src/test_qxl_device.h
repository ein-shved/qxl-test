#ifndef TEST_QXL_DEVICE_H
#define TEST_QXL_DEVICE_H

#include <spice.h>
#include <stdio.h>
#include <assert.h>
#include <spice/qxl_dev.h>
#include <spice/macros.h>

#include "common/ring.h"
#include "test_qxl_config.h"

#define NOT_IMPLEMENTED printf("%s not implemented\n", __func__);

#define dprint(lvl, fmt, ...) \
    if ((lvl) <= DEBUG) printf("%s: " fmt "\n", __func__, ## __VA_ARGS__)

#define __FUNCTION__ __func__
#define ASSERT assert

#define MAX_HEIGHT 1024
#define MAX_WIDTH 1024

#define container_of(ptr, type, member) \
    SPICE_CONTAINEROF(ptr, type, member)

#define COLOR_RGB(r,g,b) \
    (((r)<<16) | ((g)<<8) | ((b)<<0))

#define NUM_MEMSLOTS        1
#define NUM_MEMSLOTS_GROUPS 1
#define NUM_SURFACES        2
#define MEMSLOT_ID_BITS     1
#define MEMSLOT_GEN_BITS    1

#define DEFAULT_WIDTH   256
#define DEFAULT_HEIGHT  256

#define MEMSLOT_GROUP 0

#define NUM_MAX_COMMANDS 1024

typedef uint32_t color_t;

typedef struct _test_surface_t {
    //from server/tests/test_display_base.c
    uint8_t surface [ MAX_HEIGHT * MAX_WIDTH * 4 ];
} test_surface_t;
typedef struct _test_qxl_t {
    QXLInstance display_sin;
    QXLWorker *worker;
    SpiceCoreInterface *core;
    SpiceServer *spice_server;

    int worker_running;

    SpiceTimer *wakeup_timer;

    void (*produce_command) (struct _test_qxl_t*);
    int (*push_command) (struct _test_qxl_t*, QXLCommandExt *);
    void (*release_resource) (struct _test_qxl_t*, QXLCommandExt *);

    RingItem *current_command;

    test_surface_t primary_surface;
    int primary_width;
    int primary_height;

    uint32_t target_surface;
    int has_secondary;
    test_surface_t secondary_surface;
    int width;
    int height;
} test_qxl_t;

QXLInterface *test_init_qxl(void);
void test_init_qxl_interface (test_qxl_t *qxl);

#endif //TEST_QXL_DEVICE_H
