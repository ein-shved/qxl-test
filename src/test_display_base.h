#ifndef TEST_DISPLAY_BASE_H
#define TEST_DISPLAY_BASE_H

#include "test_qxl_device.h"
#include "test_qxl_config.h"

//Command types
typedef (*command_gen) (void *opaque, TestCommand* command);

typedef enum TestCommandType {
    COMMAND_SLEEP,
    COMMAND_UPDATE,
    COMMAND_CREATE_SURFACE,
    COMMAND_DESTROY_SURFACE,
    COMMAND_CREATE_PRIMARY,
    COMMAND_DESTROY_PRIMARY,
    COMMAND_DRAW,
    COMMAND_CONTROL,
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
typedef enum TestCommandDrawType {
        COMMAND_DRAW_FROM_BITMAP,
        COMMAND_DRAW_SOLID,
} TestCommandDrawType;
typedef struct TestBitmap {
    uint8_t *ptr;
    int destroyable;
} TestBitmap;
typedef struct TestCommandDraw {
    TestCommandDrawType type;
    QXLRect rect;
    union {
        uint32_t color;
        TestBitmap bitmap;
    };
} TestCommandDraw;
typedef enum TestCommandControlType{
    COMMAND_CONTROL_PUSH_LAST,      //ABS
    COMMAND_CONTROL_PUSH_FIRST,     //ABS
    COMMAND_CONTROL_PUSH,           //REL
    COMMAND_CONTROL_PUSH_NEXT,      //ABS (rel current)
    COMMAND_CONTROL_PUSH_PREV,      //ABS (rel current)
    COMMAND_CONTROL_REMOVE_LAST,    //ABS
    COMMAND_CONTROL_REMOVE_FIRST,   //ABS
    COMMAND_CONTROL_REMOVE,         //REL
    COMMAND_CONTROL_REMOVE_NEXT,    //ABS (rel current)
    COMMAND_CONTROL_REMOVE_PREV,    //ABS (rel current)
    COMMAND_CONTROL_USE_INSTEAD,    /* gets command from cg and
                                     * use it instead currend, without
                                     * adding to ring
                                     */
} TestCommandControlType;
typedef struct TestCommandControl {
    TestCommandControlType type;
    union {
        command_gen *cg;
        void *opaque;
        RingItem *pos;  //not need for ABS commands.
    };
} TestCommandControl;

typedef struct TestCommand {
    TestCommandType type;
    int times;  /* if =0 then infinitely
                 * else, command will removed after times execs
                 */
    union {
        TestCommandSleep sleep;
        TestCommandUpdate update;
        TestCommandCreateSurface create_surface;
        TestCommandCreateSurface create_primary;
        TestCommandDraw draw;
        TestCommandControl control;
    };
} TestCommand;

typedef struct TestCommandItem TestCommandItem;

typedef struct TestNewCommand { //used by simple_command_gen as opaque
    TestCommand command;
    int times; /* if 0 - never freed
                * else - freed after times calls
                * should be equal to times of command, or 0
                */
} TestNewCommand;
void siple_command_gen (void *opaque, TestCommand *command);

void test_qxl_display_init (test_qxl_t *qxl);
void add_command (const TestCommand *command);
void remove_command (TestCommandItem *item);
void create_primary_surface ( test_qxl_t *qxl,
                              uint32_t width, uint32_t height );
void free_commands ();

#endif //TEST_DISPLAY_BASE_H
