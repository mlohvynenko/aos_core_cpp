/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_SM_LAUNCHER_RUNTIMES_CONTAINER_ITF_CONTAINERHANDLER_HPP_
#define AOS_SM_LAUNCHER_RUNTIMES_CONTAINER_ITF_CONTAINERHANDLER_HPP_

#include <string>
#include <vector>

#include <core/common/tools/error.hpp>
#include <core/common/tools/optional.hpp>
#include <core/common/tools/time.hpp>
#include <core/common/types/common.hpp>

namespace aos::sm::launcher {

/**
 * Process status.
 */
struct ProcessStatus {
    std::string       mInstanceID;
    InstanceState     mState;
    Optional<int32_t> mExitCode;
};

/**
 * Container handler interface.
 */
class ContainerHandlerItf {
public:
    /**
     * Destructor.
     */
    virtual ~ContainerHandlerItf() = default;

    /**
     * Starts a container for the given instance.
     *
     * @param instanceID instance ID.
     * @param timeout start timeout.
     * @return Error.
     */
    virtual Error StartContainer(const std::string& instanceID, Duration timeout) = 0;

    /**
     * Returns the status of a container.
     *
     * @param instanceID instance ID.
     * @return RetWithError<ProcessStatus>.
     */
    virtual RetWithError<ProcessStatus> GetContainerStatus(const std::string& instanceID) = 0;

    /**
     * Returns the status of all managed containers.
     *
     * @return RetWithError<std::vector<ProcessStatus>>.
     */
    virtual RetWithError<std::vector<ProcessStatus>> ListContainers() = 0;

    /**
     * Stops a container.
     *
     * @param instanceID instance ID.
     * @param timeout stop timeout.
     * @return Error.
     */
    virtual Error StopContainer(const std::string& instanceID, Duration timeout) = 0;

    /**
     * Removes a stopped container.
     *
     * @param instanceID instance ID.
     * @return Error.
     */
    virtual Error RemoveContainer(const std::string& instanceID) = 0;
};

} // namespace aos::sm::launcher

#endif
