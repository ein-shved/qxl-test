#include <stdlib.h>
#include <string.h>

#include <spice/qxl_dev.h>

#include "test_display_base.h"
#include "test_qxl_device.h"
#include "common/ring.h"

#define print_rect(lvl,rect)\
    dprint((lvl), "l:%d r:%d t:%d b:%d", (rect).left,\
    (rect).right, (rect).top, (rect).bottom)

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
static uint32_t image_unique = 1;

/* ================================================================
 * ===================== COMMAND EXECUTION ========================
 * ================================================================
 */

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
        switch (update->drawable.type) {
        case QXL_DRAW_COPY: {
            //update->image.descriptor.type == SPICE_IMAGE_TYPE_BITMAP
            if (update->bitmap.destroyable) {
                free (update->bitmap.ptr);
            }
            break;
        }
        free (update);
        break;
        }
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
static void test_fill_clip_data (QXLDrawable *drawable, TestClipList *clip_rects)
{
    if (clip_rects == NULL || clip_rects->num_rects == 0) {
        drawable->clip.type = SPICE_CLIP_TYPE_NONE;
    } else {
        QXLClipRects *cmd_clip;

        cmd_clip = calloc (sizeof(QXLClipRects) + 
            clip_rects->num_rects * sizeof(QXLRect), 1);
        cmd_clip->num_rects = clip_rects->num_rects;
        cmd_clip->chunk.data_size = clip_rects->num_rects*sizeof(QXLRect);
        cmd_clip->chunk.prev_chunk = cmd_clip->chunk.next_chunk = 0;
        memcpy(cmd_clip + 1, clip_rects->ptr, cmd_clip->chunk.data_size);

        drawable->clip.type = SPICE_CLIP_TYPE_RECTS;
        drawable->clip.data = (intptr_t)cmd_clip;

        if (clip_rects->destroyable) {
            free(clip_rects->ptr);
        }
    }
}
static TestSpiceUpdate *test_create_update_image (test_qxl_t *qxl, 
                TestBitmap *bitmap, const QXLRect *bbox, uint32_t image_id,
                uint32_t surface_id, TestClipList *clip_rects)
{
    TestSpiceUpdate *update;
    QXLImage *image;
    QXLDrawable *drawable;
    uint32_t bw, bh;

    bw = bbox->right - bbox->left;
    bh = bbox->bottom - bbox->top;

    update = calloc (sizeof(*update), 1);
    drawable = &update->drawable;
    image = &update->image;

    drawable->surface_id = surface_id; 
    drawable->bbox = *bbox;

    test_fill_clip_data (drawable, clip_rects); 

    drawable->effect            = QXL_EFFECT_BLEND;
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
   
    QXL_SET_IMAGE_ID (image, QXL_IMAGE_GROUP_DEVICE, image_id);

    image->descriptor.width     = image->bitmap.x = bw;
    image->descriptor.height    = image->bitmap.y = bh;
    image->bitmap.data          = (intptr_t)bitmap->ptr;
    image->descriptor.type      = SPICE_IMAGE_TYPE_BITMAP;
    image->bitmap.flags         = QXL_BITMAP_DIRECT | QXL_BITMAP_TOP_DOWN;
    image->bitmap.stride        = bw * 4;
    image->bitmap.palette       = 0;
    image->bitmap.format        = SPICE_BITMAP_FMT_32BIT;
    image->descriptor.flags     = QXL_IMAGE_CACHE;
    
    update->bitmap              = *bitmap;

    set_cmd (&update->ext, QXL_CMD_DRAW, (intptr_t)drawable);

    return update;
}

static TestSpiceUpdate *test_create_update_fill (test_qxl_t *qxl,
           color_t color, const QXLRect *bbox,
           uint32_t surface_id, TestClipList *clip_rects)
{
    TestSpiceUpdate *update;
    QXLDrawable *drawable;
    QXLFill *fill;

    update = calloc (sizeof(*update), 1);
    drawable = &update->drawable;
    fill = &drawable->u.fill;

    drawable->surface_id = surface_id; 
    drawable->bbox = *bbox;

    test_fill_clip_data (drawable, clip_rects);

    drawable->effect            = QXL_EFFECT_OPAQUE;
    test_set_release_info (&drawable->release_info, (intptr_t)update);
    drawable->type              = QXL_DRAW_FILL;
    drawable->surfaces_dest[0]  = -1;
    drawable->surfaces_dest[1]  = -1;
    drawable->surfaces_dest[2]  = -1;

    fill->rop_descriptor    = SPICE_ROPD_OP_PUT;
    fill->brush.type        = SPICE_BRUSH_TYPE_SOLID;
    fill->brush.u.color     = color;
    fill->mask.flags        = 0;
    fill->mask.pos.x        = 0;
    fill->mask.pos.y        = 0;
    fill->mask.bitmap       = 0;

    set_cmd (&update->ext, QXL_CMD_DRAW, (intptr_t)drawable);
    return update;
}

static TestSpiceUpdate *test_create_update_solid (test_qxl_t *qxl, 
                TestCommandDraw *command, int is_last_call, 
                uint32_t surface_id, TestClipList *clip_rects)
{
    TestBitmap bitmap_str;
    uint32_t *dst;
    QXLRect *bbox = &command->rect;
    int bw, bh;
    int i;
    int fill_up;

    bw = bbox->right - bbox->left;
    bh = bbox->bottom - bbox->top;
 
    if (command->image_id == 0) {
        command->bitmap.ptr = bitmap_str.ptr = malloc (bw * bh * 4);
        fill_up = TRUE;
        command->image_id = image_unique++;
    } else {
        bitmap_str = command->bitmap;
        fill_up = command->color != *(color_t *)(bitmap_str.ptr);
    }
    dst = (uint32_t *) bitmap_str.ptr;

    if (fill_up) {
        for (i=0; i < bw * bh; ++i, ++dst) {
            *dst = command->color;
        }
    }
    bitmap_str.destroyable = is_last_call;

    return test_create_update_image (qxl, &bitmap_str,
        bbox, command->image_id, surface_id, clip_rects);
}

/* ================================================================
 * ====================== COMMAND CONTROL =========================
 * ================================================================
 */

RingItem *add_command (const TestCommand *command)
{
    TestCommandItem *item = calloc (sizeof(TestCommandItem),1);
    memcpy (&item->command, command, sizeof(TestCommand));
    ring_item_init(&item->link);
    ring_add_before (&item->link, &commands);
    return &item->link;
}
void remove_command (RingItem *item)
{
    TestCommandItem *command = (TestCommandItem *)item;
    ring_remove (item);
    free (command);
}
TestCommand *get_next_command (RingItem *item) 
{
    if (ring_is_empty (&commands)) {
        return NULL;
    }

    TestCommandItem *command_item = (TestCommandItem *)ring_next(&commands, item);
    if (!command_item) {
        command_item = (TestCommandItem *)ring_get_head(&commands);
    }
    return &command_item->command;
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
        remove_command (item);
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
        command = &command_container;
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
        if (command->draw.cg) {
            command->draw.cg (command->draw.opaque, command);
        }
        switch (command->draw.type) {
        case COMMAND_DRAW_FROM_BITMAP: {
            dprint (2, "draw bitmap");
            if (command->draw.image_id == 0) {
                command->draw.image_id = image_unique++;
            }
            update = test_create_update_image (qxl, 
                &command->draw.bitmap, &command->draw.rect, 
                command->draw.image_id, 0, &command->draw.clip_rects );
            break;
        }
        case COMMAND_DRAW_SOLID: {
            dprint (2, "draw solid");
            update = test_create_update_solid (qxl,
                &command->draw,(command->times == 1), 0,
                &command->draw.clip_rects);
            break;
        }
        case COMMAND_DRAW_FILL: {
            dprint (2, "fill surface");
            update = test_create_update_fill (qxl,
                command->draw.color, &command->draw.rect, 0,
                &command->draw.clip_rects);
            break;
        }
        case COMMAND_DRAW_SURFACE: {
            dprint (2, "draw surface (NOT IMPLEMENED)");   
            //TODO: implement
            break;
        }
        default:
            dprint (2, "unknown draw command with type_code %d", command->draw.type);
            break;
        }
        if (update != NULL ) {
            qxl->push_command(qxl, &update->ext);
        }
        break;
    }

    case COMMAND_CONTROL: {
        TestCommand local_container;
        switch (command->control.type) {
        case COMMAND_CONTROL_PUSH_LAST:
            command->control.cg(command->control.opaque, &local_container);
            add_command(&local_container);
            break;
        
        case COMMAND_CONTROL_REMOVE_PREV:
            remove_command(item->link.prev);
            break;
        case COMMAND_CONTROL_REMOVE_NEXT:
            remove_command(item->link.next);
            break;
        case COMMAND_CONTROL_REMOVE:
            remove_command(command->control.pos);
            break;
        case COMMAND_CONTROL_IGNORE:
            dprint(2, "ignore");
            break;
        }   
        break;
    }
    case COMMAND_SWITCH_SURFACE:
        dprint (2, "switching to surface %d", command->switch_surface.surface_id);
        assert (command->switch_surface.surface_id < NUM_SURFACES);
        qxl->target_surface = command->switch_surface.surface_id;
        break;

    default:
        dprint (1, "unknown command of type_code %d", command->type );
        break;
    }

    command = &item->command;
    if (surface_cmd != NULL) {
        qxl->push_command (qxl, &surface_cmd->ext);
    }

    qxl->current_command = ring_next (&commands, &item->link); 
    if (command->times > 0) {
        if ( --command->times <= 0) {
            if (qxl->current_command == &item->link) {
                qxl->current_command = NULL;
            }
            remove_command (&item->link); 
        }
    }
}

void draw_command_init (TestCommand *command) {
    command->type = COMMAND_DRAW;
    command->draw.image_id      = 0;
    command->draw.cg            = NULL;
    command->draw.opaque        = NULL;
    command->draw.bitmap.ptr         = NULL;
    command->draw.bitmap.destroyable = FALSE;
    command->draw.clip_rects.num_rects   = 0;
    command->draw.clip_rects.ptr         = NULL;
    command->draw.clip_rects.destroyable = FALSE;
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
