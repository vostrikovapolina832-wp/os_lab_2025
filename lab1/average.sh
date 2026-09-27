#!/bin/bash
# Без аргументов среднее не определено: делить на ноль нельзя.
if (( $# == 0 )); then
printf 'Count: 0\n'
printf 'Error: pass at least one number.\n' >&2
exit 1
fi
# Проверяем каждый отдельный аргумент до начала вычислений.
for value in "$@"; do
# Разрешаем целые и десятичные числа: 7, -2, +3, 1.5.
if [[ ! $value =~ ^[+-]?[0-9]+([.][0-9]+)?$ ]]; then
printf 'Error: not a number: %s\n' "$value" >&2
exit 1
fi
done
# awk умеет делить с дробной частью; LC_ALL=C задает точку.
# -- завершает список опций, в том числе перед числом -2.
LC_ALL=C awk -- '
BEGIN {
count = ARGC - 1; # ARGV[0] - имя awk.
sum = 0; # Сначала сумма равна нулю.
for (i = 1; i < ARGC; i++) { # Перебираем аргументы.
sum += ARGV[i]; # Прибавляем очередное число.
}
printf "Count: %d\n", count;
printf "Average: %.6f\n", sum / count;
exit; # Не читаем stdin или файлы.
}' "$@"
