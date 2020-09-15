---
title: Another tmux cheatsheets
---

# tmux

Another tmux cheatsheets.

You can check my [.tmux.conf file](https://github.com/thobiast/dotfiles).

### Session

| tmux command | description |
| :----------  | :---------  |
| tmux new -s my\_session | create a new tmux session named "my\_session" |
| tmux attach -t my\_session | attaches an existing session named "my\_session" |
| tmux switch -t my\_session | switch to a new existing session named "my\_session" |
| tmux ls | list all sessions |
| tmux detach (or prefix + d) | detach currently session |
| tmux kill-session -t my\_session | kill existing session named "my\_session" |
| prefix + s | choose session to attach from a list |
| prefix + $ | rename current session |

### Windows

| tmux command | description |
| :----------  | :---------  |
| prefix + w | choose a window from a list |
| prefix + c | create a new window |
| prefix + 0-9 | move to window number |
| prefix + , | rename window |
| :move-window -t {session-name}: | move current window to another tmux session named "session-name" |
| :move-window -t 2 | move current window to index 2. If target index 2 does not exist |
| :swap-window -t 0     | to swap the current window with the top window (index 0) |
| :kill-window | kill current window |

### Panes

| tmux command | description |
| :----------  | :---------  |
| :set synchronize-panes (on\|off) | synchronize input Panes |
| :break-pane (or prefix + !) | move the current pane into a new separate window |
| :kill-pane (or prefix + x) | kill current pane |
| :display-panes (or prefix + q) | show pane numbers |
| prefix + z | zoom pane (in and out) |
| prefix + {  | move the current pane left |
| prefix + }  | move the current pane right |
| prefix space  | switch to the next layout |
| :join-pane -t 5 | move current window or pane to a new pane on window index 5 |

### Others

| tmux command | description |
| :----------  | :---------  |
| prefix + w | list and select to attach session, windows and pane numbers.  |
| prefix + ? | list shortcuts |
| source-file ~/.tmux.conf | reload tmux config file |
