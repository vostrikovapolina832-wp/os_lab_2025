#!/bin/bash
# Показываем папку, в которой запущен скрипт.
printf 'Current directory:\n'
pwd
# Показываем текущие дату, время и часовой пояс.
printf 'Date and time:\n'
date '+%Y-%m-%d %H:%M:%S %Z'
# %s подставляет значение PATH как одну строку текста.
printf 'PATH:\n%s\n' "$PATH"
