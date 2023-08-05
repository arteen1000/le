# l(ightweight)e(macs)

Welcome to the *le* text editor. It is in active development, and this README will be updated with more details shortly.

Currently it is fully functional as a text-viewer, with text-editor functionality in progress, and key-bindings being added as appropriate, with fixes and editor functionality being both optimized and ensured correct and adhering to what I would see from something like `vim` or `emacs` as I go along.

## Acknowledgements

[kilo](https://github.com/antirez/kilo/blob/master/kilo.c) helped me to see how the general problem of a text-editor could be approached.

[emacs](https://github.com/emacs-mirror/emacs) is both a hobby and my text-editor and responsible for the key-bindings. I strongly recommend building from source with native compilation. Here are instructions that I follow for macOS:

```
$ git clone https://github.com/emacs-mirror/emacs.git && cd emacs
$ export CC=clang
$ ./autogen.sh
$ ./configure --with-native-compilation=aot --without-pop --with-ns
$ make -j12 # adjust this to be a couple lower than your logical CPUs
$ src/emacs -Q # test it to make sure all is well
$ make install
$ rm -rf /Applications/Emacs.app
$ mv nextstep/Emacs.app /Applications
$ cd .. && rm -rf emacs
```
Make sure you have your `.emacs.d` or `.emacs` saved prior to this as it may be overriden.
