#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#define MAX_CMD_L 1024
#define MAX_ARGS 1024
#define MAX_H 1024

#define ERROR(msg) do { printf("%s\n", msg); } while(0)

// Struct to store each history entry
typedef struct{
    char cmd[MAX_CMD_L];
    pid_t pid;
    time_t strt_t;
    double dur;
}his_entry;

his_entry his[MAX_H];
int his_c = 0;

void store_h(char *cmd, pid_t pid, time_t strt_t, double dur){
    if (his_c < MAX_H){
        strncpy(his[his_c].cmd, cmd, MAX_CMD_L - 1);
        his[his_c].pid = pid, 
        his[his_c].strt_t = strt_t, 
        his[his_c].dur = dur, 
        his_c++;
    }
    else{
        for (int i = 1; i < MAX_H; i++){
            his[i - 1] = his[i];
        }
        strncpy(his[MAX_H - 1].cmd, cmd, MAX_CMD_L - 1);
        his[MAX_H - 1].dur = dur;
        his[MAX_H - 1].pid = pid;
        his[MAX_H - 1].strt_t = strt_t;
    }
}
// Func. to Display history
void print_h(){
    for (int i = 0; i < his_c; i++){
        printf("%d. %s [PID: %d] [Start: %s] [Duration: %.2lf seconds]\n",
               i + 1,
               his[i].cmd,
               his[i].pid,
               ctime(&(his[i].strt_t)),
               his[i].dur);
    }
}
// Function to handle SIGINT (Ctrl+C)
void ctrl_c(int sig_num){
    printf("\nCaught Ctrl+C (SIGINT). Use 'y' to quit.\n");
    fflush(stdout);
}

// Function to execute a single command
void exec_cmd(char *cmd, int fd_i, int fd_o){
    char *args[MAX_ARGS];
    int x = 0;
    char *t = strtok(cmd, " ");
    while (t != NULL){
        args[x++] = t;
        t = strtok(NULL, " ");
    }
    args[x] = NULL;

    if (fd_i != 0){
        dup2(fd_i, STDIN_FILENO);
        close(fd_i);
    }
    if (fd_o != STDOUT_FILENO){
        dup2(fd_o, STDOUT_FILENO);
        close(fd_o);
    }
    if (execvp(args[0], args) == -1){
        ERROR("failed in Executing");
        exit(EXIT_FAILURE);
    }
}
// Function to launch cmd with piping
int run_cmd(char *cmd){
    char *org_cmd = strdup(cmd);
    if (!org_cmd){
        ERROR("Failed to allocate memory for command copy");
        return -1;
    }
    int back = 0;
    // Check if the cmd ends with '&' for background execution
    if (cmd[strlen(cmd) - 1] == '&') {
        back = 1;
        cmd[strlen(cmd) - 1] = '\0'; // Remove '&' from command
    }

    // Check if command is "history"
    if (strcmp(cmd, "his") == 0){
        print_h();
        free(org_cmd);
        return 1;
    }

    // Tokenize by pipes
    char *cmds[MAX_ARGS];
    int n_cmd = 0;
    char *t = strtok(cmd, "|");
    while (t != NULL){
        cmds[n_cmd++] = t;
        t = strtok(NULL, "|");
    }

    int fd[2], fd_i = 0;
    pid_t pid;
    for (int i = 0; i < n_cmd; i++){
        pipe(fd);
        pid = fork();
        if (pid == 0){  // Child process
            close(fd[0]);
            exec_cmd(cmds[i], fd_i, i == n_cmd - 1 ? STDOUT_FILENO : fd[1]);
        }
        else if (pid < 0){
            ERROR("Fork failed");
            free(org_cmd);
            return -1;
        }
        else{  
            // Parent process
            close(fd[1]);
            if (fd_i != 0) {
                close(fd_i);
            }
            fd_i = fd[0];
        }
    }

    if (!back){
        // Wait for the last child and record his for foreground process
        time_t strt_t = time(NULL);
        int sts;
        waitpid(pid, &sts, 0);
        double dur = difftime(time(NULL), strt_t);
       
        // Store the original command in his
        store_h(org_cmd, pid, strt_t, dur);
    }
    else{
        // For background processes, don't wait
        printf("[Running in background, PID: %d]\n", pid);
        time_t strt_t = time(NULL);
        store_h(org_cmd, pid, strt_t, 0);
    }

    // Free the original command copy
    free(org_cmd);
    return 1;
}
void sigchld(int sig){
    // Wait for any child process without blocking
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
// Function to read a command from the user
int read_cmd(char *cmd){
    printf("One-Piece Shell $$ >>");
    if (fgets(cmd, MAX_CMD_L, stdin) == NULL) {
        return -1; // Error or EOF
    }
    // Remove the newline character from the input
    cmd[strcspn(cmd, "\n")] = '\0';
    return 0;
}
// Main loop of the shell
int main(){
    char cmd[MAX_CMD_L];
    signal(SIGCHLD, sigchld); 
    signal(SIGINT, ctrl_c);
    while (1){
        if (read_cmd(cmd) == -1){
            break;
        }
        // Exit on "y" command
        if (strcmp(cmd, "y") == 0){
            break;
        }
        run_cmd(cmd);
    }
    return 0;
}
