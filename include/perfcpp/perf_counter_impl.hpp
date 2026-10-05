#pragma once
#include "event_counter.h"

namespace perf {
class EventCounterImpl: EventCounter
{
  private:

  /// List of event names and codes.
  const CounterDefinition& _counter_definitions;

  /// The configuration of counters (include user, kernel, etc.).
  Config _config;

  /// List of requested events and metrics that are added to groups. This list is only to track the order and
  /// configuration of the user's requested events.
  RequestedEventSet _requested_event_set;

  /// List of requested live events. This list is only to track the order and
  /// configuration of the user's request.
  RequestedEventSet _requested_live_event_set;

  /// Hardware counter groups holding performance counters that are started, stopped, and read. The bool indicates if
  /// that group is "open", meaning no counter can or should be added (false), because the group is full or the user
  /// wanted to schedule the counters together without any other.
  std::vector<std::pair<Group, bool>> _hardware_event_groups;

  /// List of counters that are marked to be read "live" (without stopping) using the "rdpmc" instruction (only
  /// implemented on x86 hardware).
  std::vector<Counter> _hardware_live_counters;

  /// Start and stop time points for time events.
  std::pair<std::chrono::steady_clock::time_point, std::chrono::steady_clock::time_point> _start_and_end_time;

  /// Flag indicating if the EventCounter was opened. Opens automatically on startup at the latest.
  bool _is_opened{ false };

  EventCounter(const CounterDefinition& counter_definition,
               const Config config,
               RequestedEventSet requested_event_set,
               RequestedEventSet requested_live_event_set)
    : _counter_definitions(counter_definition)
    , _config(config)
    , _requested_event_set(std::move(requested_event_set))
    , _requested_live_event_set(std::move(requested_live_event_set))
  {
  }

  /**
   * @return The number of opened (or to open) counters (groups or group leaders and live counters).
   */
  [[nodiscard]] std::size_t size() const noexcept
  {
    return _hardware_event_groups.size() + _hardware_live_counters.size();
  }

  /**
   * Extracts information (counter/metric name, hardware counter configuration, flag if is included into events) from
   * the given event into a given result vector. The result vector can be used to schedule the events, based on the user
   * request.
   *
   * @param name Name of the event to add.
   * @param is_visible_in_results Indicates if the added event/metric/time should be visible in the results.
   * @param events List to extend the requested events. If the event is a single hardware event, the list will
   * have one entry. If the event is a metric, the list will have multiple entries.
   */
  void unfold(const std::string& name,
              bool is_visible_in_results,
              std::vector<std::pair<RequestedEvent, std::optional<CounterConfig>>>& events) const;

  /**
   * Adds the provided event to the given result vector.
   * If the event is already in the result vector, only the visibility (is_shown_in_results) will be adjusted.
   *
   * @param pmu_name Name of the PMU.
   * @param event_name Name of the event.
   * @param event_config Configuration of the counter.
   * @param is_shown_in_results Visibility.
   * @param requested_events List of requested events.
   */
  static void add(std::string_view pmu_name,
                  std::string_view event_name,
                  const CounterConfig& event_config,
                  bool is_shown_in_results,
                  std::vector<std::pair<RequestedEvent, std::optional<CounterConfig>>>& requested_events);

  /**
   * Schedules the given events based on the request into hardware groups and places the event names in the
   * user-requested event set. If the events do not fit (e.g., based on the request; too many counters requested to be
   * placed on the same hardware counter), the method will throw an exception to let the user know.
   *
   * @param events List of events to schedule.
   * @param schedule Request of the user.
   */
  void schedule(std::vector<std::pair<RequestedEvent, std::optional<CounterConfig>>>&& events, Schedule schedule);

  /**
   * Try to append the given event to any hardware counter.
   *
   * @param event Event to append.
   * @param event_config Configuration of the event.
   * @return True, if the event could be appended to any hardware counter. False, otherwise.
   */
  [[nodiscard]] bool append_to_any_hardware_counter(RequestedEvent& event, const CounterConfig& event_config);

  /**
   * Tries to create a new group and appends the given event.
   *
   * @param event Event to append.
   * @param event_config Configuration of the event.
   * @param is_keep_open If true, further events can be added in the future. Otherwise, the event will be the only one.
   */
  void create_new_group(RequestedEvent& event, const CounterConfig& event_config, bool is_keep_open);
};

}