/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_SM_LAUNCHER_RUNTIMES_CONTAINER_TESTS_MOCKS_CONTAINERHANDLERMOCK_HPP_
#define AOS_SM_LAUNCHER_RUNTIMES_CONTAINER_TESTS_MOCKS_CONTAINERHANDLERMOCK_HPP_

#include <gmock/gmock.h>

#include <sm/launcher/runtimes/container/itf/containerhandler.hpp>

namespace aos::sm::launcher {

class ContainerHandlerMock : public ContainerHandlerItf {
public:
    MOCK_METHOD(Error, StartContainer, (const std::string&, Duration), (override));
    MOCK_METHOD(RetWithError<ProcessStatus>, GetContainerStatus, (const std::string&), (override));
    MOCK_METHOD((RetWithError<std::vector<ProcessStatus>>), ListContainers, (), (override));
    MOCK_METHOD(Error, StopContainer, (const std::string&, Duration), (override));
    MOCK_METHOD(Error, RemoveContainer, (const std::string&), (override));
};

} // namespace aos::sm::launcher

#endif
