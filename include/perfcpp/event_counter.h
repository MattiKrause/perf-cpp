#pragma once

#include "config.h"
#include "counter_definition.h"
#include "counter_result.h"
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace perf {
/**
 * The EventCounter allows to specify events that should be counted by hardware performance counters and start/stop
 * counting.
 */
class EventCounter
{
  friend class MultiEventCounterBase;

public:
  /**
   * Mode how to schedule multiple events to physical hardware counters.
   */
  enum class Schedule : std::uint8_t
  {
    Append,   /// Events are placed on any hardware counter.
    Separate, /// Events are placed on a separate hardware counters.
    Group     /// Events are placed on the same hardware counter.
  };

  /**
   * Creates a copy from another event counter. The copy will hold the same counter definition, config, event sets, and
   * counter configurations. However, the copy will not be opened (this is also true for the counters).
   *
   * @param other EventCounter to copy from.
   * @return Copied event counter.
   */
  [[nodiscard]] static EventCounter copy_from_template(const EventCounter& other);

  explicit EventCounter(const CounterDefinition& counter_definition, Config config = {})
    : _counter_definitions(counter_definition)
    , _config(config)
  {
  }

  explicit EventCounter(Config config = {})
    : EventCounter(CounterDefinition::global(), config)
  {
  }

  EventCounter(const EventCounter&) = delete;
  EventCounter(EventCounter&&) noexcept = default;

  ~EventCounter();

  EventCounter& operator=(const EventCounter&) = delete;
  EventCounter& operator=(EventCounter&&) noexcept = delete;

  /**
   * Add the specified event to the list of countered performance events.
   * The event must exist within the counter definitions.
   *
   * @param event_name Name of the event.
   * @param schedule Request to schedule events anywhere (append), or to a single hardware counter (individual).
   * @return True, if the event could be added.
   */
  virtual bool add(std::string&& event_name, const Schedule schedule = Schedule::Append) { return add(event_name, schedule); }

  /**
   * Add the specified event to the list of monitored countered events.
   * The event must exist within the counter definitions.
   *
   * @param event_name Name of the event.
   * @param schedule Request to schedule events anywhere (append), or to a single hardware counter (individual).
   * @return True, if the event could be added.
   */
  virtual bool add(const std::string& event_name, Schedule schedule = Schedule::Append) = 0;

  /**
   * Add the specified events to the list of countered events.
   * The events must exist within the counter definitions.
   *
   * @param event_names List of names of the events.
   * @param schedule Request to schedule events anywhere (append), or to a single hardware counter (individual), or as a
   * group (all to the same hardware counter).
   * @return True, if the events could be added.
   */
  virtual bool add(std::vector<std::string>&& event_names, const Schedule schedule = Schedule::Append)
  {
    return add(event_names, schedule);
  }

  /**
   * Add the specified events to the list of countered performance events.
   * The events must exist within the counter definitions.
   *
   * @param event_names List of names of the counted events.
   * @param schedule Request to schedule events anywhere (append), or to a single hardware counter (individual), or as a
   * group (all to the same hardware counter).
   * @return True, if the events could be added.
   */
  virtual bool add(const std::vector<std::string>& event_names, Schedule schedule = Schedule::Append) = 0;

  /**
   * Add the specified event to the list of countered performance events.
   * The event can be read "live" without stopping the counter (only x86 hardware).
   * The event must exist within the counter definitions.
   *
   * @param event_name Name of the event.
   */
  virtual void add_live(std::string&& event_name) { add_live(event_name); }

  /**
   * Add the specified event to the list of countered performance events.
   * The event can be read "live" without stopping the counter (only x86 hardware).
   * The event must exist within the counter definitions.
   *
   * @param event_name Name of the event.
   */
   virtual void add_live(const std::string& event_name) = 0;

  /**
   * Add the specified events to the list of countered performance events.
   * The events can be read "live" without stopping the counter (only x86 hardware).
   * The events must exist within the counter definitions.
   *
   * @param event_names List of event names.
   */
  virtual void add_live(std::vector<std::string>&& event_names) = 0;

  /**
   * Opens hardware performance counters.
   */
  virtual void open() = 0;

  /**
   * Opens (if not already done) and starts recording performance counters.
   *
   * @return True, of the performance counters could be started.
   */
  virtual bool start() = 0;

  /**
   * Stops recording performance counters.
   */
  virtual void stop() = 0;

  /**
   * Closes the hardware performance counters.
   */
  virtual void close() = 0;

  /**
   * Returns the result of the performance measurement.
   *
   * @param normalization Normalization value, default = 1.
   * @return List of event names and values.
   */
  [[nodiscard]] virtual CounterResult result(std::uint64_t normalization = 1U) const = 0;

  /**
   * Performs a live read for every group without stopping the counter and writes it into the result input/output.
   * The reason for having an output parameter is to not allocate any memory during a lightweight read.
   *
   * @param result Output parameter to write the result without allocating any memory.
   */
  virtual void live_result(std::vector<double>& result) const noexcept = 0;

  /**
   * Performs a live read for every group without stopping the counter and writes it into the result input/output.
   * The reason for having an output parameter is to not allocate any memory during a lightweight read.
   *
   * @param result Output parameter to write the result without allocating any memory.
   * @param normalization  Normalization value.
   */
  virtual void live_result(std::vector<double>& result, std::uint64_t normalization) const noexcept = 0;

  /**
   * Performs a live read for every group without stopping the counter.
   *
   * @param counter_index Index of the counter to be read live.
   * @return The live value of the counter.
   */
  [[nodiscard]] virtual std::optional<double> live_result(std::uint64_t counter_index) const noexcept = 0;

  /**
   * Performs a live read for every group without stopping the counter.
   *
   * @param counter_index Index of the counter to be read live.
   * @param normalization Normalization value.
   * @return The live value of the counter.
   */
  [[nodiscard]] virtual std::optional<double> live_result(std::uint64_t counter_index,
                                                  std::uint64_t normalization) const noexcept = 0;

  /**
   * @return A list of event names that are added as live evens.
   */
  [[nodiscard]] virtual std::vector<std::string_view> live_event_names() const = 0;

  /**
   * @return Configuration of the counter.
   */
  [[nodiscard]] virtual const Config& config() const noexcept = 0;

  /**
   * Update the configuration of the counter.
   *
   * @param config New config.
   */
  virtual void config(const Config config) noexcept = 0;
};

/**
 * The LiveEventCounter grants access to live events of an existing EventCounter.
 * While live events can be read every time–using EventCounter–, this class enables to "start" (read the current value),
 * "stop" (read the current value again), and calculate the difference as a result without allocating memory for when
 * reading the values.
 */
class LiveEventCounter
{
public:
  explicit LiveEventCounter(const EventCounter& event_counter);
  LiveEventCounter(const LiveEventCounter&) = default;
  LiveEventCounter(LiveEventCounter&&) noexcept = default;
  ~LiveEventCounter() = default;
  LiveEventCounter& operator=(const LiveEventCounter&) = delete;
  LiveEventCounter& operator=(LiveEventCounter&&) noexcept = delete;

  /**
   * Retrieves the current value for every live counter and mark them as "start" value.
   */
  void start() noexcept;

  /**
   * Retrieves the current value for every live counter and mark them as "stop" value.
   */
  void stop() noexcept;

  /**
   * Calculates the difference between the start- and the stop values for the live event with the given name.
   * Returns 0 if the event name was not found.
   *
   * @param event_name Event to calculate the stop - start value for.
   * @return The difference between the stop and the start value, or 0 if the name was not found.
   */
  [[nodiscard]] double get(std::string_view event_name) const;

  /**
   * Calculates the difference between the start- and the stop values for the live event with the given name.
   * Returns 0 if the event name was not found.
   *
   * @param event_name Event to calculate the stop - start value for.
   * @param normalization Value to normalize the results.
   * @return The difference between the stop and the start value divided by the normalization value; or 0 if the name
   * was not found.
   */
  [[nodiscard]] double get(std::string_view event_name, std::uint64_t normalization) const;

private:
  /// EventCounter to access live events.
  const EventCounter& _event_counter;

  /// List of live events, received via EventCounter.
  std::vector<std::string_view> _event_names;

  /// List of (start, stop) tuples for all live events.
  std::vector<std::pair<std::optional<double>, std::optional<double>>> _counter_values;
};

class MultiEventCounterBase
{
public:
  MultiEventCounterBase() noexcept = default;
  MultiEventCounterBase(const MultiEventCounterBase&) = default;
  MultiEventCounterBase(MultiEventCounterBase&&) noexcept = default;
  virtual ~MultiEventCounterBase() = default;
  MultiEventCounterBase& operator=(const MultiEventCounterBase&) = default;
  MultiEventCounterBase& operator=(MultiEventCounterBase&&) noexcept = default;

  /**
   * Add the specified event to the list of countered performance events.
   * The event must exist within the counter definitions.
   *
   * @param event_name Name of the event.
   * @param schedule Request to schedule events anywhere (append), or to a single hardware counter (individual).
   * @return True, if the event could be added.
   */
  bool add(std::string&& event_name, EventCounter::Schedule schedule = EventCounter::Schedule::Append);

  /**
   * Add the specified counter to the list of monitored performance counters.
   * The counter must exist within the counter definitions.
   *
   * @param counter_name Name of the counter.
   * @param schedule Request to schedule events anywhere (append), or to a single hardware counter (individual).
   * @return True, if the counter could be added.
   */
  bool add(const std::string& counter_name, const EventCounter::Schedule schedule = EventCounter::Schedule::Append)
  {
    return add(std::string{ counter_name }, schedule);
  }

  /**
   * Add the specified counters to the list of monitored performance counters.
   * The counters must exist within the counter definitions.
   *
   * @param event_names List of names of the counters.
   * @param schedule Request to schedule events anywhere (append), or to a single hardware counter (individual), or as a
   * group (all to the same hardware counter).
   * @return True, if the counters could be added.
   */
  bool add(std::vector<std::string>&& event_names,
           const EventCounter::Schedule schedule = EventCounter::Schedule::Append)
  {
    return add(event_names, schedule);
  }

  /**
   * Add the specified counters to the list of monitored performance counters.
   * The counters must exist within the counter definitions.
   *
   * @param event_names List of names of the counters.
   * @param schedule Request to schedule events anywhere (append), or to a single hardware counter (individual), or as a
   * group (all to the same hardware counter).
   * @return True, if the counters could be added.
   */
  bool add(const std::vector<std::string>& event_names,
           EventCounter::Schedule schedule = EventCounter::Schedule::Append);

  /**
   * Stops recording performance counters.
   */
  void stop();

  /**
   * Closes the hardware performance counters.
   */
  void close();

  /**
   * Returns the result of the performance measurement.
   *
   * @param normalization Normalization value, default = 1.
   * @return List of counter names and values.
   */
  [[nodiscard]] CounterResult result(std::uint64_t normalization = 1U) const;

protected:
  [[nodiscard]] virtual std::vector<EventCounter>& event_counters() noexcept = 0;
  [[nodiscard]] virtual const std::vector<EventCounter>& event_counters() const noexcept = 0;
};

class StartableMultiEventCounterBase : public MultiEventCounterBase
{
public:
  StartableMultiEventCounterBase() noexcept = default;
  StartableMultiEventCounterBase(const StartableMultiEventCounterBase&) = default;
  StartableMultiEventCounterBase(StartableMultiEventCounterBase&&) noexcept = default;
  ~StartableMultiEventCounterBase() override = default;
  StartableMultiEventCounterBase& operator=(const StartableMultiEventCounterBase&) = default;
  StartableMultiEventCounterBase& operator=(StartableMultiEventCounterBase&&) noexcept = default;

  /**
   * Opens and starts all event counters.
   *
   * @return True, of the event counters could be started.
   */
  bool start();
};

/**
 * Wrapper for EventCounter to record counters on different user-level threads.
 * Each thread can start/stop its own counter.
 * The results can be aggregated or queried for a specific thread.
 */
class MultiThreadEventCounter final : public MultiEventCounterBase
{
public:
  MultiThreadEventCounter(const CounterDefinition& counter_definition, std::uint16_t num_threads, Config config = {});

  explicit MultiThreadEventCounter(const std::uint16_t num_threads, const Config config = {})
    : MultiThreadEventCounter(CounterDefinition::global(), num_threads, config)
  {
  }

  MultiThreadEventCounter(EventCounter&& event_counter, std::uint16_t num_threads);

  MultiThreadEventCounter(const EventCounter& event_counter, const std::uint16_t num_threads)
    : MultiThreadEventCounter(EventCounter::copy_from_template(event_counter), num_threads)
  {
  }

  MultiThreadEventCounter(const MultiThreadEventCounter&) = delete;
  MultiThreadEventCounter(MultiThreadEventCounter&&) noexcept = default;

  ~MultiThreadEventCounter() override { this->close(); }

  MultiThreadEventCounter& operator=(const MultiThreadEventCounter&) = delete;
  MultiThreadEventCounter& operator=(MultiThreadEventCounter&&) noexcept = default;

  /**
   * Opens and starts recording performance counters for the given thread.
   *
   * @param thread_id Id of the thread.
   * @return True, of the performance counters could be started.
   */
  bool start(std::uint16_t thread_id) { return this->_thread_local_counter[thread_id].start(); }

  /**
   * Stops and closes recording performance counters.
   *
   * @param thread_id Id of the thread.
   */
  void stop(std::uint16_t thread_id) { this->_thread_local_counter[thread_id].stop(); }

  /**
   * Returns the result of the performance measurement for a given thread.
   *
   * @param thread_id Id of the thread.
   * @param normalization Normalization value, default = 1.
   * @return List of counter names and values.
   */
  [[nodiscard]] CounterResult result_of_thread(const std::uint16_t thread_id, std::uint64_t normalization = 1U) const
  {
    return _thread_local_counter[thread_id].result(normalization);
  }

private:
  std::vector<perf::EventCounter> _thread_local_counter;

  [[nodiscard]] std::vector<EventCounter>& event_counters() noexcept override { return _thread_local_counter; }
  [[nodiscard]] const std::vector<EventCounter>& event_counters() const noexcept override
  {
    return _thread_local_counter;
  }
};

/**
 * Wrapper for EventCounter to record counters on different process ids (i.e., linux thread ids).
 * ProcessIds / ThreadIds have to be specified. The counter can be started/stopped at once.
 * The results will be aggregated.
 */
class MultiProcessEventCounter final : public StartableMultiEventCounterBase
{
public:
  MultiProcessEventCounter(const CounterDefinition& counter_list, std::vector<pid_t>&& process_ids, Config config = {});

  explicit MultiProcessEventCounter(std::vector<pid_t>&& process_ids, const Config config = {})
    : MultiProcessEventCounter(CounterDefinition::global(), std::move(process_ids), config)
  {
  }

  MultiProcessEventCounter(EventCounter&& event_counter, std::vector<pid_t>&& process_ids);

  MultiProcessEventCounter(const EventCounter& event_counter, std::vector<pid_t>&& process_ids)
    : MultiProcessEventCounter(EventCounter::copy_from_template(event_counter), std::move(process_ids))
  {
  }

  MultiProcessEventCounter(const MultiProcessEventCounter&) = delete;
  MultiProcessEventCounter(MultiProcessEventCounter&&) noexcept = default;

  ~MultiProcessEventCounter() override { this->close(); }

  MultiProcessEventCounter& operator=(const MultiProcessEventCounter&) = delete;
  MultiProcessEventCounter& operator=(MultiProcessEventCounter&&) noexcept = default;

private:
  std::vector<perf::EventCounter> _process_local_counter;

  [[nodiscard]] std::vector<EventCounter>& event_counters() noexcept override { return _process_local_counter; }
  [[nodiscard]] const std::vector<EventCounter>& event_counters() const noexcept override
  {
    return _process_local_counter;
  }
};

/**
 * Wrapper for EventCounter to record counters on different CPU cores.
 * CPU ids have to be specified. The counter can be started/stopped at once.
 * The results will be aggregated.
 */
class MultiCoreEventCounter final : public StartableMultiEventCounterBase
{
public:
  MultiCoreEventCounter(const CounterDefinition& counter_definition,
                        std::vector<std::uint16_t>&& cpu_ids,
                        Config config = {});

  explicit MultiCoreEventCounter(std::vector<std::uint16_t>&& cpu_ids, const Config config = {})
    : MultiCoreEventCounter(CounterDefinition::global(), std::move(cpu_ids), config)
  {
  }

  MultiCoreEventCounter(EventCounter&& event_counter, std::vector<std::uint16_t>&& cpu_ids);

  MultiCoreEventCounter(const EventCounter& event_counter, std::vector<std::uint16_t>&& cpu_ids)
    : MultiCoreEventCounter(EventCounter::copy_from_template(event_counter), std::move(cpu_ids))
  {
  }

  MultiCoreEventCounter(const MultiCoreEventCounter&) = delete;
  MultiCoreEventCounter(MultiCoreEventCounter&&) noexcept = default;

  ~MultiCoreEventCounter() override { this->close(); }

  MultiCoreEventCounter& operator=(const MultiCoreEventCounter&) = delete;
  MultiCoreEventCounter& operator=(MultiCoreEventCounter&&) noexcept = default;

private:
  std::vector<perf::EventCounter> _cpu_local_counter;

  [[nodiscard]] std::vector<EventCounter>& event_counters() noexcept override { return _cpu_local_counter; }
  [[nodiscard]] const std::vector<EventCounter>& event_counters() const noexcept override { return _cpu_local_counter; }
};
}