#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char* read_filename() { //функция считывания имени файла
  char* line = NULL;
  size_t bufsize = 0;
  // Memory:
  // - 100 (line): 0
  // - 200 (bufsize): 0

  int nread = getline(&line, &bufsize, stdin); // getline(100, 200, stdin)
  if (nread == -1) {
    return NULL; 
  }

  // Memory:
  // - 100 (line): 456
  // - ...
  // - 456: 'r'
  // - 457: 'e'
  // - 458: 's'
  // - 459: '\n'
  // - 460: '\0'

  size_t len = strlen(line); //количество символов строки(т е имя файла) до '\0'
  // printf("len=%d\n", len); 
  line[len - 1] = '\0';

  // Memory:
  // - 100 (line): 456
  // - ...
  // - 456: 'r'
  // - 457: 'e'
  // - 458: 's'
  // - 459: '\0'
  // - 460: '\0'
  return line; // 456
}

char write_number(int fd, float number) { //
  int n_written_bytes = write(fd, &number, sizeof(number));
  if (n_written_bytes <= 0) {
    printf("write error\n");
    return 0;
  }
  assert(n_written_bytes == sizeof(number));
  return 1;
}

int main(int argc, char *argv[]) { //int main(сколько аргументов, имя файла), имя файла - тоже аргумент
  if (argc != 1) { // arg count, если количество аргументов не равно 1, выводим ошибку
    printf("Usage: %s\n", argv[0]);
    return 1;
  }

  char* filename_result = read_filename(); //считали имя файла
  if (filename_result == NULL) { //если его нет, выводим ошибку и сообщение об ожидании имени файла
    printf("expected filename\n");
    return 1;
  }

  //открываем два конца трубы
  int pipe_fds[2]; 
  if (pipe(pipe_fds) == -1) { //проверяем трубу на ошибки(EMFILE, EFAULT), если есть, выводим 
    printf("pipe error\n");
    return 1;
  }
  // pipe_fds = {10, 20}
  // write(20, "abc") -> (kernel buffer) -> read(10) = "abc"

  //создаём дочерний процесс
  int pid_child = fork();
  if (pid_child == -1) { //если не вышло создать, выходит с ошибкой
    printf("fork error\n");
    return 1;
  }
  // pid_child=0 -- у дочернего процесса
  // pid_child=номер_дочернего -- у родительского процесса

  if (pid_child == 0) {
    // Мы в дочернем процессе.
    if (close(pipe_fds[1]) != 0) {
      printf("close error\n");
      return 1;
    }
    // Закрыли тот конец трубы, что на запись
    // Теперь из трубы можем только читать
    // Говорим, что дальше при обращении к stdin нужно в действительности обращаться к тому концу трубы, что на чтение
    if (dup2(pipe_fds[0], 0) == -1) {
      printf("dup2 error\n");
      return 1;
    }
  
    //вызываем дочерний процесс
    char* argv_child[3] = {"./child", filename_result, (char *)NULL};
    //предполагаем, что дочерний процесс находится в текущем каталоге
    //как аргумент мы передаём имя файла, тк дочерняя программа должна быть отдельной программой
    if (execv("child", argv_child) == -1) { 
      printf("exec error\n");
      return 1;
    }

    return 0; // Если вдруг execv вернёт не -1 (такого не должно быть)
  }

  assert(pid_child > 0); //условие pid_child == 0 не сработало, мы в родительском процессе

  if (close(pipe_fds[0]) != 0) {
    printf("close error\n");
    return 1;
  }
  // Закрыли тот конец трубы, что на чтение.

  while (1) { //читаем со входа числа
    // 10 20\n
    //   ^
    //   sep=' '
    //      ^
    //      sep='\n'
    //

    // Считываем очередное число из входного файла и преобразуем его из
    // текста во float.
    float number;
    int result_scanf = scanf("%f", &number); //считали число
    if (result_scanf == EOF) { // -1, если встретили конец файла, вышли
      break;
    }
    if (result_scanf == 0) { // 0, если встретили ерунду
      printf("expected a number\n");
      return 1;
    }
    assert(result_scanf == 1); //благополучно считали число

    if (! write_number(pipe_fds[1], number)) { //отправляем число нашему дочернему процессу
      return 1;
    }

    //мы пытаемся понять, кончились цифры в строке
    char sep = getchar();
    if (sep == '\n') { //если разделитель перенос строки, 
      // записываем в дочерний процесс, что данные кончились через бесконечность
      if (! write_number(pipe_fds[1], 1.0 / 0.0)) { //если не удалость напечатать
        return 1;
      }
    }
  }
  //вышли из цикла

  if (close(pipe_fds[1]) != 0) {
    printf("close error\n");
    return 1;
  }
  // Закрыли тот конец трубы, что на запись
  // Даём понять дочернему процессу, что данных больше не будет

  //надо дождаться конца работы дочернего процесса
  int wstatus; 
  if (wait(&wstatus) == -1) { //вернуть -1 в случае ошибки
    printf("wait error\n");
    return 1;
  }
  // Ребёнок завершился.

  if (! (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0)) { //Мы не можем вернуть 0, если с ребёнком что-то не так
    printf("error in child\n");
    return 1;
  }

  return 0;
}
/*
user -> main (stdin): result.txt
main: creates child, passes "result.txt"

user -> main (stdin): 1 2 3
main -> child (pipe1): 1 2 3
child -> result.txt: 1+2+3=6

user -> main (stdin): 1 2
main -> child (pipe1): 1 2
child -> result.txt: 1+2=3
 */
