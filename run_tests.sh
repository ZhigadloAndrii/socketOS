#!/bin/bash

# Назва файлу для збереження результатів
RESULTS_FILE="results.txt"

# Очистка файлу результатів перед початком тестів
> $RESULTS_FILE

# Функція для запуску одного тесту
run_test() {
    socket_type=$1
    blocking_type=$2
    sync_type=$3
    packet_size=$4
    packet_count=$5

    echo "Запуск тесту: $socket_type $blocking_type $sync_type $packet_size $packet_count" | tee -a $RESULTS_FILE

    # Запуск сервера
    ./server $socket_type $blocking_type $sync_type &
    server_pid=$!

    # Чекаємо, поки сервер запуститься
    sleep 1

    # Запуск клієнта
    ./client $socket_type $blocking_type $packet_size $packet_count | tee -a $RESULTS_FILE

    # Зупинка сервера
    kill $server_pid

    echo "---------------------" | tee -a $RESULTS_FILE
}

# Масиви для різних параметрів тестів
socket_types=("INET" "UNIX")
blocking_types=("blocking" "non-blocking")
sync_types=("sync" "async")
packet_sizes=(64 1024 4096)
packet_counts=(100 1000 10000)

# Цикли для перебору всіх комбінацій параметрів
for socket_type in "${socket_types[@]}"; do
    for blocking_type in "${blocking_types[@]}"; do
        for sync_type in "${sync_types[@]}"; do
            for packet_size in "${packet_sizes[@]}"; do
                for packet_count in "${packet_counts[@]}"; do
                    run_test $socket_type $blocking_type $sync_type $packet_size $packet_count
                done
            done
        done
    done
done

echo "Всі тести завершено. Результати збережено в $RESULTS_FILE"
