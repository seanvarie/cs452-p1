#ifndef LAB_H
#define LAB_H
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define PROMPT_ENV_VARIABLE "MY_PROMPT"
#define HOME_ENV_VARIABLE "HOME"
#define EXIT_COMMAND "exit"
#define CHANGE_DIRECTORY_COMMAND "cd"
#define LIST_JOBS_COMMAND "jobs"
#define LIST_HISTORY_COMMAND "history"//TODO: check what this should be
#define BACKGROUND_CHAR '&'

#define lab_VERSION_MAJOR 1
#define lab_VERSION_MINOR 0
#define UNUSED(x) (void)x;

#ifdef __cplusplus
extern "C"
{
#endif

  struct jobs
  {
    int jobNumber;
    struct jobs * next;
    char ** jobCmd;
    int processID;
  };

  struct shell
  {
    int shell_is_interactive;
    pid_t shell_pgid;
    struct termios shell_tmodes;
    int shell_terminal;
    char *prompt;
  };

  struct jobs * backgroundJobsList;

  /**
   * @brief Set the shell prompt. This function will attempt to load a prompt
   * from the requested environment variable, if the environment variable is
   * not set a default prompt of "shell>" is returned.  This function calls
   * malloc internally and the caller must free the resulting string.
   *
   * @param env The environment variable
   * @return const char* The prompt
   */
  char *get_prompt(const char *env);

  /**
   * Changes the current working directory of the shell. Uses the linux system
   * call chdir. With no arguments the users home directory is used as the
   * directory to change to.
   *
   * @param dir The command to be run as returned by cmd_parse, dir[0] is 
   * assumed to be "cd" and dir[1] (if not null) is treated as the argument 
   * (the directory to change to) 
   * @return  On success, zero is returned.  On error, -1 is returned, and
   * errno is set to indicate the error.
   */
  int change_dir(char **dir);

  /**
   * @brief Convert line read from the user into to format that will work with
   * execvp. We limit the number of arguments to ARG_MAX loaded from sysconf.
   * This function allocates memory that must be reclaimed with the cmd_free
   * function.
   *
   * @param line The line to process
   *
   * @return The line read in a format suitable for exec
   */
  char **cmd_parse(char const *line);

  /**
   * @brief Free the line that was constructed with parse_cmd
   * If line is null, this will do nothing
   *
   * @param line the line to free
   */
  void cmd_free(char ** line);

  /**
   * @brief Trim the whitespace from the start and end of a string.
   * For example "   ls -a   " becomes "ls -a". This function modifies
   * the argument line so that all printable chars are moved to the
   * front of the string
   *
   * @param line The line to trim
   * @return The new line with no whitespace
   */
  char *trim_white(char *line);


  /**
   * @brief Takes an argument list and checks if the first argument is a
   * built in command such as exit, cd, jobs, etc. If the command is a
   * built in command this function will handle the command and then return
   * true. If the first argument is NOT a built in command this function will
   * return false.
   *
   * @param sh The shell
   * @param argv The command to check
   * @return True if the command was a built in command
   */
  bool do_builtin(struct shell *sh, char **argv);

  /**
   * @brief Initialize the shell for use. Allocate all data structures
   * Grab control of the terminal and put the shell in its own
   * process group. NOTE: This function will block until the shell is
   * in its own program group. Attaching a debugger will always cause
   * this function to fail because the debugger maintains control of
   * the subprocess it is debugging.
   *
   * @param sh
   */
  void sh_init(struct shell *sh);

  /**
   * @brief Destroy shell. Free any allocated memory and resources and exit
   * normally.
   *
   * @param sh
   */
  void sh_destroy(struct shell *sh);

  /**
   * @brief Parse command line args from the user when the shell was launched
   *
   * @param argc Number of args
   * @param argv The arg array
   */
  void parse_args(int argc, char **argv);

  /**
   * @brief executes command in the child process. If background is false, 
   * takes control of the terminal before executing the command.
   * This function should never return. If it does return, the command 
   * failed.
   * 
   * @param cmd the command to be executed in a form to be passed to execvp
   * @param sh the shell running this command
   * @param background whether or not this command is to be run in the background
   */
  void execute_command(char ** cmd, struct shell sh, bool background);

  /**
   * @brief traverses through the provided list and removes any elements 
   * which reference jobs that have finished. When a job is removed, a 
   * message is printed with information about the job. If printAll is 
   * true, a message is printed for all jobs regardless of their status.
   * 
   * @param listHead the head of the list to be evaluated
   * @param printAll whether or not to print a message about all jobs
   * 
   * @return the resulting list, if the first element was not removed from the list, this will be the same as listHead
   */
  struct jobs * evaluate_background_jobs(struct jobs * listHead, bool printAll);

  /**
   * @brief adds the specified job to the background jobs list provided and returns the head of the list
   * this function allocated memory for the new element in the list which must later be freed calling 
   * destroy_background_list on listHead will free this memory
   * 
   * @note the element added to the list will point to the provided cmd, therefor, the memory pointed to 
   * by jobsCmd and the strings of the cmd should remain allocated at least until that element is 
   * freed.
   * 
   * @note this function is considered to "take ownership" of the memory reference by the jobsCmd 
   * parameter and the strings referenced in that memory. What this means is that jobsCmd (and the 
   * strings referenced by it) will be freed when destroy_background_list is called on the created 
   * element or a list containing the created element. This is done using the cmd_free method, which 
   * also implies that jobsCmd should have been returned from cmd_parse
   * 
   * @param listHead pointer to the head of the list of background jobs, if NULL, the list is considered to be empty
   * @param jobsCmd the command that was run for this job
   * @param processID the processID of the process executing this job
   * 
   * @return listHead if it is not NULL, or the new head of the list, containing the new job, if it is
   */
  struct jobs * add_to_background_list(struct jobs * listHead, char ** jobsCmd, int processID);

    /**
   * @brief destroys the job list which has a head pointed to by listHead.
   * frees all memory associated with the list
   * 
   * @param listHead head of the list to be destroyed
   */
  void destroy_background_list(struct jobs * listHead);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
