/*
 * Copyright 2020 Google LLC
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

#include "Firestore/core/src/util/task.h"

#include <chrono>  // NOLINT(build/c++11)
#include <cstdint>

#include "Firestore/core/src/util/log.h"

namespace firebase {
namespace firestore {
namespace util {

Task::Task(Executor* executor,
           Executor::Operation&& operation)
    : executor_(executor),
      target_time_(),  // immediate
      tag_(Executor::kNoTag),
      id_(UINT32_C(0)),
      operation_(std::move(operation)),
      done_(false) {
}

Task::Task(Executor* executor,
           Executor::TimePoint target_time,
           Executor::Tag tag,
           Executor::Id id,
           Executor::Operation&& operation)
    : executor_(executor),
      target_time_(target_time),
      tag_(tag),
      id_(id),
      operation_(std::move(operation)),
      done_(false) {
}

Task::Task(Task&& other) noexcept
    : executor_(other.executor_),
      target_time_(other.target_time_),
      tag_(other.tag_),
      id_(other.id_),
      operation_(std::move(other.operation_)),
      done_(other.done_) {
}

// Don't bother calling Dispose in the destructor because it just clears
// local state.
Task::~Task() = default;

Task& Task::operator=(Task&& other) noexcept {
  executor_ = other.executor_;
  target_time_ = other.target_time_;
  tag_ = other.tag_;
  id_ = other.id_;
  operation_ = std::move(other.operation_);
  done_ = other.done_;
  return *this;
}

void Task::Execute() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (done_) {
    // `done_` could mean that the this Task is disposed or the Executor is
    // already destroyed, so avoid taking any action involving the Executor.
    return;
  }

  operation_();

  done_ = true;
  if (executor_) {
    executor_->Complete(tag_, id_);
  }
}

void Task::Detach() {
  std::lock_guard<std::mutex> lock(mutex_);

  executor_ = nullptr;
}

void Task::Dispose() {
  std::lock_guard<std::mutex> lock(mutex_);

  executor_ = nullptr;
  done_ = true;
}

bool Task::operator<(const Task& rhs) const {
  // target_time_ and id_ are immutable after assignment; no lock required.

  // Order by target time, then by the order in which entries were created.
  if (target_time_ < rhs.target_time_) {
    return true;
  }
  if (target_time_ > rhs.target_time_) {
    return false;
  }

  return id_ < rhs.id_;
}

Executor::TimePoint MakeTargetTime(Executor::Milliseconds delay) {
  return std::chrono::time_point_cast<Executor::Milliseconds>(
      Executor::Clock::now()) +
         delay;
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase
