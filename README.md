# pkgrename.c

pkgrename.c is a standalone, advanced version of the original Bash script, written in C. It currently works on Linux and Windows, possibly on other systems, too.

The program in action looks like this:

    $ pkgrename
       "totally_not_helpful_filename.pkg"
    => "Baldur's Gate and Baldur's Gate II_ Enhanced Editions [v1.02] [CUSA15671].pkg"
    OK? [Y]es [N]o [A]ll [E]dit [T]ag [M]ix [O]nline [R]eset [C]hars [S]FO [Q]uit: y

The program's help screen ("pkgrename --help"):

    Usage: pkgrename [options] [file|directory ...]
    
    Renames PS4 PKGs to match a file name pattern. The default pattern is:
    "%title% [%dlc%] [{v%app_ver%}] [%title_id%] [%release_group%] [%release%] [%backport%]"
    
    Pattern variables:
    ------------------
      Name             Example
      ----------------------------------------------------------------------
      %app%            "App"
      %app_ver%        "1.50"
      %backport%       "Backport" (*)
      %category%       "gp"
      %content_id%     "EP4497-CUSA05571_00-00000000000GOTY1"
      %dlc%            "DLC"
      %firmware%       "4.70"
      %game%           "Game"
      %other%          "Other"
      %patch%          "Update"
      %release_group%  "PRELUDE" (*)
      %release%        "John Doe" (*)
      %sdk%            "4.50"
      %size%           "0.11 GiB"
      %title%          "The Witcher 3: Wild Hunt – Game of the Year Edition"
      %title_id%       "CUSA05571"
      %type%           "Update" (**)
      %version%        "1.00"
    
      (*) Backports not targeting 5.05 are detected by searching file names for
      strings "BP", "Backport", and "BACKPORT". The same principle applies
      to release groups and releases.
    
      (**) %type% is %category% mapped to "Game,Update,DLC,App,Other".
      These 5 default strings can be changed via option "--set-type", e.g.:
        --set-type "Game,Patch,DLC,-,-" (no spaces before or after commas)
      Each string must have a value. To hide a category, use the value "-".
      %app%, %dlc%, %game%, %other%, and %patch% are mapped to their corresponding
      %type% values. They will be displayed if the PKG is of that specific category.
    
      Only the first occurence of each pattern variable will be parsed.
      After parsing, empty pairs of brackets, empty pairs of parentheses, and any
      remaining curly braces ("[]", "()", "{", "}") will be removed.
    
    Curly braces expressions:
    -------------------------
      Pattern variables and other strings can be grouped together by surrounding
      them with curly braces. If an inner pattern variable turns out to be empty,
      the whole curly braces expression will be removed.
    
      Example 1 - %firmware% is empty:
        "%title% [FW %firmware%]"   => "Example DLC [FW ].pkg"  WRONG
        "%title% [{FW %firmware%}]" => "Example DLC.pkg"        CORRECT
    
      Example 2 - %firmware% has a value:
        "%title% [{FW %firmware%}]" => "Example Game [FW 7.55].pkg"
    
    Handling of special characters:
    -------------------------------
      - For exFAT compatibility, some characters are replaced by a placeholder
        character (default: underscore).
      - Some special characters like copyright symbols are automatically removed
        or replaced by more common alternatives.
      - Numbers appearing in parentheses behind a file name indicate the presence
        of non-ASCII characters.
    
    Interactive prompt:
    -------------------
      - [Y]es     Rename the file as seen.
      - [N]o      Skip the file and drops all changes.
      - [A]ll     Same as yes, but also for all future files.
      - [E]dit    Prompt to manually edit the title.
      - [T]ag     Prompt to enter a release group or a release.
      - [M]ix     Convert the letter case to mixed-case style.
      - [O]nline  Search the PS Store online for title information.
      - [R]eset   Undo all changes.
      - [C]hars   Reveal special characters in the title.
      - [S]FO     Show file's param.sfo information.
      - [Q]uit    Exit the program.
      - [B]       (Hidden) Toggle the "Backport" tag.
    
    Options:
    --------
      -f, --force           Force-prompt even when file names match.
      -h, --help            Print this help screen.
      -0, --leading-zeros   Show leading zeros in pattern variables %app_ver%,
                            %firmware%, %sdk%, and %version%.
      -m, --mixed-case      Automatically apply mixed-case letter style.
          --no-placeholder  Hide characters instead of using placeholders.
      -n, --no-to-all       Do not prompt; do not actually rename any files.
                            This can be used for a test run.
      -o, --online          Automatically search online for %title%.
      -p, --pattern x       Set the file name pattern to string x.
          --placeholder x   Set the placeholder character to x.
          --print-database  Print all current database entries.
      -r, --recursive       Traverse subdirectories recursively.
          --set-type x      Set %type% mapping to 5 comma-separated strings x.
      -u, --underscores     Use underscores instead of spaces in file names.
      -v, --verbose         Display additional infos.
          --version         Print release date.
      -y, --yes-to-all      Do not prompt; rename all files automatically.

How to compile for Linux (requires libcurl development files):

    gcc pkgrename.c src/*.c -o pkgrename -lcurl -s -O3

How to compile for Windows:

    x86_64-w64-mingw32-gcc-win32 pkgrename.c src/*.c -o pkgrename.exe --static -s -O3

Or download a compiled Windows release at https://github.com/hippie68/pkgrename/releases.

Please report bugs, make feature requests, or add missing data at https://github.com/hippie68/pkgrename/issues.

#### For Windows: How to run pkgrename.exe from anywhere with modified arguments:

Put pkgrename.exe in a folder (you can also put other command line programs there).
Inside that folder, create a new file named "pkgrename.bat" and open it with Notepad.
Write the following lines, while replacing "ARGUMENTS" with your preferred arguments (e.g.: "-m --pattern "%title% [%title_id%]"), including the quotes:

    @echo off
    pkgrename.exe "ARGUMENTS" %*

Now click Start, type "env" and select "Edit environment variables for your account". Select "Path" and click edit. Select "New" and enter the folder where you put pkgrename.bat into. Close and reopen any opened command line windows for the changes to apply.

# pkgrename (original Bash script)
Renames PS4 PKG files based on local param.sfo information and predefined patterns.
Requires Bash script or program "sfo" (https://github.com/hippie68/sfo) in your $PATH environment variable.

Usage: `pkgrename [options] [file|directory ...]`

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
