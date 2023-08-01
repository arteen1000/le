# l(ightweight)e(macs)

Welcome to the *le* text editor. This is a fun little program and a very real text editor with some very strong capabilities. I would argue that it's more usable than certain ones in the market (nano -- and for me personally, vim).

	On that point, here is the particular use case:

You are working in the terminal, and for whatever reason, you don't want to use emacs. Maybe it's because `emacs -nw` takes too long to start up, or maybe it's just because for some reason (me) you don't like the way it looks and feels in the terminal. You type `vim`, and the trauma of your first-year computer science experience comes back. You struggle to figure out how to get to the end of the line, because it is not `C-e` but some completely unrelated token (`$`???). You wish you had a version of emacs that was super-lightweight with those barebones commands that you use so very often, cherishing every keystroke.

Also, for some reason, maybe due to [bug#64544](https://debbugs.gnu.org/cgi/bugreport.cgi?bug=64544), `emacsclient` is a pain to use on your system. You just wish there existed a `l(ightweight)e(emacs)`. `**le**`.

Unfortunately, you only found about `mg` after you already had the idea and was excited about writing it. Besides, you want it to have a vim look and feel, but the emacs key-bindings, because that is the vision.

## Installation and Usage

## Supported Keybindings

All correspond directly to their `emacs` equivalent.

Most of the keyboard (~95 ASCII characters) -> `self-insert-command`

`C-f` -> `forward-char`
`C-b` -> `backward-char`
`C-n` -> `next-line`
`C-p` -> `previous-line`
`C-v` -> `scroll-up-command`
`M-v` -> `scroll-down-command`
`M-<` -> `beginning-of-buffer`
`M->` -> `end-of-buffer`
`<delete>` -> `delete-backward-char` (`fn+<delete>` on macOS)
`<backspace>` -> `delete-forward-char` (`<delete>` on macOS)

To do (planned):

- more keybindings -> specifically kill-ring and killing
- 
Lots more support planned.