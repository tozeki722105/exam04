#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define FATAL_ERR "error: fatal\n"
#define ERROR_CD_ARGS "error: cd: bad arguments\n"
#define ERROR_CD_PATH "error: cd: cannot change directory to "
#define ERROR_EXEC "error: cannot execute "

// エラーを出力してプログラムを終了する関数
void fatal_error()
{
	write(2, FATAL_ERR, strlen(FATAL_ERR));
	exit(1);
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

int x_dup(int cpyfd)
{
	int ret = dup(cpyfd);
	if (ret == -1) {
		put_err(FATAL_ERR);
		exit(1);
	}
	return (ret);
}

// コマンドを実行する関数
void exec_command(char **args, char **envp)
{
	if (execve(args[0], args, envp) == -1) {
		write(2, ERROR_EXEC, strlen(ERROR_EXEC));
		write(2, args[0], strlen(args[0]));
		write(2, "\n", 1);
		exit(1);
	}
}

// "cd" コマンドの処理
void exec_cd(char **args)
{
	if (!args[1] || args[2]) {
		write(2, ERROR_CD_ARGS, strlen(ERROR_CD_ARGS));
	} else if (chdir(args[1]) != 0) {
		write(2, ERROR_CD_PATH, strlen(ERROR_CD_PATH));
		write(2, args[1], strlen(args[1]));
		write(2, "\n", 1);
	}
}

exec_line(char *cmd[], char **envp, int is_pipe, int fd[], int tmp_fd)
{
	if (strcmp(cmd[0], "cd") == 0) {
		exec_cd(cmd);
	} else {
		if (is_pipe)  // pipeがある時しかpipe()を実行しない
			x_pipe(fd);
		pid_t pid = x_fork();
		if (pid == 0) {
			x_dup2(tmp_fd, STDIN_FILENO);
			if (is_pipe) {  // pipefdのハンドリングをちゃんとやっている
				x_dup2(fd[1], STDOUT_FILENO);
				close(fd[0]);
				close(fd[1]);
			}
			exec_command(cmd, envp);
		} else {
			if (waitpid(pid, NULL, 0) < 0)  // pipeを生成したら、都度pid指定でwaitしている
				fatal_error();
			if (is_pipe) {
				close(fd[1]);
				tmp_fd = fd[0];
			} else {
				close(tmp_fd);
				tmp_fd = x_dup(0);
			}
		}
	}
}

int main(int argc, char **argv, char **envp)
{
	if (argc < 2)
		return 0;

	int i = 1;
	int fd[2];
	int tmp_fd = x_dup(0);  // 標準入力をバックアップ
	while (argv[i]) {
		char *cmd[1024];
		int cmd_idx = 0;

		// セミコロンやパイプごとにコマンドを分割
		while (argv[i] && strcmp(argv[i], "|") != 0 && strcmp(argv[i], ";") != 0) {
			cmd[cmd_idx++] = argv[i++];
		}
		cmd[cmd_idx] = NULL;

		if (cmd_idx == 0) {  // 最初のコマンドが(|),(;)の時のハンドリング
			i++;
			continue;
		}
		int is_pipe = (argv[i] && strcmp(argv[i], "|") == 0);
		exec_line(cmd, envp, is_pipe, fd, tmp_fd);
		if (argv[i])
			i++;
	}
	close(tmp_fd);
}
