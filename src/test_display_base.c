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
typedef struct TestSpiceUpdate {
    QXLCommandExt ext; //first
    QXLDrawable drawable;
    QXLImage image;
    TestBitmap bitmap;
} TestSpiceUpdate;

typedef struct TestCommandItem {
    RingItem link;
    TestCommand command;
} TestCommandItem;

static Ring commands;
static int image_unique = 1;

static void set_cmd(QXLCommandExt *ext, uint32_t type, QXLPHYSICAL data)
{
    ext->cmd.type = type;
    ext->cmd.data = data;
    ext->cmd.padding = 0;
    ext->group_id = MEMSLOT_GROUP;
    ext->flags = 0;
}
static void test_set_release_info(QXLReleaseInfo *info, intptr_t ptr)
{
    info->id = ptr;
    //info->group_id = MEMSLOT_GROUP;
}
static void release_resource (test_qxl_t *qxl, QXLCommandExt *ext)
{
    switch (ext->cmd.type) {
    case QXL_CMD_SURFACE: {
        TestSurfaceCmd *cmd = container_of (ext, TestSurfaceCmd, ext);
        free (cmd); //Can do free(ext);
        break;
    }

    case QXL_CMD_DRAW: {
        TestSpiceUpdate *update = container_of (ext, TestSpiceUpdate, ext);
        if (update->bitmap.destroyable) {
            free (update->bitmap.ptr);
        }
        free (update);
        break;
    }
        
    }
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
    test_set_release_info(&surface_cmd->release_info, (intptr_t)simple_cmd);
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
    test_set_release_info(&surface_cmd->release_info, (intptr_t)simple_cmd);
    surface_cmd->type = QXL_SURFACE_CMD_DESTROY;
    surface_cmd->flags = 0; // ?
    surface_cmd->surface_id = surface_id;
    return simple_cmd;
}

static TestSpiceUpdate *test_create_update_from_bitmap (test_qxl_t *qxl, 
                TestBitmap *bitmap, const QXLRect *bbox
                /*,int num_clip_rects, QXLRect* clip_rects */)
{
    TestSpiceUpdate *update;
    QXLImage *image;
    QXLDrawable *drawable;
    uint32_t bw, bh;

    bw = bbox->right - bbox->left;
    bh = bbox->bottom - bbox->top;

    update = calloc (sizeof(*update), 1);
    update->bitmap = *bitmap;
    drawable = &update->drawable;
    image = &update->image;

    drawable->surface_id = 0;//qxl->target_surface;
    drawable->bbox = *bbox;

    //TODO add here clip rects!
    drawable->clip.type = SPICE_CLIP_TYPE_NONE;

    drawable->effect            = QXL_EFFECT_OPAQUE;
    test_set_release_info (&drawable->release_info, (intptr_t)update);//same as &update->ext
    drawable->type              = QXL_DRAW_COPY;
    drawable->surfaces_dest[0]  = -1;
    drawable->surfaces_dest[1]  = -1;
    drawable->surfaces_dest[2]  = -1;

    drawable->u.copy.rop_descriptor     = SPICE_ROPD_OP_PUT;
    drawable->u.copy.src_bitmap         = (intptr_t)image;
    drawable->u.copy.src_area.top       = 0;
    drawable->u.copy.src_area.left      = 0;
    drawable->u.copy.src_area.right     = bw;
    drawable->u.copy.src_area.bottom    = bh;
    
    QXL_SET_IMAGE_ID (image, QXL_IMAGE_GROUP_DEVICE, image_unique);
    image->descriptor.type      = SPICE_IMAGE_TYPE_BITMAP;
    image->bitmap.flags         = QXL_BITMAP_DIRECT | QXL_BITMAP_TOP_DOWN;
    image->bitmap.stride        = bw * 4; //TODO what this?
    image->descriptor.width     = image->bitmap.x = bw;
    image->descriptor.height    = image->bitmap.y = bh;
    image->bitmap.data          = (intptr_t)bitmap->ptr;
    image->bitmap.palette       = 0;
    image->bitmap.format        = SPICE_BITMAP_FMT_32BIT;

    set_cmd (&update->ext, QXL_CMD_DRAW, (intptr_t)drawable);

    return update;
}

static TestSpiceUpdate *test_create_update_solid (test_qxl_t *qxl, 
                uint32_t color, const QXLRect *bbox )
{
    TestBitmap bitmap_str;
    uint32_t *dst;
    int bw, bh;
    int i;

    bw = bbox->right - bbox->left;
    bh = bbox->bottom - bbox->top;
    
    bitmap_str.destroyable = TRUE;
    bitmap_str.ptr = malloc (bw * bh * 4);
    dst = (uint32_t *) bitmap_str.ptr;

    for (i=0; i < bw * bh; ++i, ++dst) {
        *dst = color;
    }

    return test_create_update_from_bitmap (qxl, &bitmap_str, bbox);
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
void free_commands () 
{    
    RingItem *item, *next;
    Ring* ring = (Ring*)&commands;
    //FIXME something wrong with macros!
    for (item = ring_get_head(ring),                       
         next = (item != NULL) ? ring_next(ring, item) : NULL;
         item != NULL;                                   
    item = next, next = item != NULL ? ring_next(ring, item) : NULL) {
//    RING_FOREACH_SAVE(item, next, ring) {
        remove_command ((TestCommandItem *)item);
    }
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

    if (!qxl->has_secondary) {
        qxl->width = width;
        qxl->height = height;
    }

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
    TestCommand command_container;
    QXLWorker *qxl_worker = qxl->worker;

    if ( ring_is_empty(&commands) ) {
        qxl->current_command = NULL;
        return;
    }

    if ( qxl->current_command == NULL ) {       //If no commands in qxl struct
                                                //get first from commands ring
        qxl->current_command = ring_get_head (&commands);       
                                                //if no commands in ring
        if (qxl->current_command == NULL ) {
            dprint (3, "command ring is empty");
            return;                             //quit
        }
    }

    item = (TestCommandItem *)qxl->current_command;
    command = &item->command;

    if (command->type == COMMAND_CONTROL && 
        command->control.type == COMMAND_CONTROL_USE_INSTEAD ) {
        command->control.cg(command->control.opaque, &command_container);

    }

    switch (command->type) {
    case COMMAND_SLEEP:
        dprint (2, "sleeping for %d secs", command->sleep.secs);
        sleep (command->sleep.secs);

        break;
    case COMMAND_UPDATE: {
        dprint (2, "update area");
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
        dprint (2, "create surface");
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
        dprint (2, "destroy surface");
        qxl->has_secondary = FALSE;
        surface_cmd = destroy_surface (qxl->target_surface);
        qxl->target_surface = 0;
        break;

    case COMMAND_CREATE_PRIMARY:
        dprint (2, "crate primary");
        create_primary_surface ( qxl,
                                 command->create_primary.rect.right - command->create_primary.rect.left,
                                 command->create_primary.rect.bottom - command->create_primary.rect.top );

        break;

    case COMMAND_DESTROY_PRIMARY:
        dprint (2, "destroy primary");
        qxl_worker->destroy_primary_surface(qxl_worker, 0);
        break;
    
    case COMMAND_DRAW: {
        TestSpiceUpdate *update = NULL;
        switch (command->draw.type) {
        case COMMAND_DRAW_FROM_BITMAP: {
            dprint (2, "draw bitmap");
            update = test_create_update_from_bitmap (qxl, 
                        &command->draw.bitmap, &command->draw.rect);
            break;
        }
        case COMMAND_DRAW_SOLID: {
            dprint (2, "draw solid");
            update = test_create_update_solid (qxl,
                        command->draw.color, &command->draw.rect);
            break;
        }
        default:
            dprint (2, "unknown draw command");
            break;
        }

        if (update != NULL ) {
            qxl->push_command(qxl, &update->ext);
        }
        break;
    }

    default:
        dprint (1, "unknown command of type_code %d", command->type );
        break;
    }

    if (surface_cmd != NULL) {
        qxl->push_command (qxl, &surface_cmd->ext);
    }
    qxl->current_command = ring_next (&commands, &item->link); 
    if (command->times > 0) {
        if ( --command->times <= 0) {
            if (qxl->current_command == &item->link) {
                qxl->current_command = NULL;
            }
            remove_command (item); 
        }
    }
}

void siple_command_gen (void *opaque, TestCommand *command) {
    TestNewCommand *new_cmd = (TestNewCommand *)opaque;
    *command=new_cmd->command;
    if (new_cmd->times != 0) {
        if (--new_cmd->times == 0) {
            free (new_cmd);
        }
    }
}
void test_qxl_display_init (test_qxl_t *qxl) {
    ring_init (&commands);
    qxl->current_command = ring_get_head(&commands);
    qxl->target_surface = 0;
    qxl->produce_command = produce_command;
    qxl->release_resource = release_resource;
}
