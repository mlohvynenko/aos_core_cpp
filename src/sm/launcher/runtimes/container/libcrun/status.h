/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Forward declarations for libcrun status types and functions.
 * Struct layout mirrors crun 1.27.1 (commit 3ec076b3b6714ec2f1a10533cf18d5605a6de637).
 */

#ifndef AOS_LIBCRUN_WRAPPER_STATUS_H
#define AOS_LIBCRUN_WRAPPER_STATUS_H

#include <sys/types.h>

#include "error.h"

#ifndef LIBCRUN_PUBLIC
#define LIBCRUN_PUBLIC __attribute__((visibility("default")))
#endif

struct libcrun_container_status_s {
    pid_t              pid;
    unsigned long long process_start_time;
    char              *bundle;
    char              *rootfs;
    char              *cgroup_path;
    char              *scope;
    int                systemd_cgroup;
    char              *created;
    int                detached;
    char              *external_descriptors;
    char              *owner;
};
typedef struct libcrun_container_status_s libcrun_container_status_t;

LIBCRUN_PUBLIC void libcrun_free_container_status(libcrun_container_status_t *status);

LIBCRUN_PUBLIC int libcrun_read_container_status(libcrun_container_status_t *status,
    const char *state_root, const char *id, libcrun_error_t *err);

LIBCRUN_PUBLIC int libcrun_is_container_running(libcrun_container_status_t *status, libcrun_error_t *err);

#endif
