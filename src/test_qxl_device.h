#ifndef TEST_QXL_DEVICE_H
#define TEST_QXL_DEVICE_H

#define DEBUG 2

#include <spice.h>
#include <stdio.h>
#include <assert.h>

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

typedef enum TestCommandType {
    COMMAND_SLEEP,
    COMMAND_UPDATE
} TestCommandType;
typedef struct TestCommandSleep {
    uint32_t secs;
} TestCommandSleep;
typedef struct TestCommandUpdate {
} TestCommandUpdate;
typedef struct TestCommand {
    TestCommandType type;
    union {
        TestCommandSleep sleep;
        TestCommandUpdate update;
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

    test_surface_t primary_surface;
    int primary_width;
    int primary_height;

    SpiceTimer *wakeup_timer;

    void (*produce_command) (struct _test_qxl_t*);

    int cmd_index;
};
typedef struct _test_qxl_t test_qxl_t;

QXLInterface *test_init_qxl(void);

#endif //TEST_QXL_DEVICE_H
