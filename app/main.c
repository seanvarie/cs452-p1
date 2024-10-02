#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../src/lab.h"

int main(int argc, char *argv[])
{
  parse_args(argc, argv);

  char *line = NULL;
  char **cmd = NULL;
  bool backgroundCurrentJob;
  bool isBuiltinCmd;
  struct shell sh;
  backgroundJobsList = NULL;
  sh_init(&sh);
  using_history();
  while ((line = readline(sh.prompt)))
  {

    trim_white(line);

    if (NULL == line[0])
    { // blank command
      free(line);
      backgroundJobsList = evaluate_background_jobs(backgroundJobsList, false);
      continue;
    }

    add_history(line);

    backgroundCurrentJob = false;
    if (BACKGROUND_CHAR == line[strlen(line) - 1])
    {
      backgroundCurrentJob = true;
      line[strlen(line) - 1] = NULL;
    }

    cmd = cmd_parse(line);

    if (strcmp(cmd[0], EXIT_COMMAND) == 0)
    {
      break;
    }
    isBuiltinCmd = do_builtin(NULL, cmd);
    backgroundJobsList = evaluate_background_jobs(backgroundJobsList, false);
    if (!isBuiltinCmd)
    {
      int forkResult = fork(); // TODO: error handling?
      if (forkResult == 0)
      {
        execute_command(cmd, sh, backgroundCurrentJob);
        printf("command failed:");
        for (int i = 0; NULL != cmd[i]; i++)
        {
          printf(" %s", cmd[i]);
        }
        printf("\n");
        exit(0);
      }
      if (backgroundCurrentJob)
      {
        backgroundJobsList = add_to_background_list(backgroundJobsList, cmd, forkResult);
      }
      else
      {
        waitpid(forkResult, NULL, 0); // TODO: error handling
        cmd_free(cmd);
        cmd = NULL;
      }
      /* take back control of the terminal.  */
      tcsetpgrp(sh.shell_terminal, sh.shell_pgid);
    }
    else
    {
      cmd_free(cmd);
      cmd = NULL;
    }

    free(line);
    line = NULL;
  }

  cmd_free(cmd);
  cmd = NULL;
  free(line);
  line = NULL;

  destroy_background_list(backgroundJobsList);
  sh_destroy(&sh);

  return 0;
}
