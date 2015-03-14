### Notice 2009-04-12 ###
Version **1.1.1** is up. Last version caused all chat sessions to crash, it is now fixed.

Version **1.1.0** is up. It fixes some memory leaks, fixes the nicks in the logs. And there is a new version for windows. For complete list of changes, see the [CHANGELOG](CHANGELOG.md)

Update from any earlier version is highly recommended.

For windows users, there is a newer simplier installation. Remove the old mimetex.bat file, it is no longer in use. Simply put mimetex.exe in C:\Windows\ and the plugin in the plugin folder. Make sure the old plugin dll is removed.

Also make sure to check out the wiki pages for help on mimeTeX and mathTeX installation.

### Wanted ###
Nothing right now. Maybe compiling a 64bit version of the plugin. See issues.

## Screenshots ##
|![http://pidgintex.googlecode.com/svn/trunk/pidginTeXmsg.png](http://pidgintex.googlecode.com/svn/trunk/pidginTeXmsg.png)|![http://pidgintex.googlecode.com/svn/trunk/pidginTeXconfig.png](http://pidgintex.googlecode.com/svn/trunk/pidginTeXconfig.png)|
|:------------------------------------------------------------------------------------------------------------------------|:------------------------------------------------------------------------------------------------------------------------------|
|_Sending messages on a dark grey GTK-theme._|_Config menu_ |

## What is it ##
pidginTeX is a **fast** plugin for the instant messaging client [Pidgin](http://www.pidgin.im) and uses [mimeTeX](http://www.forkosh.com/mimetex.html) or [mathTeX](http://www.forkosh.com/mathtex.html) to render [LaTeX](http://en.wikipedia.org/wiki/LaTeX) expressions in messages and chat.
It is intended for [math](http://en.wikipedia.org/wiki/Math) but mathTeX is fully compatible with LaTeX and any package can be used and mimeTeX also supports many other features. See _Why use pidginTeX_ below.

## Requirements ##
Either mimeTeX or mathTeX is needed. See links for more information. If you still can't decide, see the [FAQ](FAQ.md).
### mimeTeX ###
is a small (< 1MB) program with no other dependencies.
### mathTeX ###
depends on latex and dvipng for rendering.

## How to use ##
The syntax is simple, write `$$latex expression$$` anywhere in a message and an image will appear in the message log.

### Example ###
```
"Hey John, how do I solve this? $$a x^2 + b x + c = 0$$ ?"
"Well Bill, you can use this $$x_{1,2} = \frac{-b \pm\sqrt{b^2-4 a c}}{2 a}$$
```

## Why use pidginTeX? ##
It is built upon pidgin-LaTeX but with many improvements such as
  * Much faster render time (less than a second)
  * Transparent images
  * Does not require latex and ImageMagick when using mimeTeX, suitable for system where the user doesn't want to install alot of software.
  * Many options, such as font size, color, styles
  * The ability to send the rendered images. Usefull when the  reciever doesn't have

## Installation ##
When compiling from source in Linux
  1. Obtain the pidgin-dev package
  1. Check out the a copy from the repository or download the source tarball.
  1. `$ make install`   ( for user )
  1. or
  1. `$ make install PREFIX=/usr` ( all users, change /usr if needed )
  1. Start Pidgin

When downloading the library, place the file in Pidgins plugin directory and restart Pidgin. See further [mimeTeXmathTeXHowto](mimeTeXmathTeXHowto.md) on how to compile the required libraries.