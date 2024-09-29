Цей проект містить інструменти для тестування продуктивності різних конфігурацій сокетів у Linux.

Компоненти:
server.c - Сервер для обробки з'єднань
client.c - Клієнт для відправки пакетів
run_tests.sh - Bash-скрипт для автоматизації тестів

Вимоги:
GCC компілятор
Linux операційна система

Компіляція
Щоб скомпілювати сервер та клієнт, виконайте наступні команди:

gcc -o server server.c -pthread
gcc -o client client.c

Використання
Сервер

./server <тип сокету: INET/UNIX> <blocking/non-blocking> <sync/async>

Наприклад:

./server INET non-blocking async

Клієнт

./client <тип сокету: INET/UNIX> <blocking/non-blocking> <розмір пакету> <кількість пакетів>

Наприклад:

./client INET non-blocking 1024 1000

Автоматизоване тестування
Для запуску всіх тестів, виконайте bash-скрипт:

chmod +x run_tests.sh
./run_tests.sh

Цей скрипт перебере всі можливі комбінації параметрів і збереже результати в файл results.txt.
Параметри тестування

Типи сокетів: INET, UNIX
Режими блокування: blocking, non-blocking
Режими синхронізації: sync, async
Розміри пакетів: 64, 1024, 4096 байт
Кількість пакетів: 100, 1000, 10000

Результати
Результати тестів будуть збережені у файлі results.txt. Кожен тест включатиме наступну інформацію:

Час створення сокета
Час встановлення з'єднання
Час передачі даних
Час закриття сокета
Загальний час роботи
Швидкість передачі (пакетів/сек та байт/сек)
