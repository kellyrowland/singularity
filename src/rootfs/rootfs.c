/* 
 * Copyright (c) 2015-2016, Gregory M. Kurtzer. All rights reserved.
 * 
 * “Singularity” Copyright (c) 2016, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * 
 * This software is licensed under a customized 3-clause BSD license.  Please
 * consult LICENSE file distributed with the sources of this project regarding
 * your rights to use or distribute this software.
 * 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such,
 * the U.S. Government has been granted for itself and others acting on its
 * behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
 * to reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so. 
 * 
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "file.h"
#include "util.h"
#include "message.h"
#include "config_parser.h"
#include "rootfs.h"
#include "privilege.h"
#include "image/image.h"
#include "dir/dir.h"

static int module = 0;
static char *chroot_dir = NULL;

int singularity_rootfs_init(char *source, char *mount_point, int writable) {
    message(DEBUG, "Checking on container source type\n");

    chroot_dir = strdup(mount_point);

    if ( is_file(source) == 0 ) {
        module = ROOTFS_IMAGE;
        return(rootfs_image_init(source, mount_point, writable));
    } else if ( is_dir(source) == 0 ) {
        module = ROOTFS_DIR;
        return(rootfs_dir_init(source, mount_point, writable));
    }

    message(ERROR, "Unknown rootfs source type\n");
    return(-1);
}

int singularity_rootfs_mount(void) {
    message(DEBUG, "Mounting image\n");

    if ( module == ROOTFS_IMAGE ) {
        return(rootfs_image_mount());
    } else if ( module == ROOTFS_DIR ) {
        return(rootfs_dir_mount());
    }

    message(ERROR, "Called rootfs_mount() without rootfs_init()\n");
    return(-1);
}

int singularity_rootfs_umount(void) {
    message(DEBUG, "Unmounting image\n");

    if ( module == ROOTFS_IMAGE ) {
        return(rootfs_image_umount());
    } else if ( module == ROOTFS_DIR ) {
        return(rootfs_dir_umount());
    }

    message(ERROR, "Called rootfs_umount() without rootfs_init()\n");
    return(-1);
}



int singularity_rootfs_chroot(void) {
    message(VERBOSE, "Entering container file system space\n");

    priv_escalate();
    if ( chroot(chroot_dir) < 0 ) { // Flawfinder: ignore (yep, yep, yep... we know!)
        message(ERROR, "failed enter container at: %s\n", chroot_dir);
        ABORT(255);
    }
    priv_drop();

    message(DEBUG, "Changing dir to '/' within the new root\n");
    if ( chdir("/") < 0 ) {
        message(ERROR, "Could not chdir after chroot to /: %s\n", strerror(errno));
        ABORT(1);
    }

    return(0);
}