/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIRESTORE_CORE_SRC_UTIL_TASK_H_
#define FIRESTORE_CORE_SRC_UTIL_TASK_H_

#include <memory>
#include <mutex>  // NOLINT(build/c++11)

#include "Firestore/core/src/util/executor.h"

namespace firebase {
namespace firestore {
namespace util {

/**
 * A task for an Executor to execute, either synchronously or asynchronously,
 * either immediately or after some delay.
 */
class Task {
 public:
  Task() = default;

  /**
   * Constructs a new Task for immediate execution.
   *
   * @param executor The Executor that owns the Task.
   * @param operation The operation to perform.
   */
  Task(Executor* executor,
       Executor::Operation&& operation);

  /**
   * Constructs a new Task for delayed execution.
   *
   * @param executor The Executor that owns the Task.
   * @param target_time The absolute time after which the task should execute.
   * @param tag The implementation-defined type of the task.
   * @param id The number identifying the specific instance of the task.
   * @param operation The operation to perform.
   */
  Task(Executor* executor,
       Executor::TimePoint target_time,
       Executor::Tag tag,
       Executor::Id id,
       Executor::Operation&& operation);

  Task(const Task& other) = delete;
  Task(Task&& other) noexcept;

  Task& operator=(const Task& other) = delete;
  Task& operator=(Task&& other) noexcept;

  ~Task();

  /**
   * Executes the operation if the Task has not already been executed or
   * canceled.
   */
  void Execute();

  /**
   * Detaches the task from the owning Executor, clearing the embedded Executor
   * pointer but does not mark the task done.
   */
  void Detach();

  /**
   * Disposes of the task, marking it done, and otherwise preventing it from
   * interacting with the rest of the system.
   */
  void Dispose();

  bool is_immediate() const {
    return tag_ == Executor::kNoTag;
  }

  Executor::Tag tag() const {
    return tag_;
  }

  Executor::Id id() const {
    return id_;
  }

  bool operator<(const Task& rhs) const;

 private:
  std::mutex mutex_;

  Executor* executor_ = nullptr;
  Executor::TimePoint target_time_;
  Executor::Tag tag_ = 0;
  Executor::Id id_ = 0;
  Executor::Operation operation_;

  // True if the operation has either been run or canceled. Non-default
  // constructors initialize this to false.
  bool done_ = true;
};

/**
 * Converts a delay into an absolute TimePoint representing the current time
 * plus the delay.
 *
 * @param delay The number of milliseconds to delay.
 * @return A time representing now plus the delay.
 */
Executor::TimePoint MakeTargetTime(Executor::Milliseconds delay);

}  // namespace util
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_UTIL_TASK_H_
