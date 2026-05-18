/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Include Aos logger first so its LOG_ERR macro is established before syslog.h (pulled in by
// libcrun/error.h) can define its own LOG_ERR=3.  Save and restore across the C-header block.
#include <core/common/tools/logger.hpp>

#include <thread>
#include <iostream>
#include <chrono>
#include <iomanip>

#pragma push_macro("LOG_ERR")
#undef LOG_ERR
extern "C" {
#include <libcrun/container.h>
#include <libcrun/status.h>
}
#pragma pop_macro("LOG_ERR")

#include "crunhandler.hpp"

namespace aos::sm::launcher {

/***********************************************************************************************************************
 * Private helpers
 **********************************************************************************************************************/

namespace {

libcrun_context_t MakeContext(const std::string& stateRoot, const std::string& id)
{
    libcrun_context_t ctx = {};

    ctx.state_root = stateRoot.c_str();
    ctx.state_root = "/run/crun";
    ctx.id         = id.c_str();
    ctx.fifo_exec_wait_fd = -1;

    return ctx;
}

Error ReleaseLibcrunError(libcrun_error_t& err, const char* op)
{
    const char* msg = (err && err->msg) ? err->msg : "unknown error";

    LOG_ERR() << op << ": " << msg;
    libcrun_error_release(&err);

    return Error(ErrorEnum::eFailed, msg);
}


void printCurrentDateTime(std::string_view prefix)
{
    auto now        = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ns     = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() % 1000000000;

    std::cout << prefix << "Current date and time: " << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
              << "." << std::setfill('0') << std::setw(9) << now_ns << std::endl;
}


} // namespace

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error CrunHandler::Init(const std::string& /*runnerBin*/, const std::string& runtimeDir)
{
    mRuntimeDir = runtimeDir;

    return ErrorEnum::eNone;
}

Error CrunHandler::StartContainer(const std::string& instanceID, Duration /*timeout*/)
{
    const std::string bundleDir  = mRuntimeDir + "/" + instanceID;
    const std::string configPath = bundleDir + "/config.json";
    const std::string pidFile    = mRuntimeDir + "/" + instanceID + "/.pid";

    libcrun_error_t   err = nullptr;
    libcrun_context_t ctx = MakeContext(mRuntimeDir, instanceID);

    ctx.bundle   = bundleDir.c_str();
    ctx.pid_file = pidFile.c_str();
    ctx.detach   = true;

    // Pre-delete any leftover container state (ignore failure).
    // libcrun_container_delete(&ctx, nullptr, instanceID.c_str(), true, &err);
    // libcrun_container_kill(&ctx, instanceID.c_str(), "SIGKILL", &err);
    libcrun_error_release(&err);

    printCurrentDateTime("Before loading container. ");

    libcrun_container_t* container = libcrun_container_load_from_file(configPath.c_str(), &err);
    if (!container) {
        return ReleaseLibcrunError(err, "load container");
    }

    printCurrentDateTime("After loading container. Before running container. ");

    if (libcrun_container_run(&ctx, container, 0, &err) < 0) {
        libcrun_container_free(container);

        return ReleaseLibcrunError(err, "run container");
    }

    printCurrentDateTime("After running container. ");

    libcrun_container_free(container);

    {
        std::lock_guard lock {mMutex};

        mManagedInstances.insert(instanceID);
    }

    return ErrorEnum::eNone;
}

RetWithError<ProcessStatus> CrunHandler::GetContainerStatus(const std::string& instanceID)
{
    return CheckProcessAlive(instanceID);
}

RetWithError<std::vector<ProcessStatus>> CrunHandler::ListContainers()
{
    std::set<std::string> instances;

    {
        std::lock_guard lock {mMutex};

        instances = mManagedInstances;
    }

    std::vector<ProcessStatus> result;

    for (const auto& id : instances) {
        auto [status, err] = CheckProcessAlive(id);
        if (!err.IsNone()) {
            LOG_WRN() << "Failed to check process status" << Log::Field("instanceID", id.c_str()) << Log::Field(err);
        }

        result.push_back(status);
    }

    return {result, ErrorEnum::eNone};
}

Error CrunHandler::StopContainer(const std::string& instanceID, Duration /*timeout*/)
{
    libcrun_error_t   err = nullptr;
    libcrun_context_t ctx = MakeContext(mRuntimeDir, instanceID);

    if (libcrun_container_kill(&ctx, instanceID.c_str(), "SIGKILL", &err) < 0) {
        return ReleaseLibcrunError(err, "kill container");
    }

    return ErrorEnum::eNone;
}

Error CrunHandler::RemoveContainer(const std::string& instanceID)
{
    {
        std::lock_guard lock {mMutex};

        mManagedInstances.erase(instanceID);
    }

    libcrun_error_t   err = nullptr;
    libcrun_context_t ctx = MakeContext(mRuntimeDir, instanceID);

    if (libcrun_container_delete(&ctx, nullptr, instanceID.c_str(), true, &err) < 0) {
        return ReleaseLibcrunError(err, "delete container");
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

RetWithError<ProcessStatus> CrunHandler::CheckProcessAlive(const std::string& instanceID) const
{
    ProcessStatus status;

    status.mInstanceID = instanceID;

    libcrun_error_t          err        = nullptr;
    libcrun_container_status_t crunStatus = {};

    if (libcrun_read_container_status(&crunStatus, "/run/crun", instanceID.c_str(), &err) < 0) {
        libcrun_error_release(&err);
        status.mState = InstanceStateEnum::eInactive;

        return {status, ErrorEnum::eNone};
    }

    const int running = libcrun_is_container_running(&crunStatus, &err);

    libcrun_free_container_status(&crunStatus);

    if (running < 0) {
        libcrun_error_release(&err);
        status.mState = InstanceStateEnum::eFailed;

        return {status, ErrorEnum::eNone};
    }

    status.mState = (running > 0) ? InstanceStateEnum::eActive : InstanceStateEnum::eFailed;

    return {status, ErrorEnum::eNone};
}

} // namespace aos::sm::launcher
