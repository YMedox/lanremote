#!/bin/bash

#команды
cmd="/home/master/workspace/lanremote/bin/Release/lanremote"
cmd_tv="/home/master/bin/send_ir.sh ON"
cmd_firefox="firefox"
#имена устройств из lanremote.conf
samsung=Samsung
yamaha=Yamaha
#адрес телевизора
IP_TV="192.168.2.203"
#с каким сайтом открывать Firefox
URL="kinopoisk.ru"

if [ "$1" = "ON" ]; then
    echo "Включаем всё"
    # Явно задаём переменные окружения
    export DISPLAY=${DISPLAY:-:0}
    export XAUTHORITY=${XAUTHORITY:-$HOME/.Xauthority}
    if pgrep -x "firefox" > /dev/null; then
      echo "Firefox запущен, пытаемся установить фокус..."
      # Добавляем задержку для синхронизации
      sleep 0.5
      wmctrl -x -a "Navigator.Firefox" 2>/dev/null || echo "wmctrl failed"
      $cmd_firefox --new-tab $URL &
    else
      echo "Firefox не запущен, запускаем..."
      $cmd_firefox $URL &
    fi
    if ping -c 1 "$IP_TV" > /dev/null 2>&1; then
       echo "Телевизор включен"
    else
       echo "Телевизор выключен"
       #Включаем телевизор IR-пультом, потому что по сети телевизор недоступен, когда выключен
       $cmd_tv
    fi
    disown
#exit 0
    #Включаем ресивер
    $cmd $yamaha ON
    sleep 5
    #Устанавливаем вход на ресивере
    $cmd $yamaha HDMI1
    sleep 10	
    #Устанавливаем вход на телевизоре 
    $cmd $samsung HDMI
    sleep 1
    #Устанавливаем режим на ресивере
    $cmd $yamaha DSP_7CH
    #убираем громкость на телевизоре
    for ((i=1; i<=20; i++))
      do
        $cmd $samsung VOLUMEDOWN
    done
    exit 0
elif [ "$1" = "OFF" ]; then
    echo "Выключаем всё"
    #Выключаем телевизор м ресивер LAN-пультом
    $cmd $samsung OFF $yamaha OFF
    exit 0
else
    echo "Ошибка: ожидаемый параметр — ON или OFF"
    exit 1
fi

