#ifndef TEST_DISPLAY_BASE_H
#define TEST_DISPLAY_BASE_H

#include "test_qxl_device.h"

typedef struct TestCommandItem TestCommandItem;

void test_qxl_display_init (test_qxl_t *qxl);
void add_command (const TestCommand *command);
void remove_command (TestCommandItem *item);
void create_primary_surface ( test_qxl_t *qxl,
                              uint32_t width, uint32_t height );

#endif //TEST_DISPLAY_BASE_H
