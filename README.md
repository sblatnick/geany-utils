# About
This repository contains various plugins compatible with [Geany](https://github.com/geany/geany) and [Geany Plugins](https://github.com/geany/geany-plugins)

A brief description of each plugin:
* `doc-panel` = Move the document panel to the right separate from the other side panel tabs.  Consider hiding tabs and using this as an alternative.  This adds keyboard shortcuts to switch open documents in order of appearance in the panel.
* `external-tools` = Call a shell script to do things like edit selected text, open programs, etc.
* `quick-find` = Side panel `ag` (fastest)|`ack` (faster)|`grep` (fast) of your project with a quick open to lines found by selecting rows in the table.
* `quick-line` = Go to a line with a modal overlay.
* `quick-opener` = Set up keyboard shortcuts to three paths to open a dialog to find files as you type.  One path is the project.  The other two you can configure.
* `quick-search` = Search for a word as you type in a modal overlay, highlighting all instances in a different color from the some of the geany plugins.  Set up keyboard shortcuts for next/previous.  Pre-populate with selected text. Bonus: Every time you select any text, it real-time highlights all instances.  Consider using this with the geany-plugins "Addons" checkbox for "Mark all occurrences of a word when double-clicking it" to highlight 3 different strings in 3 different colors.

# Prerequisites
```bash
  sudo apt install libgtk-3-dev intltool
```
# Building
## List plugins in this repo
```bash
  make
```
## Build specific plugin
```bash
  make doc-panel
```
## Place all built plugins in `~/.config/geany/plugins/`
```bash
  make install
```

# Testing one plugin
```bash
  #build plugin:
    make doc-panel
  #create a plugins directory for the shared library:
    mkdir plugins
  #copy so file into plugins directory:
    cp doc-panel.so plugins/
  #start instance of geany with it's own config:
    geany -i -c ./
  #install one plugin once ready:
    cp doc-panel.so ~/.config/geany/plugins/
```

# Quick Commands for local testing in a new instance
```bash
  clear;make doc-panel
  cp doc-panel.so plugins/;geany -i -c ./ *.c ../geany-1.37.1/src/*.c
```
