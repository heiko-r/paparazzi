/*
 * Copyright (C) 2015 The Paparazzi Team
 *
 * This file is part of paparazzi.
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @file boards/bebop/board.c
 *
 * Bebop specific board initialization function.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mcu_periph/i2c.h"
#include "mt9v117.h"
#include "mt9f002.h"
#include "mcu.h"

static int kill_gracefull(char *process_name)
  {
    /* "pidof" always in /bin on Bebop firmware tested 1.98, 2.0.57, no need for "which" */
    char pidof_commandline[200] = "/bin/pidof ";
    strcat(pidof_commandline, process_name);
    /* Bebop Busybox a
       $ cat /proc/sys/kernel/pid_max
       Gives max 32768, makes sure it never kills existing other process
    */
    char pid[7] = "";
    int ret = 0; /* Return code of kill system call */
    FILE *fp;

    while (ret == 0) {
      fp = popen(pidof_commandline, "r");
      if (fp != NULL) { /* Could open the pidof with line */
        if (fgets(pid, sizeof(pid) - 1, fp) != NULL) {
          //printf("Process ID deducted: \"%s\"\n", pid);
          if (atoi(pid) > 0) { /* To make sure we end 0 > There is a real process id found */
            char kill_command_and_process[200] = "kill -9 "; /* BTW there is no pkill on this Busybox */
            strcat(kill_command_and_process, pid);
            ret = system(kill_command_and_process);
            /* No need to wait, since if it is not closed the next pidof scan still will still find it and try to kill it */
          }
        } else {
          ret = 256; /* Could not get handle */
          pclose(fp);
        }
      } else {
        ret = 256; /* fp NULL, so no process, just return */
        return 0;
      }
    } /* end while */
    return 0;
}

void board_init(void)
{
  /*
   *  Process running by default for firmware >= v1.98
   *
   * -  /bin/sh - /usr/bin/DragonStarter.sh -out2null
   * -  //usr/bin/dragon-prog
   *
   * Thus two process to kill, the DragonStarter first
   * This to make sure OEM program does not get re-started
   *
  */
  int ret __attribute__((unused)) = system("killall -q -15 DragonStarter.sh");
  usleep(50000); /* Give DragonStarter 50ms time to end on a busy system */
  kill_gracefull("dragon-prog");
}

void board_init2(void)
{
  /* Initialize MT9V117 chipset (Bottom camera) */
  struct mt9v117_t mt9v117 = {
    // Initial values

    // I2C connection port
    .i2c_periph = &i2c0
  };
  mt9v117_init(&mt9v117);

  /* Initialize MT9F002 chipset (Front camera) */
  struct mt9f002_t mt9f002 = {
    // Precomputed values to go from InputCLK of (26/2)MHz to 96MH
    .interface = MT9F002_PARALLEL,
    .input_clk_freq = (26 / 2),
    .vt_pix_clk_div = 7,
    .vt_sys_clk_div = 1,
    .pre_pll_clk_div = 1,
    .pll_multiplier = 59,
    .op_pix_clk_div = 8,
    .op_sys_clk_div = 1,
    .shift_vt_pix_clk_div = 1,
    .rowSpeed_2_0 = 1,
    .row_speed_10_8 = 1,

    // Initial values
    .target_fps = 5,
    .target_exposure = 30,
    .gain_green1 = 2.0,
    .gain_blue = 2.0,
    .gain_red = 2.0,
    .gain_green2 = 2.0,
    .output_width = 2048,
    .output_height = 3320,
    .output_scaler = 1.0,
    .offset_x = 1000,
    .offset_y = 0,

    // I2C connection port
    .i2c_periph = &i2c0
  };

  mt9f002_init(&mt9f002);
}
