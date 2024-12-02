#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define FATAL_ERR "error: fatal\n"

typedef enum {
	TRUE = 1,
	FALSE = 0
} t_bool;

void put_err(char *str)
{
	while (*str)
		write(STDERR_FILENO, str++, 1);
}

int get_delim_off(char **ptr, t_bool pipe_flag)
{
	int i = 0;
	while (ptr[i]) {
		if (ptr[i][0] == ';')
			return (i);
		if (pipe_flag && ptr[i][0] == '|')
			return (i);
		i++;
	}
	return (i);
}

void x_pipe(int *pipefd)
{
	if (pipe(pipefd) == -1) {
		put_err(FATAL_ERR);
		exit(1);
	};
}

pid_t x_fork(void)
{
	pid_t ret = fork();
	if (ret == -1) {
		put_err(FATAL_ERR);
		exit(1);
	}
	return (ret);
}

void x_dup2(int srcfd, int destfd)
{
	if (dup2(srcfd, destfd) == -1) {
		put_err(FATAL_ERR);
		exit(1);
	}
}

// int x_dup(int cpyfd)
// {
// 	int ret = dup(cpyfd);
// 	if (ret == -1) {
// 		put_err(FATAL_ERR);
// 		exit(1);
// 	}
// 	return (ret);
// }

void x_execve(char *path, char **arg, char **envp)
{
	if (execve(path, arg, envp) == -1) {
		put_err("error: cannot execute ");
		put_err(path);
		put_err("\n");
		exit(1);
	}
}

void wait_proc(int proc_count)
{
	int i = 0;
	while (i++ < proc_count) {
		if (waitpid(0, NULL, 0) == -1) {
			put_err(FATAL_ERR);
			exit(1);
		}
	}
}

int exec_cmd(char **ptr, int delim_off, char **envp)
{
	static int tmpfd_in = STDIN_FILENO;
	int pipefd[2];

	x_pipe(pipefd);
	pid_t pid = x_fork();
	if (pid == 0) {
		close(pipefd[0]);
		x_dup2(tmpfd_in, STDIN_FILENO);
		if (ptr[delim_off] && strcmp(ptr[delim_off], "|") == 0)
			x_dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);  // add
		ptr[delim_off] = NULL;
		x_execve(*ptr, ptr, envp);
		exit(0);
	}
	close(pipefd[1]);
	if (tmpfd_in != STDIN_FILENO)
		close(tmpfd_in);
	tmpfd_in = pipefd[0];
	if (!ptr[delim_off] || strcmp(ptr[delim_off], ";") == 0) {
		close(tmpfd_in);
		tmpfd_in = STDIN_FILENO;
	}
	return (1);
}

void do_cd(char **ptr, int line_end_off)
{
	if (line_end_off != 2) {
		put_err("error: cd: bad arguments\n");
		return;
	}
	if (chdir(ptr[line_end_off - 1]) == -1) {
		put_err("error: cd: cannot change directory to ");
		put_err(ptr[line_end_off - 1]);
		put_err("\n");
	}
	return;
}

void exec_line(char **ptr, int line_end_off, char **envp)
{
	if (strcmp(*ptr, "cd") == 0)
		return (do_cd(ptr, line_end_off));

	int proc_count = 0;
	// int recoverfd_in = x_dup(STDIN_FILENO);
	while (1) {
		int delim_off = get_delim_off(ptr, TRUE);
		proc_count += exec_cmd(ptr, delim_off, envp);
		if (!ptr[delim_off] || strcmp(ptr[delim_off], ";") == 0)
			break;
		ptr = &ptr[delim_off + 1];
	}
	wait_proc(proc_count);
	// x_dup2(recoverfd_in, STDIN_FILENO);
	// close(recoverfd_in);
}

int main(int argc, char **argv, char **envp)
{
	if (argc <= 1)
		return (0);
	char **ptr = ++argv;
	while (1) {
		int line_end_off = get_delim_off(ptr, FALSE);
		exec_line(ptr, line_end_off, envp);
		if (!ptr[line_end_off])
			break;
		ptr = &ptr[line_end_off + 1];
	}
}