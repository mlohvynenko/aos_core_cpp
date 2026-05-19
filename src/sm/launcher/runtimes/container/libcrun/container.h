/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Forward declarations for libcrun container types and functions.
 * Struct layout mirrors crun 1.27.1 (commit 3ec076b3b6714ec2f1a10533cf18d5605a6de637).
 */

#ifndef AOS_LIBCRUN_WRAPPER_CONTAINER_H
#define AOS_LIBCRUN_WRAPPER_CONTAINER_H

#include <stdbool.h>

#include "error.h"

#ifndef LIBCRUN_PUBLIC
#define LIBCRUN_PUBLIC __attribute__((visibility("default")))
#endif

/* Opaque – only used via pointer, no field access needed. */
struct custom_handler_manager_s;

struct libcrun_context_s {
    const char *state_root;
    const char *id;
    const char *bundle;
    const char *console_socket;
    const char *pid_file;
    const char *notify_socket;
    const char *handler;
    int         preserve_fds;
    int         listen_fds;

    crun_output_handler output_handler;
    void               *output_handler_arg;

    int fifo_exec_wait_fd;

    bool systemd_cgroup;
    bool detach;
    bool no_new_keyring;
    bool force_no_cgroup;
    bool no_pivot;

    char **argv;
    int    argc;

    struct custom_handler_manager_s *handler_manager;
};
typedef struct libcrun_context_s libcrun_context_t;

/* Opaque container object – no fields accessed by callers. */
struct libcrun_container_s;
typedef struct libcrun_container_s libcrun_container_t;

/* Opaque OCI spec – passed as NULL in our use cases. */
struct runtime_spec_schema_config_schema_s;
typedef struct runtime_spec_schema_config_schema_s runtime_spec_schema_config_schema;

LIBCRUN_PUBLIC libcrun_container_t *libcrun_container_load_from_file(const char *path, libcrun_error_t *err);
LIBCRUN_PUBLIC void libcrun_container_free(libcrun_container_t *container);

LIBCRUN_PUBLIC int libcrun_container_run(libcrun_context_t *context, libcrun_container_t *container,
    unsigned int options, libcrun_error_t *error);

LIBCRUN_PUBLIC int libcrun_container_delete(libcrun_context_t *context,
    runtime_spec_schema_config_schema *def, const char *id, bool force, libcrun_error_t *err);

LIBCRUN_PUBLIC int libcrun_container_kill(libcrun_context_t *context, const char *id,
    const char *signal, libcrun_error_t *err);

#endif
