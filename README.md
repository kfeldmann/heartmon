Heartmon
========

Manage application availability by monitoring heartbeat signals
in its log stream.

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
