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
    Baldur's Gate and Baldur's Gate II_ Enhanced Editions [UPDATE 1.02] [CUSA15671].pkg
    Rename? [Y]es [N]o [A]ll [E]dit [M]ix [O]nline [R]eset [C]hars [S]FO [Q]uit: y

- `[Y]es` Renames the file as seen
- `[N]o` Skips the file and drops all changes
- `[A]ll` Same as yes, but also for all future files
- `[E]dit` Prompts to manually edit the title
- `[M]ix` Converts the title to mixed case format
- `[O]nline` Searches the PS Store online for the current file's title information
- `[R]eset` Reverts all title changes
- `[C]hars` Shows special characters, if still present
- `[S]FO` Shows file's param.sfo information
- `[Q]uit` Exits the script immediately

You can easily customize the naming scheme in the customization section at the top of the script:

    pattern='$title [$category] [$title_id] [$releasegroup] [$backport].pkg'

Possible variables: title, category, backport, sdk, firmware, releasegroup                              
Plus original SFO strings: app_ver, content_id, title_id, version  

You can fully customize every aspect of the file name.
Further information is found inside the script's customization section.

For exFAT compatibility, by default certain characters are replaced with underscores (which is also customizable).

# pkgrename.c

pkgrename.c is a standalone, advanced version written in C. It currently works on GNU system and Windows. Please report any bugs or make feature requests at https://github.com/hippie68/pkgrename/issues.
As this version is still being worked on, please use it with care and try a read-only run first (option -n).

    Usage: pkgrename [options] file|directory [file|directory ...]
    
    Renames PS4 PKGs to match a file name pattern. The default pattern is:
    "%title% [%type%] [%title_id%] [%release_group%] [%backport%]"
    
    Available pattern variables:
    
      Name             Example
      ----------------------------------------------------------------------
      %app_ver%        "1.50"
      %backport%       "Backport" (*)
      %category%       "gp"
      %content_id%     "EP4497-CUSA05571_00-00000000000GOTY1"
      %firmware%       "4.70"
      %release_group%  "PRELUDE" (*)
      %sdk%            "4.50"
      %size%           "0.11 GB
      %title%          "The Witcher 3: Wild Hunt – Game of the Year Edition"
      %title_id%       "CUSA05571"
      %type%           "Update 1.50" (**)
      %uploader%       "John Doe" (*)
      %version%        "1.00"
    
      (*) A backport is currently detected by searching file names for existing
      strings "BP", "Backport", and "BACKPORT". The same goes for release
      groups and uploaders.
    
      (**) %type% is %category% mapped to "Game,Update %app_ver%,DLC,App".
      These default strings, up to 5, can be changed via option "--set-type":
        --set-type "Game,Patch,DLC,App,Other"
                   (no spaces before or after commas)
    
    Curly braces expressions:
    
      Pattern variables and other strings can be grouped together by surrounding
      them with curly braces. If an inner pattern variable turns out to be empty,
      the whole curly braces expression will be removed.
    
        Example 1 - %firmware% is empty:
          "%title% [FW %firmware%]"   => "Example DLC [FW ].pkg"  WRONG
          "%title% [{FW %firmware%}]" => "Example DLC.pkg"        CORRECT
    
        Example 2 - %firmware% has a value:
          "%title% [{FW %firmware%}]" => "Example Game [FW 7.55].pkg"
    
    Only the first occurence of each pattern variable will be parsed.
    After parsing, empty pairs of brackets, empty pairs of parentheses, and any
    remaining curly braces ("[]", "()", "{", "}") will be removed.
    
    Options:
      -f, --force           Force-prompt even when file names match.
      -m, --mixed-case      Automatically apply mixed-case letter style.
          --no-placeholder  Hide characters instead of using placeholders.
      -0, --leading-zeros   Show leading zeros in pattern variables %app_ver%,
                            %firmware%, %sdk%, and %version%.
      -n, --no-to-all       Do not prompt; do not actually rename any files.
      -o, --online          Automatically search online for %title%.
      -p, --pattern x       Set the file name pattern to string x.
          --placeholder x   Set the file name placeholder character to x.
      -r, --recursive       Traverse subdirectories recursively.
          --set-type x      Set %type% mapping to 5 comma-separated strings x.
      -u, --underscores     Use underscores instead of spaces in file names.
      -v, --verbose         Display additional infos.
      -y, --yes-to-all      Do not prompt; rename all files automatically.

How to compile (requires libcurl development files):

    gcc pkgrename.c src/*.c -o pkgrename -lcurl --pedantic -Wall

Or download a compiled Windows release at https://github.com/hippie68/pkgrename/releases.
