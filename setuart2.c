/*
 * setuart2.c
 *
 * Simple utility for enabling second UART on pcDuino
 *
 * Copyright (C) 2013 William Greathouse, Brecksville, OH, USA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define PAGE_SIZE       4096
#define GPIO_PAGE       0x01c20000
#define GPIO_OFFSET     0X0800

volatile unsigned int *gpio;

int main(int argc, char *argv[])
{
    int mem_fd;
    void *gpio_map;
    unsigned int setting;
    unsigned int pullup;


    /* Check for somewhat reasonable input from the command line */
    if ( argc != 2 ||
            (strcasecmp(argv[1], "on")  != 0 &&
             strcasecmp(argv[1], "off") != 0) )
    {
        fprintf(stderr, "%s: either 'on' or 'off' argument is required\n",
            argv[0]);
        return 1;
    }

    /***** This is the meat of the code *****/

    /* Get access to the device memory map -- This requires root authority! */
    mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (mem_fd < 0)
    {
        perror("/dev/mem");
        return 1;
    }

    gpio_map = mmap(
        NULL,
        PAGE_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        GPIO_PAGE);

    close(mem_fd); // no longer needed

    if (gpio_map == MAP_FAILED)
    {
        perror("gpio_map");
        fprintf(stderr, "mmap error=%d\n", (int)gpio_map);
        return 2;
    }

    /* Now get a pointer to the gpio region */
    gpio = (volatile unsigned int *)gpio_map + GPIO_OFFSET/4;

    /* How we want to set up the pin modes */
    setting = gpio[0x120/4+2] & 0xffff00ff;
    pullup  = gpio[0x140/4] & 0xffffff0f;
    if (strcasecmp(argv[1], "on") == 0)
    {
        setting |= 0x00003300;
    }
    else
    {
        pullup |= 0x00000050;
    }

    /* set the pin modes */
    gpio[0x120/4+2] = setting;
    gpio[0x140/4]   = pullup;

    /* Warm and fuzzy... display the new settings for the pin modes */
    fprintf(stdout, " PI_CFG2:0x%08x\n", gpio[0x120/4+2]);
    fprintf(stdout, "PI_PULL1:0x%08x\n", gpio[0x120/4+8]);

    /* No longer need the memory map, so give it up */
    munmap(gpio_map, PAGE_SIZE);

    return 0;
}