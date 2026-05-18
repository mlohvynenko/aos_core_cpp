/*
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_SM_LAUNCHER_RUNTIMES_CONTAINER_RUNNER_HPP_
#define AOS_SM_LAUNCHER_RUNTIMES_CONTAINER_RUNNER_HPP_

#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <core/common/tools/time.hpp>

#include "itf/runner.hpp"

namespace aos::sm::launcher {

/**
 * Service runner.
 */
class Runner : public RunnerItf {
public:
    /**
     * Initializes Runner instance.
     *
     * @param receiver run status receiver.
     * @param processManager process manager.
     * @return Error.
     */
    Error Init(RunStatusReceiverItf& receiver, ProcessManagerItf& processManager) override;

    /**
     * Starts monitoring thread.
     *
     * @return Error.
     */
    Error Start() override;

    /**
     * Stops Runner.
     *
     * @return Error.
     */
    Error Stop() override;

    /**
     * Starts service instance.
     *
     * @param instanceID instance ID.
     * @param params runtime parameters.
     * @return RunStatus.
     */
    RunStatus StartInstance(const std::string& instanceID, const RunParameters& params) override;

    /**
     * Stops service instance.
     *
     * @param instanceID instance ID.
     * @return Error.
     */
    Error StopInstance(const std::string& instanceID) override;

private:
    static constexpr auto cDefaultStartInterval   = 5 * Time::cSeconds;
    static constexpr auto cDefaultStopTimeout     = 5 * Time::cSeconds;
    static constexpr auto cStartTimeMultiplier    = 1.2;
    static constexpr auto cDefaultStartBurst      = 3;
    static constexpr auto cDefaultRestartInterval = 1 * Time::cSeconds;

    static constexpr auto cStatusPollPeriod = std::chrono::seconds(1);

    void                            MonitorUnits();
    std::vector<RunStatus>&         GetRunningInstances() const;
    RetWithError<InstanceState>     GetStartingProcessState(const std::string& instanceID, Duration startInterval);

    struct StartingUnitData {
        std::condition_variable mCondVar;
        InstanceState           mRunState;
        Optional<int32_t>       mExitCode;
    };

    struct RunningUnitData {
        InstanceState     mRunState;
        Optional<int32_t> mExitCode;
    };

    RunStatusReceiverItf* mRunStatusReceiver = nullptr;

    ProcessManagerItf*      mProcessManager = {};
    std::thread             mMonitoringThread;
    std::mutex              mMutex;
    std::condition_variable mCondVar;

    std::map<std::string, StartingUnitData> mStartingUnits;
    std::map<std::string, RunningUnitData>  mRunningUnits;
    mutable std::vector<RunStatus>          mRunningInstances;

    bool mClosed = false;
};

} // namespace aos::sm::launcher

#endif
