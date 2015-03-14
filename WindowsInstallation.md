# Short information #
Only mimetex is supported on windows, since mathtex simply doesn't have a windows port.
There is no need to install latex, ImageMagick or any other software other than mimetex.exe.

# If you are upgrading #
Do make sure you have deleted the old mimetex.bat (from every folder). It's deprecated and will never be used again.

# Version 1.1.0 and newer #
## Easy version ##
  * Get the plugin (1.1.0). Put it in C:\Program Files\Pidgin\plugins (or wherever you installed Pidgin)
  * Get [mimetex.zip](http://www.forkosh.com/mimetex.exe/windows/mimetex.zip). Extract mimetex.exe into C:\Windows
  * Start Pidgin and select the plugin.

## Hardcode version ##
  * Get the plugin
  * Put mimetex.exe somewhere in your path. Either C:\Windows, system, system32 or add a new one in your system variables.
  * Start Pidgin and select the plugin.

# Version 1.0.7 and older #

## Easy version ##
  * Get the [plugin (1.0.4)](http://pidgintex.googlecode.com/files/pidginTeX-1.0.4.dll) and place it in the plugins folder (Default C:\Program Files\Pidgin\plugins\)
  * Get [mimetex](http://www.forkosh.com/mimetex.exe/windows/mimetex.zip)
  * Extract mimetex.exe to your plugin folder (Default C:\Program Files\Pidgin\plugins\)
  * Get [mimetex.bat](http://pidgintex.googlecode.com/files/mimetex.bat) from the download page. Place it in C:\Windows
  * If C:\Program Files\Pidgin\plugins is the correct folder on your system, you are done, if not you need to perform the last step:
  * Edit the line in mimetex.bat (open mimetex.bat in notepad) and change the the path to where you have placed mimetex.exe (Example "C:\My Folder\Somewhere\mimetex.exe" )

## Hardcore version ##
  * Install the [plugin (1.0.4)](http://pidgintex.googlecode.com/files/pidginTeX-1.0.4.dll)
  * Get [mimetex](http://www.forkosh.com/mimetex.exe/windows/mimetex.zip)
  * Extract (or compile) mimetex anywhere not included in your PATH
  * Get [mimetex.bat](http://pidgintex.googlecode.com/files/mimetex.bat), edit the path inside it to where you placed mimetex.exe
  * Place mimetex.bat somewhere that included in your PATH