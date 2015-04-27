# tightvnc-remotehelp

To properly check out this repository, you must have a utf16
filter in your `~/.gitconfig`

An example filter:

    [filter "utf16"]
        clean = iconv -f utf-16le -t utf-8
        smudge = iconv -f utf-8 -t utf-16le
        required

This project requires Visual Studio 2013 to build.
