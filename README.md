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
```
