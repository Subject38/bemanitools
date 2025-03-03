#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <windows.h>

#include "aciodrv/device.h"

#include "aciotest/bi2a-sdvx.h"
#include "aciotest/handler.h"
#include "aciotest/icca.h"
#include "aciotest/kfca.h"
#include "aciotest/panb.h"
#include "aciotest/rvol.h"

#include "util/log.h"

static uint8_t aciotest_cnt = 0;
static uint8_t bi2a_mode = 0;

/**
 * Enumerate supported ACIO nodes based on their product id.
 */
static bool aciotest_assign_handler(
    uint32_t product_type, struct aciotest_handler_node_handler *handler)
{
    if (product_type == AC_IO_NODE_TYPE_ICCA) {
        handler->init = aciotest_icca_handler_init;
        handler->update = aciotest_icca_handler_update;

        return true;
    }

    if (product_type == AC_IO_NODE_TYPE_KFCA) {
        handler->init = aciotest_kfca_handler_init;
        handler->update = aciotest_kfca_handler_update;

        return true;
    }

    if (product_type == AC_IO_NODE_TYPE_PANB) {
        handler->init = aciotest_panb_handler_init;
        handler->update = aciotest_panb_handler_update;

        return true;
    }

    if (product_type == AC_IO_NODE_TYPE_RVOL) {
        handler->init = aciotest_rvol_handler_init;
        handler->update = aciotest_rvol_handler_update;

        return true;
    }

    if (product_type == AC_IO_NODE_TYPE_BI2A) {
        if (bi2a_mode == 0) {
            handler->init = aciotest_bi2a_sdvx_handler_init;
            handler->update = aciotest_bi2a_sdvx_handler_update;

            return true;
        } else {
            printf("Unknown BI2A device specified");
        }
    }

    return false;
}

/**
 * Tool to test real ACIO hardware.
 */
int main(int argc, char **argv)
{
    if (argc < 3) {
        printf(
            "aciotest, build "__DATE__
            " " __TIME__
            "\n"
            "Usage: %s <com port str> <baud rate>\n"
            "Example for two slotted readers: %s COM1 57600\n",
            argv[0],
            argv[0]);
        return -1;
    }

    log_to_writer(log_writer_stdout, NULL);

    struct aciodrv_device_ctx *device =
        aciodrv_device_open_path(argv[1], atoi(argv[2]));

    if (!device) {
        printf("Opening acio device failed\n");
        return -1;
    }

    printf("Opening acio device successful\n");

    uint8_t node_count = aciodrv_device_get_node_count(device);
    printf("Enumerated %d nodes\n", node_count);

    struct aciotest_handler_node_handler handler[aciotest_handler_max];
    memset(
        &handler,
        0,
        sizeof(struct aciotest_handler_node_handler) * aciotest_handler_max);

    for (uint8_t i = 0; i < node_count; i++) {
        char product[4];
        uint32_t product_type = aciodrv_device_get_node_product_type(device, i);
        aciodrv_device_get_node_product_ident(device, i, product);

        printf(
            "> %d: %c%c%c%c (%08x)\n",
            i + 1,
            product[0],
            product[1],
            product[2],
            product[3],
            product_type);

        if (!aciotest_assign_handler(product_type, &handler[i])) {
            printf(
                "ERROR: Unsupported acio node product %08x on node %d\n",
                product_type,
                i);
        }
    }

    for (uint8_t i = 0; i < aciotest_handler_max; i++) {
        if (handler[i].init != NULL) {
            if (!handler[i].init(device, i, &handler[i].ctx)) {
                printf("ERROR: Initializing node %d failed\n", i);
                handler[i].update = NULL;
            }
        }
    }

    printf(">>> Initializing done, press enter to start update loop <<<\n");

    if (getchar() != '\n') {
        return 0;
    }

    while (true) {
        system("cls");
        printf("%d\n", aciotest_cnt++);

        for (uint8_t i = 0; i < aciotest_handler_max; i++) {
            if (handler[i].update != NULL) {
                if (!handler[i].update(device, i, handler[i].ctx)) {
                    printf("ERROR: Updating node %d, removed from loop\n", i);
                    handler[i].update = NULL;
                    Sleep(5000);
                }
            }
        }

        /* avoid cpu banging */
        Sleep(30);
    }

    return 0;
}