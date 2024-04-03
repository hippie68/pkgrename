# pkgrename.c

pkgrename.c is a standalone, advanced version of the original Bash script, written in C. It currently works on Linux and Windows, possibly on other systems, too.


- [Manual](#manual)
  - [Tagging](#tagging)
  - [Querying (for scripts/tools)](#querying)
  - [How to compile](#how-to-compile)
- [For Windows users](#for-windows-users)
  - [How to run pkgrename.exe from anywhere](#how-to-run-pkgrenameexe-from-anywhere-with-modified-arguments)
- [Original Bash script](#pkgrename-original-bash-script-superseded-by-pkgrenamec)

# Manual

The program in action looks like this:

    $ pkgrename
       "totally_not_helpful_filename.pkg"
    => "Baldur's Gate and Baldur's Gate II_ Enhanced Editions [v1.02] [CUSA15671].pkg"
    [Y/N/A] [E]dit [T]ag [M]ix [O]nline [R]eset [C]hars [S]FO [L]og [H]elp [Q]uit: y

The program's help screen ("pkgrename --help"):

    Usage: pkgrename [OPTIONS] [FILE|DIRECTORY ...]
    
    Renames PS4 PKGs to match a file name pattern. The default pattern is:
    "%title% [%dlc%] [{v%app_ver%}{ + v%merged_ver%}] [%title_id%] [%release_group%] [%release%] [%backport%]"
    
    Pattern variables:
    ------------------
      Name             Example
      ----------------------------------------------------------------------
      %app%            "App"
      %app_ver%        "4.03"
      %backport%       "Backport" (*)
      %category%       "gp"
      %content_id%     "EP4497-CUSA05571_00-00000000000GOTY1"
      %file_id%        "EP4497-CUSA05571_00-00000000000GOTY1-A0100-V0100"
      %dlc%            "DLC"
      %firmware%       "10.01"
      %game%           "Game"
      %merged_ver%     "" (**)
      %other%          "Other"
      %patch%          "Update"
      %region%         "EU"
      %release_group%  "PRELUDE" (*)
      %release%        "John Doe" (*)
      %sdk%            "4.50"
      %size%           "19.34 GiB"
      %title%          "The Witcher 3: Wild Hunt – Game of the Year Edition"
      %title_id%       "CUSA05571"
      %true_ver%       "4.03" (**)
      %type%           "Update" (***)
      %version%        "1.00"
    
      (*) Backports not targeting 5.05 are detected by searching file names for the
      words "BP" and "Backport" (case-insensitive). The same principle applies to
      release groups and releases.
    
      (**) Patches and apps merged with patches are detected by searching PKG files
      for changelog information. If a patch is found, both %merged_ver% and
      %true_ver% are the patch version. If no patch is found or if patch detection
      is disabled (command [P]), %merged_ver% is empty and %true_ver% is %app_ver%.
      %merged_ver% is always empty for non-app PKGs.
    
      (***) %type% is %category% mapped to "Game,Update,DLC,App,Other".
      These 5 default strings can be changed via option "--set-type", e.g.:
        --set-type "Game,Patch %app_ver%,DLC,-,-" (no spaces before or after commas)
      Each string must have a value. To hide a category, use the value "-".
      %app%, %dlc%, %game%, %other%, and %patch% are mapped to their corresponding
      %type% values. They will be displayed if the PKG is of that specific category.
    
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
      - [Y]es      Rename the file as seen.
      - [N]o       Skip the file and drop all changes.
      - [A]ll      Same as yes, but also for all future files.
      - [E]dit     Prompt to manually edit the title.
      - [T]ag      Prompt to enter a release group or a release.
      - [M]ix      Convert the letter case to mixed-case style.
      - [O]nline   Search the PS Store online for title information.
      - [R]eset    Undo all changes.
      - [C]hars    Reveal special characters in the title.
      - [S]FO      Show file's param.sfo information.
      - [L]og      Print existing changelog data.
      - [H]elp     Print help.
      - [Q]uit     Exit the program.
      - [B]        Toggle the "Backport" tag.
      - [P]        Toggle changelog patch detection for the current PKG.
      - Backspace  Go back to the previous PKG.
      - Space      Return to the current PKG.
    
    Options:
    --------
      -c, --compact              Hide files that are already renamed.
          --disable-colors       Disable colored text output.
      -f, --force                Force-prompt even when file names match.
      -h, --help                 Print this help screen.
      -0, --leading-zeros        Show leading zeros in pattern variables %app_ver%,
                                 %firmware%, %merged_ver%, %sdk%, %true_ver%,
                                 %version%.
      -m, --mixed-case           Automatically apply mixed-case letter style.
          --no-placeholder       Hide characters instead of using placeholders.
      -n, --no-to-all            Do not prompt; do not actually rename any files.
                                 This can be used to do a test run.
      -o, --online               Automatically search online for %title%.
      -p, --pattern PATTERN      Set the file name pattern to string PATTERN.
          --placeholder X        Set the placeholder character to X.
          --print-tags           Print all built-in release tags.
      -q, --query                For scripts/tools: print file name suggestions, one
                                 per line, without renaming the files. A successful
                                 query returns exit code 0.
      -r, --recursive            Traverse subdirectories recursively.
          --set-type CATEGORIES  Set %type% mapping to comma-separated string
                                 CATEGORIES (see section "Pattern variables").
          --tagfile FILE         Load additional %release% tags from text file FILE,
                                 one tag per line.
          --tags TAGS            Load additional %release% tags from comma-separated
                                 string TAGS (no spaces before or after commas).
      -u, --underscores          Use underscores instead of spaces in file names.
      -v, --verbose              Display additional infos.
          --version              Print the current pkgrename version.
      -y, --yes-to-all           Do not prompt; rename all files automatically.

## Tagging

You can organize your PKGs by tagging them:

       "unnamed.pkg"
    => "Assassin's Creed Valhalla [v1.00] [CUSA18534].pkg"
    [Y/N/A] [E]dit [T]ag [M]ix [O]nline [R]eset [C]hars [S]FO [L]og [H]elp [Q]uit: t

    Enter new tag: dup  [DUPLEX]

Pressing Enter at this point will use word completion to apply the suggested value:

    => "Assassin's Creed Valhalla [v1.00] [CUSA18534] [DUPLEX].pkg"
    [Y/N/A] [E]dit [T]ag [M]ix [O]nline [R]eset [C]hars [S]FO [L]og [H]elp [Q]uit: 

The next time pkgrename is run on this file, it will recognize and preserve the tag.
You can add your own tag values, by using options --tags and/or --tagfile:

    pkgrename --tags "user500,Umbrella Corp.,john_wayne"
    pkgrename --tagfile tags.txt

If you use a text file, each line must contain a single tag:

    user500
    Umbrella Corp.
    john_wayne

## Querying
Use querying to receive name suggestions for your scripts/tools, for example:

    $ pkgrename -p '%title% [%true_ver%]' --query ps4.pkg ps3.pkg flower.gif subdirectory/
    Super Mario Bros. [1.00].pkg
    ps3.pkg
    flower.gif
    subdirectory/
    $ echo $?
    0

Files that can't be renamed (are not PKGs, are broken, etc.) and directories are returned unchanged.  
A successful query returns exit code 0. On error, the list is incomplete and a non-zero value is returned to indicate failure.

## How to compile...

...for Linux/Unix (requires libcurl development files):

    gcc -Wall -Wextra --pedantic pkgrename.c src/*.c -o pkgrename -lcurl -pthread -s -O3

...for Windows:

    x86_64-w64-mingw32-gcc-win32 -Wall -Wextra -pedantic pkgrename.c src/*.c -o pkgrename.exe --static -pthread -s -O3

Or download a compiled Windows release at https://github.com/hippie68/pkgrename/releases.

Please report bugs, make feature requests, or add missing data at https://github.com/hippie68/pkgrename/issues.

# For Windows users

On Windows 10/11, it is **strongly recommended** to activate the UTF-8 beta feature: Settings - Time & Language - Language - Administrative language settings - Change system locale... - Beta: Use Unicode UTF-8 for worldwide language support.  
For Windows 10 users it is recommended to use the new Windows Terminal application (which is now the default terminal in Windows 11) instead of the standard cmd.exe command prompt.
When using both the UTF-8 beta feature and Windows Terminal, pkgrename should work as intended.

If the UTF-8 beta feature is not used, file names that contain multibyte characters may cause this error:

    Could not read file system information: "weird_characters_????.pkg".

Such a PKG file can't be renamed then and will be skipped.

## How to run pkgrename.exe from anywhere with modified arguments

Put pkgrename.exe in a folder (you can also put other command line programs there).
Inside that folder, create a new batch file named "pkgrename.bat" and open it with Notepad.
Write the following lines, while replacing ARGUMENTS with your preferred arguments:

    @echo off
    pkgrename.exe ARGUMENTS %*
    
For example:

    @echo off
    pkgrename.exe --pattern "%%title%% [%%title_id%%]" --tagfile "C:\Users\Luigi\pkgrename_tags.txt" %*

Note: As seen above, if the batch file contains pattern variables, their percent signs need to be escaped by doubling them. For example, %title% must be changed to %%title%%.
    
Now click Start, type "env" and select "Edit environment variables for your account". Select "Path" and click edit. Select "New" and enter the folder where you put pkgrename.bat into. Close and reopen any opened command line windows for the changes to apply.

# pkgrename (original Bash script, superseded by pkgrename.c)
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
