#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *line = NULL;
    size_t len = 0;

    while (1)
    {
        printf("wish> ");
        fflush(stdout);

        if (getline(&line, &len, stdin) == -1)
        {
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "exit") == 0)
        {
            break;
        }

        printf("You entered: %s\n", line);
    }

    free(line);
    return 0;


}
