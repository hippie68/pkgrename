# pkgrename
Renames PS4 PKG files based on local param.sfo information and predefined patterns.
Requires Bash script "sfo" (https://github.com/hippie68/sfo) in your $PATH environment variable.

Usage: `pkgrename [-fhor] [file/directory ...]`

Options:  
  -f  Force prompt when file name matches pattern  
  -h  Display this help info"  
  -o  Default to online search"  
  -r  Traverse directories recursively"

The script in action looks like this:

    totally_not_helpful_filename.pkg
    Baldur's Gate and Baldur's Gate II_ Enhanced Editions [UPDATE v1.02] [CUSA15671].pkg
    Rename? [Y]es [N]o [A]ll [E]dit [O]nline [R]eload [S]FO [Q]uit: 

For exFAT compatibility, certain characters are replaced.
Further info on how to customize your file names is found inside the pkgrename script.
