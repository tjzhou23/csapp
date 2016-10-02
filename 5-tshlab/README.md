# Shell lab

Main functions to be implemented

* `eval`: Learn how to fork/execve, block/unblock signals.
* `builtin_bgfg`: Learn to use `kill` to send signal.
* `wait_fg`: A classical solution to race condition caused by `pause`. Learn to use `sigsuspend`.
* `sigint_handler`: Use `kill` to send signal.
* `sigstp_handler`: Use `kill` to send signal.
* `sigchld_handler`: Maintain the job list.