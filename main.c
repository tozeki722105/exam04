#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
	TRUE = 1,
	FALSE = 0
} t_bool;

int get_delim_off(char **ptr, t_bool pipe_flag)
{
	int i;

	i = 0;
	while (ptr[i]) {
		if (ptr[i][0] == ';')
			return (i);
		if (pipe_flag && ptr[i][0] == '|')
			return (i);
		i++;
	}
	return (i);
}

int exec_cmd(char **ptr, int delim_off, char **envp)
{
	static int tmpfd_in = STDIN_FILENO;
	int        pipefd[2];

	pipe(pipefd);
	pid_t pid = fork();
	if (pid == 0) {
		close(pipefd[0]);
		if (tmpfd_in != STDIN_FILENO)
			dup2(tmpfd_in, STDIN_FILENO);
		if (ptr[delim_off] && ptr[delim_off][0] == '|')
			dup2(pipefd[1], STDOUT_FILENO);
		ptr[delim_off] = NULL;
		execve(*ptr, ptr, envp);
		exit(0);
	}
	close(pipefd[1]);
	if (tmpfd_in != STDIN_FILENO)
		close(tmpfd_in);
	tmpfd_in = pipefd[0];
	if (!ptr[delim_off] || ptr[delim_off][0] == ';') {
		if (tmpfd_in != STDIN_FILENO)
			close(tmpfd_in);
		tmpfd_in = STDIN_FILENO;
	}
	return (1);
}

int do_cd(char **ptr, int line_end_off)
{
	// examでバグる
	if (line_end_off != 2) {
		printf("arg err\n");
		exit(1);
	}
	if (chdir(ptr[line_end_off - 1]) == -1) {
		printf("err path\n");
		exit(1);
	}
	return (0);
}

int exec_line(char **ptr, int line_end_off, char **envp)
{
	int recoverfd_in;
	int delim_off;
	int proc_count;

	proc_count = 0;
	if (strcmp(*ptr, "cd") == 0)
		return (do_cd(ptr, line_end_off));
	recoverfd_in = dup(STDIN_FILENO);
	while (1) {
		delim_off = get_delim_off(ptr, TRUE);
		proc_count += exec_cmd(ptr, delim_off, envp);
		if (!ptr[delim_off] || ptr[delim_off][0] == ';')
			break;
		ptr = &ptr[delim_off + 1];
	}
	dup2(recoverfd_in, STDIN_FILENO);
	close(recoverfd_in);
	return (proc_count);
}

void wait_proc(int proc_count)
{
	int i;

	i = 0;
	while (i < proc_count) {
		waitpid(0, NULL, 0);
		i++;
	}
}

int main(int argc, char **argv, char **envp)
{
	int line_end_off;
	int proc_count = 0;
	;
	char **ptr;

	if (argc <= 1)
		return (0);
	ptr = ++argv;
	while (1) {
		line_end_off = get_delim_off(ptr, FALSE);
		proc_count += exec_line(ptr, line_end_off, envp);
		if (!ptr[line_end_off])
			break;
		ptr = &ptr[line_end_off + 1];
	}
	wait_proc(proc_count);
}