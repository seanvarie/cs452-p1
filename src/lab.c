#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>
#include <readline/history.h>

#include "lab.h"

#define DEFAULT_PROMPT "shell>"
#define SPACE_STR " " // as cmd_parse is currently written, this must be a single character string

char *get_prompt(const char *env)
{
  char *retVal;

  char *envPrompt = getenv(env);

  if (envPrompt == NULL)
  {
    retVal = malloc(sizeof(char) * (strlen(DEFAULT_PROMPT) + 1));
    strcpy(retVal, DEFAULT_PROMPT);
  }
  else
  {
    retVal = malloc(sizeof(char) * (strlen(envPrompt) + 1));
    strcpy(retVal, envPrompt);
  }

  return retVal;
}

int change_dir(char **dir)
{
  int retVal = -1;
  char *directory = dir[1];
  struct passwd *userInfo;

  if (directory == NULL)
  { // try getting home from environment
    directory = getenv(HOME_ENV_VARIABLE);
  }
  if (directory == NULL)
  { // try getting home from uid
    userInfo = getpwuid(getuid());
    directory = userInfo->pw_dir;
  }

  retVal = chdir(directory);

  return retVal;
}

char **cmd_parse(char const *line)
{
  // getting pointer to first non-whitespace character of line (must be done for cmd_free to work properly)
  // this is unneccesarry if trim_white is called on line first, but is here for safety in case it isn't
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"//lineWithoutLeadingWhitespace is not being used to modify the data of line
  char *lineWithoutLeadingWhitespace = line;
  #pragma GCC diagnostic pop
  while (isspace((unsigned char)*lineWithoutLeadingWhitespace))
  {
    lineWithoutLeadingWhitespace++;
  }

  char *lineCopy = malloc(sizeof(char) * (strlen(lineWithoutLeadingWhitespace) + 1));

  strcpy(lineCopy, lineWithoutLeadingWhitespace);

  long max_args = sysconf(_SC_ARG_MAX);

  char **retVal = malloc(sizeof(char *) * (max_args + 2));

  // split lineCopy into tokens and store pointers to each in retVal
  int i = 0;
  retVal[i] = strtok(lineCopy, SPACE_STR);
  for (i = 1; i < max_args && (retVal[i] = strtok(NULL, SPACE_STR)) != NULL; i++)
    ;
  retVal[i] = NULL; // only neccesary for if we hit max_args

  return retVal;
}

void cmd_free(char **line)
{
  if (line == NULL)
  {
    return;
  }

  free(line[0]);
  free(line);
}

char *trim_white(char *line)
{
  // Find the first non whitespace character
  char *start = line;
  while (isspace((unsigned char)*start))
  {
    start++;
  }

  // If the string is all whitespace
  if (*start == '\0')
  {
    *line = '\0'; // Set the line to an empty string
    return line;
  }

  // Find the last non whitespace character
  char *end = line + strlen(line) - 1;
  while (end > start && isspace((unsigned char)*end))
  {
    end--;
  }

  *(end + 1) = '\0';

  // Move the trimmed string to the beginning of line
  memmove(line, start, end - start + 2);

  return line;
}

bool do_builtin(struct shell *sh, char **argv)
{
  UNUSED(sh)
  if (strcmp(argv[0], EXIT_COMMAND) == 0)
  {
    return true;
  }
  else if (strcmp(argv[0], CHANGE_DIRECTORY_COMMAND) == 0)
  {
    change_dir(argv);
    return true;
  }
  else if (strcmp(argv[0], LIST_JOBS_COMMAND) == 0)
  {
    evaluate_background_jobs(backgroundJobsList, true);
    return true;
  }
  else if (strcmp(argv[0], LIST_HISTORY_COMMAND) == 0)
  {
    HIST_ENTRY **fullHistory = history_list();
    for (int i = 0; NULL != fullHistory && NULL != fullHistory[i]; i++)
    {
      printf("%s\n", fullHistory[i]->line);
    }
    return true;
  }
  return false;
}

void sh_init(struct shell *sh)
{
  sh->prompt = get_prompt(PROMPT_ENV_VARIABLE);

  /* See if we are running interactively.  */
  sh->shell_terminal = STDIN_FILENO;
  sh->shell_is_interactive = isatty(sh->shell_terminal);

  if (sh->shell_is_interactive)
  {
    /* Loop until we are in the foreground.  */
    while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
      kill(-sh->shell_pgid, SIGTTIN);

    /* Ignore interactive and job-control signals.  */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    /* Put ourselves in our own process group.  */
    sh->shell_pgid = getpid();
    if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0)
    {
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    /* Grab control of the terminal.  */
    tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

    /* Save default terminal attributes for shell.  */
    tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
  }
}

void sh_destroy(struct shell *sh)
{
  free(sh->prompt);
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTIN, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);
}

void parse_args(int argc, char **argv)
{
  int opt = -1;

  while ((opt = getopt(argc, argv, "v")) != -1)
  {
    switch (opt)
    {
    case 'v':
      printf("Simple Shell version %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
      exit(0);
      break;
    }
  }
}

void execute_command(char **cmd, struct shell sh, bool background)
{
  // This is the child process
  pid_t child = getpid();
  setpgid(child, child);
  if (!background)
  {
    tcsetpgrp(sh.shell_terminal, child);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
  }
  execvp(cmd[0], cmd);
}

struct jobs *evaluate_background_jobs(struct jobs *listHead, bool printAll)
{
  struct jobs *retVal = listHead;
  struct jobs *current = listHead;
  struct jobs *previous = NULL;
  while (NULL != current)
  {
    if (-1 == waitpid(current->processID, NULL, WNOHANG))
    {
      // print done message
      printf("[%d] done", current->jobNumber);
      for (int i = 0; NULL != current->jobCmd[i]; i++)
      {
        printf(" %s", current->jobCmd[i]);
      }
      printf(" %c\n", BACKGROUND_CHAR);

      // remove element from list and clean up memory
      if (NULL == previous)
      { // retVal should be the same as current if this is true
        retVal = retVal->next;
        current->next = NULL;
        destroy_background_list(current);

        current = retVal;
      }
      else
      {
        previous->next = current->next;

        current->next = NULL;
        destroy_background_list(current);

        current = previous->next;
      }
    }
    else
    {
      if (printAll)
      {
        printf("[%d] %d running", current->jobNumber, current->processID);
        for (int i = 0; NULL != current->jobCmd[i]; i++)
        {
          printf(" %s", current->jobCmd[i]);
        }
        printf(" %c\n", BACKGROUND_CHAR);
      }
      previous = current;
      current = current->next;
    }
  }
  return retVal;
}

struct jobs *add_to_background_list(struct jobs *listHead, char **jobsCmd, int processID)
{
  struct jobs *retVal = listHead;
  struct jobs *current = listHead;
  if (NULL == retVal)
  {
    retVal = malloc(sizeof(struct jobs));
    retVal->jobCmd = jobsCmd;
    retVal->processID = processID;
    retVal->next = NULL;
    retVal->jobNumber = 1; // start job numbers at 1
  }
  else
  {
    while (NULL != current->next)
    {
      current = current->next;
    }
    current->next = malloc(sizeof(struct jobs));
    current->next->jobNumber = current->jobNumber + 1;
    current = current->next;
    current->jobCmd = jobsCmd;
    current->processID = processID;
    current->next = NULL;
  }
  return retVal;
}

void destroy_background_list(struct jobs *listHead)
{
  struct jobs *currentNode = listHead;
  struct jobs *nextNode;

  while (NULL != currentNode)
  {
    nextNode = currentNode->next;
    cmd_free(currentNode->jobCmd);
    free(currentNode);
    currentNode = nextNode;
  }
}