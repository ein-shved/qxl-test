#include <spice.h>
//#include <qxl.h>
//#include <spiceqxl_spice_server.h>

#include "test_qxl_device.h"

int main (int argc, const char *argv[])
{
    test_qxl_t _qxl;
    test_qxl_t *qxl = &_qxl;
    QXLInterface *qxl_interface;

    //Init spice server
    qxl->spice_server = spice_server_new();
    
    //TODO set spice server options here

    //FIXME maybe I need to have my own event loop?
    qxl->core = /*test_*/basic_event_loop_init();
    spice_server_init (qxl->spice_server, qxl->core);

    dprint (qxl, 1, "%s: (server init done) ", __func__);

    //Add QXL interface to spice_server
    qxl_interface = test_init_qxl();
    qxl->display_sin.base.sif = &qxl_interface->base;
    qxl->display_sin.id = 0;
    qxl->display_sin.st = (struct QXLState*)qxl;
    spice_server_add_interface (qxl->spice_server, &qxl->display_sin.base);

    dprint (qxl, 1, "%s: (interface added) ", __func__);

    //Launch worker
    qxl->worker->start (qxl->worker);
    qxl->worker_running = TRUE;
}

