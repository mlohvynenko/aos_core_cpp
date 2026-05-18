/*
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <iostream>


#include <core/common/tools/logger.hpp>

#include "runner.hpp"

namespace aos::sm::launcher {

/***********************************************************************************************************************
 * Statics
 **********************************************************************************************************************/

namespace {


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
 * Implementation
 **********************************************************************************************************************/

Error Runner::Init(RunStatusReceiverItf& receiver, ProcessManagerItf& processManager)
{
    mRunStatusReceiver = &receiver;
    mProcessManager    = &processManager;

    return ErrorEnum::eNone;
}

Error Runner::Start()
{
    LOG_DBG() << "Start runner";

    mClosed           = false;
    mMonitoringThread = std::thread(&Runner::MonitorUnits, this);

    return ErrorEnum::eNone;
}

Error Runner::Stop()
{
    {
        std::lock_guard lock {mMutex};

        if (mClosed) {
            return ErrorEnum::eNone;
        }

        LOG_DBG() << "Stop runner";

        mClosed = true;
        mCondVar.notify_all();
    }

    if (mMonitoringThread.joinable()) {
        mMonitoringThread.join();
    }

    return ErrorEnum::eNone;
}

RunStatus Runner::StartInstance(const std::string& instanceID, const RunParameters& params)
{
    printCurrentDateTime("Before starting unit. ");


    RunStatus status = {};

    status.mInstanceID = instanceID;
    status.mState      = InstanceStateEnum::eFailed;

    RunParameters fixedParams = params;

    if (!params.mStartInterval.HasValue()) {
        fixedParams.mStartInterval = cDefaultStartInterval;
    }

    if (!params.mStartBurst.HasValue()) {
        fixedParams.mStartBurst = cDefaultStartBurst;
    }

    if (!params.mRestartInterval.HasValue()) {
        fixedParams.mRestartInterval = cDefaultRestartInterval;
    }

    LOG_DBG() << "Start service instance" << Log::Field("instanceID", instanceID.c_str())
              << Log::Field("startInterval", fixedParams.mStartInterval)
              << Log::Field("startBurst", fixedParams.mStartBurst)
              << Log::Field("restartInterval", fixedParams.mRestartInterval);

    const auto startTime = static_cast<Duration>(cStartTimeMultiplier * fixedParams.mStartInterval.GetValue());

    if (status.mError = mProcessManager->StartProcess(instanceID, startTime); !status.mError.IsNone()) {
        return status;
    }



    // Start unit.
    // const auto startTime = static_cast<Duration>(cStartTimeMultiplier * fixedParams.mStartInterval.GetValue());

    // if (std::system("/apps/crun/crun/crun run -d --pid-file /run/aos/runtime/a72c6f72-3211-3eb6-9eb2-d309fccdfc89/.pid -b /run/aos/runtime/a72c6f72-3211-3eb6-9eb2-d309fccdfc89/ a72c6f72-3211-3eb6-9eb2-d309fccdfc89") != 0) {
    //     LOG_ERR() << "Failed to start instance with crun";

    //     status.mState = InstanceStateEnum::eFailed;
    // } else  {
    //     status.mState = InstanceStateEnum::eActive;
    // }

    // if (status.mError = mSystemd->StartUnit(unitName, "replace", startTime); !status.mError.IsNone()) {
    //     return status;
    // }

    printCurrentDateTime("After starting unit. ");

    if (auto [processStatus, err] = mProcessManager->GetProcessStatus(instanceID); !err.IsNone()) {
        LOG_ERR() << "Failed to get process status after starting instance" << Log::Field(err);

        status.mError = err;
    } else {
        status.mState = processStatus.mState;
    }

    // Get unit status.


    LOG_DBG() << "Start instance" << Log::Field("instanceID", instanceID.c_str())
              << Log::Field("state", status.mState) << Log::Field("error", status.mError);

    return status;
}

Error Runner::StopInstance(const std::string& instanceID)
{
    LOG_DBG() << "Stop instance" << Log::Field("instanceID", instanceID.c_str());

    {
        std::lock_guard lock {mMutex};

        mRunningUnits.erase(instanceID);
    }

    auto err = mProcessManager->StopProcess(instanceID, cDefaultStopTimeout);
    if (!err.IsNone()) {
        if (err.Is(ErrorEnum::eNotFound)) {
            LOG_DBG() << "Process not found" << Log::Field("instanceID", instanceID.c_str());

            err = ErrorEnum::eNone;
        }
    }

    if (auto removeErr = mProcessManager->RemoveProcess(instanceID); !removeErr.IsNone()) {
        if (!removeErr.Is(ErrorEnum::eNotFound) && err.IsNone()) {
            err = removeErr;
        }
    }

    return err;
}

void Runner::MonitorUnits()
{
    while (!mClosed) {
        std::unique_lock lock {mMutex};

        bool closed = mCondVar.wait_for(lock, cStatusPollPeriod, [this]() { return mClosed; });
        if (closed) {
            return;
        }

        auto [processes, err] = mProcessManager->ListProcesses();
        if (!err.IsNone()) {
            LOG_ERR() << "List processes failed" << Log::Field(err);

            return;
        }

        bool unitChanged = false;

        for (const auto& process : processes) {
            auto startUnitIt = mStartingUnits.find(process.mInstanceID);
            if (startUnitIt != mStartingUnits.end()) {
                startUnitIt->second.mRunState = process.mState;
                startUnitIt->second.mExitCode = process.mExitCode;

                if (process.mState.GetValue() == InstanceStateEnum::eFailed) {
                    startUnitIt->second.mCondVar.notify_all();
                }
            }

            auto runUnitIt = mRunningUnits.find(process.mInstanceID);
            if (runUnitIt != mRunningUnits.end()) {
                auto& runningState = runUnitIt->second;

                if (process.mState != runningState.mRunState || process.mExitCode != runningState.mExitCode) {
                    runningState = RunningUnitData {process.mState, process.mExitCode};
                    unitChanged  = true;
                }
            }
        }

        if (unitChanged || mRunningUnits.size() != mRunningInstances.size()) {
            mRunStatusReceiver->UpdateRunStatus(GetRunningInstances());
        }
    }
}

std::vector<RunStatus>& Runner::GetRunningInstances() const
{
    mRunningInstances.clear();

    std::transform(
        mRunningUnits.begin(), mRunningUnits.end(), std::back_inserter(mRunningInstances), [](const auto& unit) {
            auto error = unit.second.mExitCode.HasValue() ? Error(unit.second.mExitCode.GetValue()) : Error();

            return RunStatus {unit.first, unit.second.mRunState, error};
        });

    return mRunningInstances;
}

RetWithError<InstanceState> Runner::GetStartingProcessState(const std::string& instanceID, Duration startInterval)
{
    const auto timeout = std::chrono::milliseconds(startInterval.Milliseconds());

    auto [initialStatus, err] = mProcessManager->GetProcessStatus(instanceID);
    if (!err.IsNone()) {
        return {InstanceStateEnum::eFailed, AOS_ERROR_WRAP(Error(err, "failed to get process status"))};
    }

    {
        std::unique_lock lock {mMutex};

        mStartingUnits[instanceID].mRunState = initialStatus.mState;
        mStartingUnits[instanceID].mExitCode = initialStatus.mExitCode;

        std::ignore   = mStartingUnits[instanceID].mCondVar.wait_for(lock, timeout);
        auto runState = mStartingUnits[instanceID].mRunState;
        auto exitCode
            = mStartingUnits[instanceID].mExitCode.HasValue() ? mStartingUnits[instanceID].mExitCode.GetValue() : 0;

        mStartingUnits.erase(instanceID);

        if (runState.GetValue() != InstanceStateEnum::eActive) {
            const auto errMsg = "failed to start process";
            err               = (exitCode) ? Error(exitCode, errMsg) : Error(ErrorEnum::eFailed, errMsg);

            return {InstanceStateEnum::eFailed, AOS_ERROR_WRAP(err)};
        }

        mRunningUnits[instanceID] = RunningUnitData {InstanceStateEnum::eActive, exitCode};

        return {InstanceStateEnum::eActive, ErrorEnum::eNone};
    }
}

} // namespace aos::sm::launcher
