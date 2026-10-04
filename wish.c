#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 64
#define MAX_PATHS 64
#define MAX_COMMANDS 64

void print_error(void)
{
    const char message[] = "An error has occurred\n";
    write(STDERR_FILENO, message, sizeof(message) - 1);
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        print_error();
        exit(1);
    }

    FILE *input = stdin;

    if (argc == 2)
    {
        input = fopen(argv[1], "r");

        if (input == NULL)
        {
            print_error();
            exit(1);
        }
    }

    char *line = NULL;
    size_t len = 0;

    char *paths[MAX_PATHS];
    int path_count = 1;

    paths[0] = strdup("/bin");

    while (1)
    {
        if (input == stdin)
        {
            printf("wish> ");
            fflush(stdout);
        }

        if (getline(&line, &len, input) == -1)
        {
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        if (strlen(line) == 0)
        {
            continue;
        }

        char *command_strings[MAX_COMMANDS];
        int command_count = 0;

        char *saveptr_commands;
        char *command = strtok_r(line, "&", &saveptr_commands);

        while (command != NULL && command_count < MAX_COMMANDS)
        {
            command_strings[command_count++] = command;
            command = strtok_r(NULL, "&", &saveptr_commands);
        }

        pid_t pids[MAX_COMMANDS];
        int pid_count = 0;

        for (int command_index = 0;
             command_index < command_count;
             command_index++)
        {
            char *args[MAX_ARGS];
            int argc_cmd = 0;

            char *saveptr_args;
            char *token = strtok_r(command_strings[command_index],
                                    " \t",
                                    &saveptr_args);

            while (token != NULL && argc_cmd < MAX_ARGS - 1)
            {
                args[argc_cmd++] = token;
                token = strtok_r(NULL, " \t", &saveptr_args);
            }

            args[argc_cmd] = NULL;

            if (argc_cmd == 0)
            {
                continue;
            }

            if (command_count == 1 &&
                strcmp(args[0], "exit") == 0)
            {
                if (argc_cmd != 1)
                {
                    print_error();
                    continue;
                }

                for (int i = 0; i < path_count; i++)
                {
                    free(paths[i]);
                }

                free(line);

                if (input != stdin)
                {
                    fclose(input);
                }

                exit(0);
            }

            if (command_count == 1 &&
                strcmp(args[0], "cd") == 0)
            {
                if (argc_cmd != 2 || chdir(args[1]) != 0)
                {
                    print_error();
                }

                continue;
            }

            if (command_count == 1 &&
                strcmp(args[0], "path") == 0)
            {
                for (int i = 0; i < path_count; i++)
                {
                    free(paths[i]);
                }

                path_count = 0;

                for (int i = 1;
                     i < argc_cmd && i < MAX_PATHS;
                     i++)
                {
                    paths[path_count] = strdup(args[i]);

                    if (paths[path_count] == NULL)
                    {
                        print_error();
                        break;
                    }

                    path_count++;
                }

                continue;
            }

            int redirect = -1;

            for (int i = 0; i < argc_cmd; i++)
            {
                if (strcmp(args[i], ">") == 0)
                {
                    if (redirect != -1)
                    {
                        redirect = -2;
                        break;
                    }

                    redirect = i;
                }
            }

            if (redirect == -2 ||
                (redirect != -1 &&
                 (redirect == 0 || redirect != argc_cmd - 2)))
            {
                print_error();
                continue;
            }

            char *output_file = NULL;

            if (redirect != -1)
            {
                output_file = args[redirect + 1];
                args[redirect] = NULL;
            }

            pid_t pid = fork();

            if (pid < 0)
            {
                print_error();
                continue;
            }

            if (pid == 0)
            {
                if (output_file != NULL)
                {
                    int fd = open(output_file,
                                  O_WRONLY | O_CREAT | O_TRUNC,
                                  0666);

                    if (fd < 0)
                    {
                        print_error();
                        exit(1);
                    }

                    if (dup2(fd, STDOUT_FILENO) < 0 ||
                        dup2(fd, STDERR_FILENO) < 0)
                    {
                        close(fd);
                        print_error();
                        exit(1);
                    }

                    close(fd);
                }

                char fullpath[512];
                int found = 0;

                for (int i = 0; i < path_count; i++)
                {
                    snprintf(fullpath,
                             sizeof(fullpath),
                             "%s/%s",
                             paths[i],
                             args[0]);

                    if (access(fullpath, X_OK) == 0)
                    {
                        execv(fullpath, args);
                        found = 1;
                        break;
                    }
                }

                if (!found)
                {
                    print_error();
                }

                exit(1);
            }

            pids[pid_count++] = pid;
        }

        for (int i = 0; i < pid_count; i++)
        {
            waitpid(pids[i], NULL, 0);
        }
    }

    for (int i = 0; i < path_count; i++)
    {
        free(paths[i]);
    }

    free(line);

    if (input != stdin)
    {
        fclose(input);
    }

    return 0;
}
