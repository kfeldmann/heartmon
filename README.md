Heartmon
========

Manage application availability by monitoring heartbeat signals
in the log stream.

![Heartmon Concept Diagram][https://github.com/kfeldmann/heartmon/raw/master/heartmon-concept-v03.png]

### Usage

```
Usage: ./heartmon -d heartmon_config_directory \
       [-i include_filter] [-e exclude_filter] \
       [-w warn_seconds] [-c crit_seconds] [-r restart_seconds]

<heartmon_config_directory>/
	app/
		0: <application_binary>
		1: <argv[1]>
		2: <argv[2]>
		<n>: <argv[n]>
	fifo/
		0: <path_to_fifo[0]>
		1: <path_to_fifo[1]>
		2: <path_to_fifo[2]>
		<n>: <path_to_fifo[n]>
	log/
		0: <logger_binary>
		1: <argv[1]>
		2: <argv[2]>
		<n>: <argv[n]>
```
Heartmon will always re-spawn an app process if it terminates. There is
no support for "run once" behavior. Each spawning of the app will be
logged with a LOG_NOTICE message inserted into the log stream.

Filters use simple substrings, applied to one line of the log stream
at a time.

All filters are optional. If none are supplied, all lines will be counted
as heartbeats.

All action thresholds are optional. If none are supplied, no heartbeat
monitoring will be performed. The only action that might be taken is
that the app will be re-spawned if it terminates.

Multiple re-spawns will be limited in the following way: After a
re-spawn, there will be a grace period before any of the action timers
start counting down (warn, crit, restart). The grace period will be equal
to the shortest non-zero timer threshold.

If the "warn" threshold is exceeded, a LOG_WARNING message will be
inserted into the log stream.

If the "crit" threshold is exceeded, a LOG_WARNING message will be
inserted into the log stream.

If the "restart" threshold is exceeded, an LOG_ERR message will be
inserted into the log stream, and the app process will be sent a KILL
signal. After the process terminates, it will be re-spawned.

### Heartmon Features

- Run app and log-collector processes. Read the log stream from the app
  and/or fifo(s) and pipe it to the log collector
- If log collector terminates or stops consuming the log stream, buffer
  log data and re-spawn the log collector process
- Inject a log record for the re-spawn event into the log stream
- If app terminates, re-spawn the app process
- Inject a log record for the re-spawn event into the log stream
- Watch the log stream coming from the app for a “heartbeat.” Any line
  in the log stream that matches the configured filters is counted as a
  "heartbeat." The threshold values specify how much time can elapse
  between successive heartbeats
- What to look for is configurable using include and exclude filters
- The time thresholds for warning, critical, and restart actions are
  configurable
- If thresholds are exceeded, actions are taken, including:
  - Inject a log record into the log stream
- Kill and re-spawn the app process

### Application Best Practice

- Applications that are single-threaded can provide a direct heartbeat
  into the log stream
- Applications that have multiple threads should be careful not to have
  threads that are not represented by the heartbeat
   - For example, if the main thread generates heartbeat log entries, but
     service requests are handed off to a pool of worker threads, there
     might be a situation in which the main thread is successfully
     sending heartbeats, yet the worker threads are all blocking and
     unable to handle requests
   - Either have the heart-beating thread responsible for knowing the
     health of the other threads, or else have it send a “noop” request
     through the app. The request itself can produce the log entry used
     as the heartbeat
- Apps should be stateless and "crash-only." Terminating the app should
  never produce data inconsistencies, leave stale locks, etc.
- As a last-ditch effort, an external URL monitor can “ping” the service
  periodically, producing an identifiable heartbeat log entry. This is
  the least desirable implementation because failures of the monitoring
  system, network, etc. will cause restarts although there may be
  nothing wrong with the app


