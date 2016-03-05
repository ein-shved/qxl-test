#ifndef TEST_DISPLAY_BASE_H
#define TEST_DISPLAY_BASE_H

#include "test_qxl_device.h"
#include "test_qxl_config.h"

//Command types
typedef struct TestCommand TestCommand;
typedef void (*command_gen) (void *opaque, TestCommand* command);

typedef enum TestCommandType {
    COMMAND_SLEEP,
    COMMAND_UPDATE,
    COMMAND_CREATE_SURFACE,
    COMMAND_DESTROY_SURFACE,
    COMMAND_CREATE_PRIMARY,
    COMMAND_DESTROY_PRIMARY,
    COMMAND_DRAW,
    COMMAND_CONTROL,
    COMMAND_SWITCH_SURFACE,
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
        COMMAND_DRAW_SURFACE,
        COMMAND_DRAW_CACHED,
        COMMAND_DRAW_FILL,
} TestCommandDrawType;
typedef enum TestImageCached {
    TEST_IMAGE_NO_CACHE,
    TEST_IMAGE_CACHE,
} TestImageCached;
typedef struct TestBitmap {
    uint8_t *ptr;
    int destroyable;
} TestBitmap;
typedef struct TestClipList {
    uint32_t num_rects;
    QXLRect *ptr;
    int destroyable;
} TestClipList;
typedef struct TestCommandDraw {
    TestCommandDrawType type;
    QXLRect rect;
    TestImageCached cache;
    uint32_t image_id;  /* Must be zero first time
                         * reused on other steps.
                         */
    command_gen cg;
    void *opaque;
    uint32_t dst_surface;
    union {
        struct { //for fill and bitmap
            uint32_t color;
            TestBitmap bitmap;
        };
        uint32_t src_surface; //for surface
    };
    TestClipList clip_rects;
} TestCommandDraw;
typedef enum TestCommandControlType{
    //TODO: implement or remove
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
    COMMAND_CONTROL_IGNORE,         //Do nothing

} TestCommandControlType;
typedef struct TestCommandControl {
    TestCommandControlType type;
    command_gen cg;
    void *opaque;
    RingItem *pos;  //not need for ABS commands.
} TestCommandControl;
typedef struct TestCommandSwitchSurface {
    uint32_t surface_id;
} TestCommandSwitchSurface;

struct TestCommand {
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
        TestCommandSwitchSurface switch_surface;
    };
};

typedef struct TestCommandItem TestCommandItem;

typedef struct TestNewCommand { //used by simple_command_gen as opaque
    TestCommand command;
    int times; /* if 0 - never freed
                * else - freed after times calls
                * should be equal to times of command, or 0
                */
} TestNewCommand;

void draw_command_init (TestCommand *command);
void siple_command_gen (void *opaque, TestCommand *command);

void test_qxl_display_init (test_qxl_t *qxl);
RingItem *add_command (const TestCommand *command);
void remove_command (RingItem *item);
TestCommand *get_next_command (RingItem *item);

void create_primary_surface ( test_qxl_t *qxl,
                              uint32_t width, uint32_t height );
void free_commands ();

#endif //TEST_DISPLAY_BASE_H
