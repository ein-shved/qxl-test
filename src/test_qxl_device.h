#ifndef TEST_QXL_DEVICE_H
#define TEST_QXL_DEVICE_H

#define DEBUG 2

#include <spice.h>
#include <stdio.h>
#include <assert.h>
#include <spice/qxl_dev.h>

#ifndef BOOL
#   define BOOL int
#endif //BOOL

#ifndef TRUE
#   define TRUE 1
#endif //TRUE

#ifndef FALSE
#   define FALSE 0
#endif //FALSE

#define NOT_IMPLEMENTED printf("%s not implemented\n", __func__);

#define dprint(lvl, fmt, ...) \
    if (lvl <= DEBUG) printf(fmt, ## __VA_ARGS__)

#define __FUNCTION__ __func__
#define ASSERT assert

#define MAX_HEIGHT 1024
#define MAX_WIDTH 1024


//FIXME reuse from other source (no copy-paste)
#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - offsetof(type, member))
#endif //container_of


#define NUM_MEMSLOTS        1
#define NUM_MEMSLOTS_GROUPS 1
#define NUM_SURFACES        2
#define MEMSLOT_ID_BITS     1
#define MEMSLOT_GEN_BITS    1

#define DEFAULT_WIDTH   256
#define DEFAULT_HEIGHT  256

#define MEMSLOT_GROUP 0

#define NUM_MAX_COMMANDS 1024

typedef struct Ring RingItem;

//Command types
typedef enum TestCommandType {
    COMMAND_SLEEP,
    COMMAND_UPDATE,
    COMMAND_CREATE_SURFACE,
    COMMAND_DESTROY_SURFACE,
    COMMAND_CREATE_PRIMARY,
    COMMAND_DESTROY_PRIMARY,
    COMMAND_DRAW,
} TestCommandType;
typedef struct TestCommandSleep {
    uint32_t secs;
} TestCommandSleep;
typedef struct TestCommandUpdate {
} TestCommandUpdate;
typedef struct TestCommandCreateSurface {
    QXLRect rect;
} TestCommandCreateSurface;
typedef struct TestCommandDestroySurface {
} TestCommandDestroySurface;
typedef struct TestCommandDraw {
    enum {
        COMMAND_DRAW_FROM_BITMAP,
        COMMAND_DRAW_SIMPLE,
    } type;
    QXLRect rect;
    union {
        uint32_t color;
        struct TestBitmap {
            uint8_t *ptr;
            int destroyable;
        } bitmap;
    };
};

typedef struct TestCommand {
    TestCommandType type;
    int times;  //if =0 then infinitely
                //else, command will removed after times execs
    union {
        TestCommandSleep sleep;
        TestCommandUpdate update;
        TestCommandCreateSurface create_surface;
        TestCommandCreateSurface create_primary;
        TestCommandDraw draw;
    };
} TestCommand;


typedef struct _test_surface_t {
    //from server/tests/test_display_base.c
    uint8_t surface [ MAX_HEIGHT * MAX_WIDTH * 4 ]; 
} test_surface_t;
struct _test_qxl_t {
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

    int target_surface;
    int has_secondary;
    test_surface_t secondary_surface;
    int width;
    int height;
};
typedef struct _test_qxl_t test_qxl_t;

QXLInterface *test_init_qxl(void);

#endif //TEST_QXL_DEVICE_H
