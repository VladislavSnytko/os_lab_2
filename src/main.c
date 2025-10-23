/*
 * ОС. Лабораторная №2 — многопоточность, синхронизация
 * Вариант 7: два игрока бросают по 2 кости K раз; у кого сумма больше — тот
 * выиграл. Требуется оценить шансы победы экспериментально, запустив N
 * симуляций параллельно, при этом ограничив число одновременно работающих
 * потоков ключом -t (threads).
 */

#include <pthread.h>  // pthread_create, pthread_join, тип pthread_t
#include <stdio.h>    // printf, fprintf, perror
#include <stdlib.h>   // atoi, calloc, free
#include <time.h>     // time (источник seed для ГПСЧ)
#include <unistd.h>  // getpid (показать PID процесса для ps/htop)

typedef struct {
    int K;
    int s1_start;
    int s2_start;
    int count;
    unsigned int seed;
    unsigned long long win1;
    unsigned long long win2;
    unsigned long long draw;
} ThreadArg;

unsigned int xorshift32(unsigned int *state) {
    unsigned int x = *state;
    if (x == 0) x = 1;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

void *worker(void *p) {  // функция, которую исполняет каждый поток
    ThreadArg *a = (ThreadArg *)p;
    unsigned long long w1 = 0, w2 = 0, dr = 0;
    for (int e = 0; e < a->count; ++e) {
        int s1 = a->s1_start;
        int s2 = a->s2_start;
        for (int i = 0; i < a->K; ++i) {
            int d1 = (int)(xorshift32(&a->seed) % 6u) + 1;
            int d2 = (int)(xorshift32(&a->seed) % 6u) + 1;
            int d3 = (int)(xorshift32(&a->seed) % 6u) + 1;
            int d4 = (int)(xorshift32(&a->seed) % 6u) + 1;
            s1 += d1 + d2;
            s2 += d3 + d4;
        }
        if (s1 > s2)
            ++w1;
        else if (s2 > s1)
            ++w2;
        else
            ++dr;
    }
    a->win1 = w1;
    a->win2 = w2;
    a->draw = dr;
    return NULL;  // возвращаем NULL (результат не используется)
}

void usage(const char *prg) {  // печать подсказки по использованию
    fprintf(stderr, "Usage: %s K tour sum1 sum2 experiments -t threads\n", prg);
}

int main(int argc, char **argv) {
    if (argc != 8) {
        usage(argv[0]);
        return 1;
    }
    if (argv[6][0] != '-' || argv[6][1] != 't') {
        fprintf(stderr, "Error: expected '-t' flag before thread count\n");
        usage(argv[0]);
        return 1;
    }

    for (int i = 1; i <= 5; i++) {
        if (argv[i][0] < '0' || argv[i][0] > '9') {
            if (!(i >= 3 && argv[i][0] == '-')) {
                fprintf(stderr,
                        "Error: argument %d ('%s') is not a valid number\n", i,
                        argv[i]);
                usage(argv[0]);
                return 1;
            }
        }
    }
    if (argv[7][0] < '0' || argv[7][0] > '9') {
        fprintf(stderr,
                "Error: threads argument ('%s') is not a valid number\n",
                argv[7]);
        usage(argv[0]);
        return 1;
    }

    int K = atoi(argv[1]);
    int tour = atoi(argv[2]);
    int sum1 = atoi(argv[3]);
    int sum2 = atoi(argv[4]);
    int N = atoi(argv[5]);
    int T = atoi(argv[7]);  // число потоков

    if (K <= 0) {
        fprintf(stderr, "Error: K must be positive (got %d)\n", K);
        return 1;
    }
    if (N <= 0) {
        fprintf(stderr, "Error: experiments must be positive (got %d)\n", N);
        return 1;
    }
    if (T <= 0) {
        fprintf(stderr, "Error: threads must be positive (got %d)\n", T);
        return 1;
    }

    if (T > N) T = N;  // не создаём больше потоков, чем задач

    pthread_t *th = (pthread_t *)calloc(
        (size_t)T, sizeof(*th));  // массив идентификаторов потоков
    ThreadArg *args = (ThreadArg *)calloc(
        (size_t)T, sizeof(*args));  // массив аргументов потоков

    if (!th || !args) {
        perror("calloc");
        return 1;
    }

    int base = N / T;
    int rem = N % T;

    unsigned int seed0 = (unsigned int)time(NULL) ^
                         (unsigned int)(tour * 2654435761u); /* базовый seed */

    printf("PID=%d\n", (int)getpid());
    // 2654435761 и 0x9E3779B9 ≈ 2654435769
    // константа, связанная с золотым сечением;
    // часто используется в «умножающем» хешировании
    // и для «разброса» индексов
    for (int i = 0; i < T; ++i) {
        args[i].K = K;
        args[i].s1_start = sum1;
        args[i].s2_start = sum2;
        args[i].count = base + (i < rem ? 1 : 0);
        args[i].seed = seed0 ^ (unsigned int)(0x9e3779b9u * (i + 1));
        if (args[i].seed == 0) args[i].seed = 1u;
        if (pthread_create(&th[i], NULL, worker, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    unsigned long long W1 = 0, W2 = 0, DR = 0;
    for (int i = 0; i < T; ++i) {
        pthread_join(th[i], NULL);
        // NULL - не забираем результат
        W1 += args[i].win1;
        W2 += args[i].win2;
        DR += args[i].draw;
    }
    free(th);
    free(args);

    printf("K=%d, experiments=%d, threads=%d\n", K, N, T);
    printf("P(win1)=%.6f, P(win2)=%.6f, P(draw)=%.6f\n", (double)W1 / (double)N,
           (double)W2 / (double)N, (double)DR / (double)N);
    return 0;
}
/*
gcc src/main.c -pthread -o src/main -O3 && ./src/main 10 1 0 0 100000 -t 8
top -H -p 2276

или тесты:
./test/analyze_sh.sh ./src/main


Коротко: wall здесь — это “стеночное” время,
то есть фактическое прошедшее время
выполнения программы от старта до финиша по часам.
*/