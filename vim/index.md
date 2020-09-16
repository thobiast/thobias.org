---
title: Another vim cheatsheets
---

# vim

Another vim cheatsheets.


### Buffers

| vim command | description |
| :---------- | :---------- |
| :ls           | list buffers |
| :bn           | go to the next buffer |
| :bp or ctrl-6 | go to the previous buffer |
| :b#           | go to buffer number '#' |
| :bd           | close current buffer |
| :ba           | open all buffers in new windows (horizontally) |
| :vert ba      | open all buffers in new windows (vertically) |

### Windows

| vim command | description |
| :---------- | :---------- |
| :sp file    | open file in a new buffer (horizontal split) |
| :vsp file   | open file in a new buffer (vertical split) |
| ctrl-w q    | close current window |
| ctrl-w w    | switch between windows |
| ctrl-w Arrow keys | Move cursos between windows |
| ctrl-w +    | increase size of current window |
| ctrl-w -    | reduce size of current window |
| ctrl-w <    | decrease current window width |
| ctrl-w >    | Increase current window width |
| ctrl-w =    | make all windows equal (size) |
| ctrl-w x    | exchange current window with next |
| ctrl-w o    | make current window only one in screen |
| ctrl-w T    | move the current window into its own tab |

### Tabs

| vim command | description |
| :---------- | :---------- |
| :tabnew     | open an empty new tab |
| :tabe file  | open file in a new tab |
| :tabn or ctrl-page down or gt | move to next tab |
| :tabp or ctrl-pageup or gT | move to left tab |
| :tab ball | open all buffers in new tabs |
| :tab only | close all other tabs |
| <number>gt | move to tab number (index start from 0) |


