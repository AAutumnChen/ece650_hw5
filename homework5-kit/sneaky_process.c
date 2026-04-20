#include <stdio.h>
#include <unistd.h>

int main(void)
{
  int ch;

  printf("sneaky_process pid = %d\n", getpid());

  while ((ch = getchar()) != EOF) {
    if (ch == 'q') {
      break;
    }
  }

  return 0;
}
