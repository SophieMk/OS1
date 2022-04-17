#include <assert.h>
#include <float.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc,char* argv[]){
    printf("child: started\n");

    FILE *file = fopen(argv[1], "w"); //открываем файл
    if (file == NULL){
        printf("fopen error\n");
        return 1;
    }

    float sum = 0;
    while (1) { 
        // File descriptors:
        // 0 -- stdin
        // 1 -- stdout
        // 2 -- stderr
        float number;
        int n_read = read(0, &number, sizeof(number)); //читаем со стандартного потока ввода (нам переброшен один конец трубы)
        if (n_read == -1) {
            printf("read error\n");
            return 1;
        }
        if (n_read == 0) { // конец файла
            // Другой конец трубы закрыт в родительском процессе
            break;
        }
        // n_read > 0
        assert(n_read == sizeof(number));
        printf("number=%f\n", number);

        if (number <= FLT_MAX) { //если число не бесконечность, добавляем к сумме число
            sum += number;
            continue;
        }

        // если бесконечность
        printf("sum=%f\n", sum); //выводим сумму
        if (fprintf(file, "%f\n", sum) < 0) {
            printf("fprintf error\n");
            return 1;
        }
        sum = 0;
    }

    if (fclose(file) != 0) { //если fclose работает неверно(ех, место на жёстком диске кончилось)
        printf("fclose error\n"); // ошибка
        return 1;
    }

    return 0;
}
