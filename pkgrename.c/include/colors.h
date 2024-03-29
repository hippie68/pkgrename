#ifndef COLORS_H
#define COLORS_H

#define RESET "\033[0m"

#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define PURPLE "\033[0;35m"
#define CYAN "\033[0;36m"
#define GRAY "\033[0;37m"

#define BRIGHT_BLACK "\033[0;90m"
#define BRIGHT_RED "\033[0;91m"
#define BRIGHT_GREEN "\033[0;92m"
#define BRIGHT_YELLOW "\033[0;93m"
#define BRIGHT_BLUE "\033[0;94m"
#define BRIGHT_PURPLE "\033[0;95m"
#define BRIGHT_CYAN "\033[0;96m"
#define BRIGHT_GRAY "\033[0;97m"

#define BG_BLACK "\033[0;40m"
#define BG_RED "\033[0;41m"
#define BG_GREEN "\033[0;42m"
#define BG_YELLOW "\033[0;43m"
#define BG_BLUE "\033[0;44m"
#define BG_PURPLE "\033[0;45m"
#define BG_CYAN "\033[0;46m"
#define BG_GRAY "\033[0;47m"

#define BG_BRIGHT_BLACK "\033[0;100m"
#define BG_BRIGHT_RED "\033[0;101m"
#define BG_BRIGHT_GREEN "\033[0;102m"
#define BG_BRIGHT_YELLOW "\033[0;103m"
#define BG_BRIGHT_BLUE "\033[0;104m"
#define BG_BRIGHT_PURPLE "\033[0;105m"
#define BG_BRIGHT_CYAN "\033[0;106m"
#define BG_BRIGHT_GRAY "\033[0;107m"

#define set_color(color, stream)          \
    do {                                  \
        if (option_disable_colors == 0)   \
            fputs(color, stream);         \
    } while (0)

#endif
