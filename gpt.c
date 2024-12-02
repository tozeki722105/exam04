#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERROR_FATAL "error: fatal\n"
#define ERROR_CD_ARGS "error: cd: bad arguments\n"
#define ERROR_CD_PATH "error: cd: cannot change directory to "
#define ERROR_EXEC "error: cannot execute "

// エラーを出力してプログラムを終了する関数
void fatal_error()
{
	write(2, ERROR_FATAL, strlen(ERROR_FATAL));
	exit(1);
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

int main(int argc, char **argv, char **envp)
{
	if (argc < 2)
		return 0;

	int i = 1;
	int fd[2];
	int tmp_fd = dup(0);  // 標準入力をバックアップ
	if (tmp_fd < 0)
		fatal_error();

	while (argv[i]) {
		char *cmd[1024];
		int cmd_idx = 0;
		int is_pipe = 0;

		// セミコロンやパイプごとにコマンドを分割
		while (argv[i] && strcmp(argv[i], "|") != 0 && strcmp(argv[i], ";") != 0) {
			cmd[cmd_idx++] = argv[i++];
		}
		cmd[cmd_idx] = NULL;

		if (cmd_idx == 0) {  // 最初のコマンドが(|),(;)の時のハンドリング
			i++;
			continue;
		}

		if (argv[i] && strcmp(argv[i], "|") == 0)
			is_pipe = 1;

		if (strcmp(cmd[0], "cd") == 0) {
			exec_cd(cmd);
		} else {
			if (is_pipe && pipe(fd) < 0)  // pipeがある時しかpipe()を実行しない
				fatal_error();

			pid_t pid = fork();
			if (pid < 0)
				fatal_error();

			if (pid == 0) {
				if (is_pipe && dup2(fd[1], 1) < 0)
					fatal_error();
				if (dup2(tmp_fd, 0) < 0)
					fatal_error();
				if (is_pipe) {  // pipefdのハンドリングをちゃんとやっている
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
					tmp_fd = dup(0);
					if (tmp_fd < 0)
						fatal_error();
				}
			}
		}

		if (argv[i])
			i++;
	}

	close(tmp_fd);
	return 0;
}
