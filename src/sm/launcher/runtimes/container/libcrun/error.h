/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Forward declarations for libcrun error types and functions.
 * crun does not install public headers; this wrapper covers only
 * the symbols used by CrunHandler.
 */

#ifndef AOS_LIBCRUN_WRAPPER_ERROR_H
#define AOS_LIBCRUN_WRAPPER_ERROR_H

#include <stdio.h>

#ifndef LIBCRUN_PUBLIC
#define LIBCRUN_PUBLIC __attribute__((visibility("default")))
#endif

struct libcrun_error_s {
    int   status;
    char *msg;
};
typedef struct libcrun_error_s *libcrun_error_t;

typedef void (*crun_output_handler)(int errno_, const char *msg, int verbosity, void *arg);

LIBCRUN_PUBLIC int  libcrun_error_release(libcrun_error_t *err);
LIBCRUN_PUBLIC void libcrun_error_write_warning_and_release(FILE *out, libcrun_error_t **err);

#endif
