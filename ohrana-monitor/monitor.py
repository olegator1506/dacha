import paho.mqtt.client as mqtt
import json
import time
import os
import sys
import logging
import argparse
from dotenv import load_dotenv



# Переменная для хранения предыдущего значения
previous_value = None

def on_connect(client, userdata, flags, rc):
    """Callback при подключении к MQTT брокеру"""
    if rc == 0:
        print(f"Успешно подключились к MQTT брокеру")
        client.subscribe(MQTT_TOPIC)
        print(f"Подписались на топик: {MQTT_TOPIC}")
    else:
        print(f"Ошибка подключения, код: {rc}")
def on_message(client, userdata, msg):
    """Callback при получении сообщения"""
    global previous_value
    
    try:
        # Пытаемся распарсить JSON
        payload = json.loads(msg.payload.decode())
        current_value = payload
    except:
        # Если не JSON, работаем с текстом
        current_value = msg.payload.decode()
    
    print(f"Получено сообщение из топика {msg.topic}: {current_value}")
    
    # Проверяем изменение значения
    if previous_value is not None and current_value != previous_value:
        print(f"⚠️  Значение изменилось! Было: {previous_value}, Стало: {current_value}")
        # Здесь ваша логика реакции на изменение
        handle_value_change(previous_value, current_value)
    
    previous_value = current_value

def handle_value_change(old_value, new_value):
    """Функция обработки изменения значения"""
    print(f"Обрабатываем изменение: {old_value} -> {new_value}")
    
    # Пример действий при изменении значения:
    if isinstance(new_value, (int, float)) and new_value > 30:
        print("🔴 Температура превысила 30°C!")
    elif new_value == "alert":
        print("🚨 Получен сигнал тревоги!")
    
    # Добавьте свою логику здесь

def main():
    #    global MQTT_BROKER, MQTT_PORT,MQTT_PASSWORD, MQTT_USERNAME, MQTT_TOPIC
    dotenv_path = os.path.join(os.path.dirname(__file__), '.env')
    if os.path.exists(dotenv_path):
        load_dotenv(dotenv_path)
    MQTT_BROKER = os.environ.get('MQTT_SERVER')  #  IP адрес сервера
    MQTT_PORT = os.environ.get('MQTT_PORT')
    MQTT_TOPIC = os.environ.get('MQTT_TOPIC')  # топик для отслеживания
    MQTT_USERNAME = os.environ.get('MQTT_USERNAME')  # если требуется аутентификация
    MQTT_PASSWORD = os.environ.get('MQTT_PASSWORD')

    # Создаем клиент MQTT
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    
    # Настраиваем аутентификацию если требуется
    if MQTT_USERNAME and MQTT_PASSWORD:
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    # Назначаем callback функции
    client.on_connect = on_connect
    client.on_message = on_message
    # client.on_subscribe = on_subscribe
    
    try:
        # Подключаемся к брокеру
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        
        # Запускаем бесконечный цикл обработки сообщений
        print("Запускаем отслеживание MQTT топика...")
        client.loop_forever()
        
    except KeyboardInterrupt:
        print("\nОстанавливаем программу...")
        client.disconnect()
    except Exception as e:
        print(f"Ошибка: {e}")



if __name__ == "__main__":
    main()