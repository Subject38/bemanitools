#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <winuser.h>

#include "bemanitools/jbio.h"

#include "util/log.h"
#include "util/thread.h"

typedef struct {
    uint8_t r, g, b;
} rgb_t;

struct {
    int bit;
    char keycode;
    bool last_state;
} mappings[] = {
    {JB_IO_SYS_TEST, VK_ESCAPE, false},
    {JB_IO_SYS_SERVICE, VK_CONTROL, false},
    {JB_IO_SYS_COIN, VK_F2, false},
    {JB_IO_PANEL_01, '4', false},
    {JB_IO_PANEL_02, '5', false},
    {JB_IO_PANEL_03, '6', false},
    {JB_IO_PANEL_04, '7', false},
    {JB_IO_PANEL_05, 'R', false},
    {JB_IO_PANEL_06, 'T', false},
    {JB_IO_PANEL_07, 'Y', false},
    {JB_IO_PANEL_08, 'U', false},
    {JB_IO_PANEL_09, 'F', false},
    {JB_IO_PANEL_10, 'G', false},
    {JB_IO_PANEL_11, 'H', false},
    {JB_IO_PANEL_12, 'J', false},
    {JB_IO_PANEL_13, 'V', false},
    {JB_IO_PANEL_14, 'B', false},
    {JB_IO_PANEL_15, 'N', false},
    {JB_IO_PANEL_16, 'M', false},
};

#define IS_BIT_SET(var, bit) ((((var) >> (bit)) & 1) > 0)
#define countof(array) (sizeof(array) / sizeof(array[0]))

/**
 * Tool to test your implementations of jbio.
 */
int main(int argc, char **argv)
{
    log_to_writer(log_writer_stdout, NULL);

    jb_io_set_loggers(
        log_impl_misc, log_impl_info, log_impl_warning, log_impl_fatal);

    if (!jb_io_init(crt_thread_create, crt_thread_join, crt_thread_destroy)) {
        printf("Initializing jbio failed\n");
        return -1;
    }

    printf(">>> Initializing jbio successful\n");

    /* inputs */
    uint16_t input_panel = 0;
    uint8_t input_sys = 0;

    /* outputs */
    rgb_t lights[6] = {0};

    INPUT inputs[countof(mappings)] = {};

    ZeroMemory(inputs, sizeof(inputs));

    bool loop = true;

    for (int i = 0; i < 6; i++) {
        jb_io_set_rgb_led(i, lights[i].r, lights[i].g, lights[i].b);
    }

    while (loop) {
        if (!jb_io_read_inputs()) {
            printf("ERROR: Input read fail\n");
            return -2;
        }

        /* get inputs */
        input_panel = jb_io_get_panel_inputs();
        input_sys = jb_io_get_sys_inputs();

        size_t inputs_i = 0;
        for (size_t i = 0; i < countof(mappings); i++) {
            bool new_state = i < 3 ? IS_BIT_SET(input_sys, mappings[i].bit) : IS_BIT_SET(input_panel, mappings[i].bit);

            if (mappings[i].last_state != new_state) {
                inputs_i++;
                mappings[i].last_state = new_state;
            }
            if (new_state) {
                inputs[i].type = INPUT_KEYBOARD;
                inputs[i].ki.wVk = mappings[i].keycode;
                inputs[i].ki.dwFlags = 0;
            } else {
                inputs[i].type = INPUT_KEYBOARD;
                inputs[i].ki.wVk = mappings[i].keycode;
                inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
            }
        }
        if (inputs_i) {
            UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
            if (uSent != ARRAYSIZE(inputs)) {
                printf(
                    "SendInput failed: 0x%lx\n",
                    HRESULT_FROM_WIN32(GetLastError()));
            }
        }
        if (!jb_io_write_lights()) {
            printf("ERROR: Writing outputs failed\n");
            return -4;
        }

        /* avoid CPU banging */
        Sleep(8);
    }

    system("cls");
    jb_io_fini();

    return 0;
}
