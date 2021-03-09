# pkgrename
Renames PS4 PKG files based on local param.sfo information and predefined patterns.
Requires Bash script "sfo" (https://github.com/hippie68/sfo) in your $PATH environment variable.

Usage: `pkgrename [-hfrs] [file/directory ...]`

Option -f forces user prompt even when file name matches pattern.  
Option -r traverses directories recursively.  
Option -s searches the PS Store online for hopefully better file names. Could be useful for DLC especially.

Further info on how to customize your file names is found inside the pkgrename script.
