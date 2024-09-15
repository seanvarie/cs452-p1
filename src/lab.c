#include "lab.h"

char *get_prompt(const char *env)
{
    return env;
    //TODO: implement
}

int change_dir(char **dir)
{
    return 0;
    //TODO: implement
}

char **cmd_parse(char const *line)
{
    return &line;
    //TODO: implement
}

void cmd_free(char ** line)
{
    //TODO: implement
}

char *trim_white(char *line)
{
    return line;
    //TODO: implement
}

bool do_builtin(struct shell *sh, char **argv)
{
    return false;
    //TODO: implement
}

void sh_init(struct shell *sh)
{
    //TODO: implement
}

void sh_destroy(struct shell *sh)
{
    //TODO: implement
}

void parse_args(int argc, char **argv)
{
    //TODO: implement
}