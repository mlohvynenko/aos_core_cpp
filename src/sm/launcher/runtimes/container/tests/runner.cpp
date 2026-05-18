/*
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>

#include <core/common/tests/utils/log.hpp>

#include <sm/launcher/runtimes/container/runner.hpp>

#include "mocks/processmanagermock.hpp"
#include "mocks/runnermock.hpp"

using namespace testing;

namespace aos::sm::launcher {

class ContainerRunnerTest : public Test {
public:
    static void SetUpTestSuite() { tests::utils::InitLog(); }

    void SetUp() override { mRunner.Init(mRunStatusReceiver, mProcessManagerMock); }

protected:
    RunStatusReceiverMock mRunStatusReceiver;
    ProcessManagerMock    mProcessManagerMock;
    Runner                mRunner;
};

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

TEST_F(ContainerRunnerTest, StartInstance)
{
    RunParameters params = {{500 * Time::cMilliseconds}, {0}, {0}};
    ProcessStatus status = {"service0", InstanceStateEnum::eActive, {}};
    Error         err    = ErrorEnum::eNone;

    EXPECT_CALL(mProcessManagerMock, StartProcess("service0", _)).WillOnce(Return(err));
    EXPECT_CALL(mProcessManagerMock, GetProcessStatus("service0"))
        .WillOnce(Return(RetWithError<ProcessStatus>(status, err)));

    std::vector<ProcessStatus> processes = {status};
    EXPECT_CALL(mProcessManagerMock, ListProcesses())
        .WillRepeatedly(Return(RetWithError<std::vector<ProcessStatus>>(processes, err)));

    std::vector<RunStatus> expectedInstances {{"service0", InstanceStateEnum::eActive, Error()}};
    EXPECT_CALL(mRunStatusReceiver, UpdateRunStatus(expectedInstances)).Times(1);

    mRunner.Start();

    const auto expectedRes = RunStatus {"service0", InstanceStateEnum::eActive, ErrorEnum::eNone};

    EXPECT_EQ(mRunner.StartInstance("service0", params), expectedRes);

    sleep(2); // wait to monitor

    EXPECT_CALL(mProcessManagerMock, StopProcess("service0", _)).WillOnce(Return(err));
    EXPECT_CALL(mProcessManagerMock, RemoveProcess("service0")).WillOnce(Return(err));

    EXPECT_TRUE(mRunner.StopInstance("service0").IsNone());

    mRunner.Stop();
}

TEST_F(ContainerRunnerTest, StartProcessFailed)
{
    RunParameters params = {};

    EXPECT_CALL(mProcessManagerMock, StartProcess("service0", _)).WillOnce(Return(ErrorEnum::eFailed));

    mRunner.Start();

    const auto expectedRes = RunStatus {"service0", InstanceStateEnum::eFailed, ErrorEnum::eFailed};

    EXPECT_EQ(mRunner.StartInstance("service0", params), expectedRes);

    mRunner.Stop();
}

TEST_F(ContainerRunnerTest, GetProcessStatusFailed)
{
    mRunner.Start();

    RunParameters params = {};
    ProcessStatus status = {"service0", InstanceStateEnum::eFailed, {1}};
    Error         err    = ErrorEnum::eFailed;

    EXPECT_CALL(mProcessManagerMock, StartProcess("service0", _)).WillOnce(Return(Error()));
    EXPECT_CALL(mProcessManagerMock, GetProcessStatus("service0"))
        .WillOnce(Return(RetWithError<ProcessStatus>(status, err)));

    const auto expectedRes = RunStatus {"service0", InstanceStateEnum::eFailed, ErrorEnum::eFailed};

    EXPECT_EQ(mRunner.StartInstance("service0", params), expectedRes);

    mRunner.Stop();
}

TEST_F(ContainerRunnerTest, ListProcessesFailed)
{
    mRunner.Start();

    RunParameters params = {};

    EXPECT_CALL(mProcessManagerMock, StartProcess("service0", _)).WillOnce(Return(ErrorEnum::eFailed));

    const auto expectedRes = RunStatus {"service0", InstanceStateEnum::eFailed, ErrorEnum::eFailed};

    EXPECT_EQ(mRunner.StartInstance("service0", params), expectedRes);

    std::vector<ProcessStatus> processes = {{"service0", InstanceStateEnum::eFailed, {1}}};

    EXPECT_CALL(mProcessManagerMock, ListProcesses())
        .WillOnce(Return(RetWithError<std::vector<ProcessStatus>>(processes, Error(ErrorEnum::eFailed))));

    sleep(2); // wait to monitor

    mRunner.Stop();
}

} // namespace aos::sm::launcher
