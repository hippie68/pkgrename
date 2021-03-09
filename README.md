# pkgrename
Renames PS4 PKG files based on local param.sfo information and predefined patterns.
Requires Bash script "sfo" (https://github.com/hippie68/sfo) in your $PATH environment variable.

Usage: `pkgrename [-hrs] [file/directory ...]`

Option -s searches the PS Store online for hopefully better file names. Could be useful for DLC especially.  
Option -r traverses directories recursively.

Further info on how to customize your file names is found inside the pkgrename script.
