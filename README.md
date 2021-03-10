# pkgrename
Renames PS4 PKG files based on local param.sfo information and predefined patterns.
Requires Bash script "sfo" (https://github.com/hippie68/sfo) in your $PATH environment variable.

Usage: `pkgrename [options] [file/directory ...]`

Options:

    -f  Force prompt when file name matches pattern
    -h  Display this help info
    -o  Default to online search
    -r  Traverse directories recursively

The script in action looks like this:

    $ pkgrename
    totally_not_helpful_filename.pkg
    Baldur's Gate and Baldur's Gate II_ Enhanced Editions [UPDATE v1.02] [CUSA15671].pkg
    Rename? [Y]es [N]o [A]ll [E]dit [O]nline [R]eset [C]hars [S]FO [Q]uit: y

- [Y]es Renames the file as seen
- [N]o Skips the file and drops all changes
- [A]ll Same as yes, but also for all future files
- [E]dit Prompts to manually edit the title
- [O]nline Searches the PS Store online for the current file's title information
- [R]eload Reverts all changes
- [C]hars Shows special characters, if still present
- [S]FO Shows file's param.sfo information
- [Q]uit Exits the script immediately

For exFAT compatibility, certain characters are replaced.
Further info on how to customize your file names is found inside the pkgrename script.
