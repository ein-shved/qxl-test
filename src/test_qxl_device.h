#include <spice.h>
#include <qxl.h>
#include <spiceqxl_spice_server.h>

typedef int bool;

typedef struct _test_qxl_t {
    QXLInstance display_sin;
    QXLWorker *worker;
    SpiceCoreInterface *core;
    SpiceServer *spice_server;

    bool worker_running;
} test_qxl_t;

QXLInterface *test_init_qxl(void);
