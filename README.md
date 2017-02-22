Vt-emulator is a terminal emulator based to [Konsole](https://konsole.kde.org/download.php) ([repo](https://cgit.kde.org/konsole.git)) without graphical interface.

# Demo

```bash
bjam terminal_browser
cd browser
python -m SimpleHTTPServer 4104
script -f >(../bin/*/release/terminal_browser screen.json)
xdg-open http://localhost:4104/view_browser.html
ls -R
```

# Build

$ `bjam` or `bjam toolset=compiler` (see http://www.boost.org/build/doc/html/bbv2/overview/configuration.html)
