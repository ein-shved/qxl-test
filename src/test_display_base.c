#include <stdlib.h>
#include <string.h>

#include <spice/qxl_dev.h>

#include "test_display_base.h"
#include "test_qxl_device.h"
#include "common/ring.h"

typedef struct TestSurfaceCmd {
    QXLCommandExt ext; // first
    QXLSurfaceCmd surface_cmd;
} TestSurfaceCmd;

typedef struct TestCommandItem {
    RingItem link;
    TestCommand command;
} TestCommandItem;

Ring commands;

static void set_cmd(QXLCommandExt *ext, uint32_t type, QXLPHYSICAL data)
{
    ext->cmd.type = type;
    ext->cmd.data = data;
    ext->cmd.padding = 0;
    ext->group_id = MEMSLOT_GROUP;
    ext->flags = 0;
}
static void set_release_info(QXLReleaseInfo *info, intptr_t ptr)
{
    info->id = ptr;
    //info->group_id = MEMSLOT_GROUP;
}
static TestSurfaceCmd *create_surface(int surface_id, 
        const QXLRect *rect, uint8_t *data)
{
    int width = rect->right - rect->left;
    int height = rect->bottom - rect->top;
    TestSurfaceCmd *simple_cmd = calloc(sizeof(TestSurfaceCmd), 1);
    QXLSurfaceCmd *surface_cmd = &simple_cmd->surface_cmd;
    
    assert (width <= MAX_WIDTH && height <= MAX_HEIGHT);

    //Set the QXLCommandExt's type tp QXL_CMD_SURFACE and data
    //to surface_cmd.
    set_cmd(&simple_cmd->ext, QXL_CMD_SURFACE, (intptr_t)surface_cmd);
    set_release_info(&surface_cmd->release_info, (intptr_t)simple_cmd);
    surface_cmd->type = QXL_SURFACE_CMD_CREATE;
    surface_cmd->flags = 0; // ?
    surface_cmd->surface_id = surface_id;
    surface_cmd->u.surface_create.format = SPICE_SURFACE_FMT_32_xRGB;
    surface_cmd->u.surface_create.width = width;
    surface_cmd->u.surface_create.height = height;
    surface_cmd->u.surface_create.stride = -width * 4;
    surface_cmd->u.surface_create.data = (intptr_t)data;
    return simple_cmd;
}

static TestSurfaceCmd *destroy_surface(int surface_id)
{
    TestSurfaceCmd *simple_cmd = calloc(sizeof(TestSurfaceCmd), 1);
    QXLSurfaceCmd *surface_cmd = &simple_cmd->surface_cmd;

    set_cmd(&simple_cmd->ext, QXL_CMD_SURFACE, (intptr_t)surface_cmd);
    set_release_info(&surface_cmd->release_info, (intptr_t)simple_cmd);
    surface_cmd->type = QXL_SURFACE_CMD_DESTROY;
    surface_cmd->flags = 0; // ?
    surface_cmd->surface_id = surface_id;
    return simple_cmd;
}

void add_command (const TestCommand *command)
{
    TestCommandItem *item = calloc (sizeof(TestCommandItem),1);
    memcpy (&item->command, command, sizeof(TestCommand));
    ring_item_init(&item->link);
    ring_add_before (&item->link, &commands);
}
void remove_command (TestCommandItem *command)
{
    ring_remove (&command->link);
    free (command);
}
//from server/tests/test_display_base.c
void create_primary_surface( test_qxl_t* qxl,
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

    qxl->primary_width = width;
    qxl->primary_height = height;

    qxl_worker->create_primary_surface(qxl_worker, 0, &surface);
}

/*  Produce one command per call
 *  from command ring. Command
 *  indicator is qxl->current_command
 */
static void produce_command (test_qxl_t *qxl)
{
    TestCommand *command;
    TestCommandItem *item;
    TestSurfaceCmd *surface_cmd = NULL;
    QXLWorker *qxl_worker = qxl->worker;
    if ( qxl->current_command == NULL ) {       //If no commands in qxl struct
                                                //get first from commands ring
        qxl->current_command = ring_get_head (&commands);       
                                                //if no commands in ring
        if (qxl->current_command == NULL ) {
            dprint (2, "%s: command ring is empty\n", __func__);
            return;                             //quit
        }
    }

    item = (TestCommandItem *)qxl->current_command;
    command = &item->command;

    switch (command->type) {
    case COMMAND_SLEEP:
        dprint (2, "%s: sleeping for %d secs\n", __func__, command->sleep.secs);
        sleep (command->sleep.secs);

        break;
    case COMMAND_UPDATE: {
        dprint (2, "%s: update area\n", __func__);
        QXLRect rect = {
            .left = 0,
            .right = qxl->width,
            .top = 0,
            .bottom = qxl->height,
        };
        qxl_worker->update_area (qxl_worker, qxl->target_surface, &rect, NULL, 0 ,1);
        break;
    }

    case COMMAND_CREATE_SURFACE:
        dprint (2, "%s: create surface\n", __func__);
        qxl->target_surface = NUM_SURFACES - 1; 
        surface_cmd = create_surface ( qxl->target_surface, &command->create_surface.rect,
                                       qxl->secondary_surface.surface );
        qxl->height = command->create_surface.rect.bottom - 
                      command->create_surface.rect.top;
        qxl->width = command->create_surface.rect.right - 
                     command->create_surface.rect.left;
        qxl->has_secondary = TRUE;
        break;
    
    case COMMAND_DESTROY_SURFACE:
        dprint (2, "%s: destroy surface\n", __func__ );
        qxl->has_secondary = FALSE;
        surface_cmd = destroy_surface (qxl->target_surface);
        qxl->target_surface = 0;
        break;

    case COMMAND_CREATE_PRIMARY:
        dprint (2, "%s: crate primary\n", __func__ );
        create_primary_surface ( qxl,
                                 command->create_primary.rect.right - command->create_primary.rect.left,
                                 command->create_primary.rect.bottom - command->create_primary.rect.top );

        break;
    case COMMAND_DESTROY_PRIMARY:
        dprint (2, "%s: destroy primary\n",__func__ );
        qxl_worker->destroy_primary_surface(qxl_worker, 0);
    default:
        dprint (1, "%s: unknown command of type_code %d\n", __func__, command->type );
        break;
    }

    if (surface_cmd != NULL) {
        qxl->push_command (qxl, &surface_cmd->ext);
    }
    qxl->current_command = ring_next (&commands, &item->link); 
    if (command->times > 0) {
        if ( --command->times == 0) {
            remove_command (item); 
        }
    }
}

void test_qxl_display_init (test_qxl_t *qxl) {
    ring_init (&commands);
    qxl->current_command = ring_get_head(&commands);
    qxl->target_surface = 0;
    qxl->produce_command = produce_command;
}
