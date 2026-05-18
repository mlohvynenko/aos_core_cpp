/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_SM_LAUNCHER_RUNTIMES_CONTAINER_CRUNHANDLER_HPP_
#define AOS_SM_LAUNCHER_RUNTIMES_CONTAINER_CRUNHANDLER_HPP_

#include <mutex>
#include <set>
#include <string>

#include "itf/containerhandler.hpp"

namespace aos::sm::launcher {

/**
 * Executes instance lifecycle commands via libcrun, replacing systemd unit management.
 */
class CrunHandler : public ContainerHandlerItf {
public:
    /**
     * Initializes the crun handler.
     *
     * @param runnerBin unused (kept for API compatibility).
     * @param runtimeDir base directory for per-instance runtime files (e.g. /run/aos/runtime).
     * @return Error.
     */
    Error Init(const std::string& runnerBin, const std::string& runtimeDir);

    Error                                    StartContainer(const std::string& instanceID, Duration timeout) override;
    RetWithError<ProcessStatus>              GetContainerStatus(const std::string& instanceID) override;
    RetWithError<std::vector<ProcessStatus>> ListContainers() override;
    Error                                    StopContainer(const std::string& instanceID, Duration timeout) override;
    Error                                    RemoveContainer(const std::string& instanceID) override;

private:
    RetWithError<ProcessStatus> CheckProcessAlive(const std::string& instanceID) const;

    std::string           mRuntimeDir;
    std::mutex            mMutex;
    std::set<std::string> mManagedInstances;
};

} // namespace aos::sm::launcher

#endif
