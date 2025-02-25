// This file is licensed under the Elastic License 2.0. Copyright 2021-present, StarRocks Limited.

#pragma once

#include <queue>

#include "exec/pipeline/pipeline_driver.h"
#include "exec/workgroup/work_group_fwd.h"
#include "util/factory_method.h"

namespace starrocks {
namespace pipeline {

class DriverQueue;
using DriverQueuePtr = std::unique_ptr<DriverQueue>;

class DriverQueue {
public:
    virtual ~DriverQueue() = default;
    virtual void close() = 0;

    virtual void put_back(const DriverRawPtr driver) = 0;
    virtual void put_back(const std::vector<DriverRawPtr>& drivers) = 0;
    // *from_executor* means that the executor thread puts the driver back to the queue.
    virtual void put_back_from_executor(const DriverRawPtr driver) = 0;
    virtual void put_back_from_executor(const std::vector<DriverRawPtr>& drivers) = 0;

    virtual StatusOr<DriverRawPtr> take(int worker_id) = 0;

    // Update statistics of the driver's workgroup,
    // when yielding the driver in the executor thread.
    virtual void update_statistics(const DriverRawPtr driver) = 0;

    virtual size_t size() = 0;
    bool empty() { return size() == 0; }
};

class SubQuerySharedDriverQueue {
public:
    void update_accu_time(const DriverRawPtr driver) {
        _accu_consume_time.fetch_add(driver->driver_acct().get_last_time_spent());
    }

    double accu_time_after_divisor() { return _accu_consume_time.load() / factor_for_normal; }

    std::queue<DriverRawPtr> queue;
    // factor for normalization
    double factor_for_normal = 0;

private:
    std::atomic<int64_t> _accu_consume_time = 0;
};

class QuerySharedDriverQueue : public FactoryMethod<DriverQueue, QuerySharedDriverQueue> {
    friend class FactoryMethod<DriverQueue, QuerySharedDriverQueue>;

public:
    QuerySharedDriverQueue() {
        double factor = 1;
        for (int i = QUEUE_SIZE - 1; i >= 0; --i) {
            // initialize factor for every sub queue,
            // Higher priority queues have more execution time,
            // so they have a larger factor.
            _queues[i].factor_for_normal = factor;
            factor *= RATIO_OF_ADJACENT_QUEUE;
        }
    }
    ~QuerySharedDriverQueue() override = default;
    void close() override;

    static const size_t QUEUE_SIZE = 8;
    // maybe other value for ratio.
    static constexpr double RATIO_OF_ADJACENT_QUEUE = 1.2;
    void put_back(const DriverRawPtr driver) override;
    void put_back(const std::vector<DriverRawPtr>& drivers) override;

    void put_back_from_executor(const DriverRawPtr driver) override;
    void put_back_from_executor(const std::vector<DriverRawPtr>& drivers) override;

    void update_statistics(const DriverRawPtr driver) override;

    // Return cancelled status, if the queue is closed.
    StatusOr<DriverRawPtr> take(int worker_id) override;

    size_t size() override;

private:
    SubQuerySharedDriverQueue _queues[QUEUE_SIZE];
    std::mutex _global_mutex;
    std::condition_variable _cv;
    bool _is_closed = false;
};

// All the QuerySharedDriverQueueWithoutLock's methods MUST be guarded by the outside lock.
class QuerySharedDriverQueueWithoutLock : public FactoryMethod<DriverQueue, QuerySharedDriverQueueWithoutLock> {
    friend class FactoryMethod<DriverQueue, QuerySharedDriverQueueWithoutLock>;

public:
    QuerySharedDriverQueueWithoutLock() {
        double factor = 1;
        for (int i = QUEUE_SIZE - 1; i >= 0; --i) {
            // initialize factor for every sub queue,
            // Higher priority queues have more execution time,
            // so they have a larger factor.
            _queues[i].factor_for_normal = factor;
            factor *= RATIO_OF_ADJACENT_QUEUE;
        }
    }
    ~QuerySharedDriverQueueWithoutLock() override = default;
    void close() override {}

    void put_back(const DriverRawPtr driver) override;
    void put_back(const std::vector<DriverRawPtr>& drivers) override;
    void put_back_from_executor(const DriverRawPtr driver) override;
    void put_back_from_executor(const std::vector<DriverRawPtr>& drivers) override;

    // Always return non-nullable value.
    StatusOr<DriverRawPtr> take(int worker_id) override;

    void update_statistics(const DriverRawPtr driver) override;

    size_t size() override { return _size; }

private:
    void _put_back(const DriverRawPtr driver);

    static constexpr size_t QUEUE_SIZE = 8;
    static constexpr double RATIO_OF_ADJACENT_QUEUE = 1.2;

    SubQuerySharedDriverQueue _queues[QUEUE_SIZE];

    size_t _size = 0;
};

// DriverQueueWithWorkGroup contains two levels of queues.
// The first level is the work group queue, and the second level is the driver queue in a work group.
class DriverQueueWithWorkGroup : public FactoryMethod<DriverQueue, DriverQueueWithWorkGroup> {
    friend class FactoryMethod<DriverQueue, DriverQueueWithWorkGroup>;

public:
    ~DriverQueueWithWorkGroup() override = default;
    void close() override;

    void put_back(const DriverRawPtr driver) override;
    void put_back(const std::vector<DriverRawPtr>& drivers) override;
    // When the driver's workgroup is not in the workgroup queue
    // and the driver isn't from a executor thread (that is, from the poller or new driver),
    // the workgroup's vruntime is adjusted to workgroup_queue.min_vruntime-ideal_runtime/2,
    // to avoid sloping too much time to this workgroup.
    void put_back_from_executor(const DriverRawPtr driver) override;
    void put_back_from_executor(const std::vector<DriverRawPtr>& drivers) override;

    // Return cancelled status, if the queue is closed.
    // Firstly, select the work group with the minimum vruntime.
    // Secondly, select the proper driver from the driver queue of this work group.
    StatusOr<DriverRawPtr> take(int worker_id) override;

    void update_statistics(const DriverRawPtr driver) override;

    size_t size() override;

private:
    // The schedule period is equal to SCHEDULE_PERIOD_PER_WG_NS * num_workgroups.
    static constexpr int64_t SCHEDULE_PERIOD_PER_WG_NS = 200'1000'1000;

    // This method should be guarded by the outside _global_mutex.
    template <bool from_executor>
    void _put_back(const DriverRawPtr driver);
    // This method should be guarded by the outside _global_mutex.
    workgroup::WorkGroup* _find_min_owner_wg(int worker_id);
    workgroup::WorkGroup* _find_min_wg();
    // The ideal runtime of a work group is the weighted average of the schedule period.
    int64_t _ideal_runtime_ns(workgroup::WorkGroup* wg);

    std::mutex _global_mutex;
    std::condition_variable _cv;
    // _ready_wgs contains the workgroups which include the drivers need to be run.
    std::unordered_set<workgroup::WorkGroup*> _ready_wgs;
    size_t _sum_cpu_limit = 0;

    bool _is_closed = false;
};

} // namespace pipeline
} // namespace starrocks