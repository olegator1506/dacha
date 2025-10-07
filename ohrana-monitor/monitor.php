<?php
require_once __DIR__ . '/vendor/autoload.php';

use Bluerhinos\phpMQTT;


class MQTTWatcher {
    private $mqtt;
    private $previousValue = null;
    
    // Настройки MQTT
    private $broker = 'localhost';
    private $port = 1883;
    private $topic = 'sensors/temperature';
    private $username = null;
    private $password = null;
    private $clientId = 'php-mqtt-watcher';
    private $curValues = [];
    
    public function __construct() {
        $this->broker = $_ENV['MQTT_SERVER'];
        $this->port = $_ENV['MQTT_PORT'];
        $this->username = $_ENV['MQTT_USERNAME'];
        $this->password = $_ENV['MQTT_PASSWORD'];
        $this->clentId = $_ENV['MQTT_CLIENT_ID'];
        $this->topic = $_ENV['MQTT_BASE'] .'#';
        $this->mqtt = new phpMQTT($this->broker, $this->port, $this->clientId);
    }
    
    public function startWatching() {
        if ($this->mqtt->connect(true, NULL, $this->username, $this->password)) {
            echo "Успешно подключились к MQTT брокеру\n";
            echo "Подписываемся на топик: {$this->topic}\n";
            $this->mqtt->subscribe([$this->topic => ['qos' => 0, 'function' => [$this, 'handleMessage']]], 0);
            while ($this->mqtt->proc()) {
                // Бесконечный цикл обработки сообщений
            }
            $this->mqtt->close();
        } else {
            echo "Ошибка подключения к MQTT брокеру\n";
            exit(1);
        }
    }
    
    public function handleMessage($topic, $msg) {
        echo "Получено сообщение из топика {$topic}: {$msg}\n";
        $l = strlen($this->topic);
        if(substr($topic,0,$l) != $this->topic) 
            throw new \Exception("Invalid topic '{$topic}'");
        $key = substr($topic,$l); 
        echo "Got message {$key} = {$msg}\n";    
        if(!isset($this->curValues[$key])){
            $this->curValues[$key] = $msg;
        }

        if($this->curValues[$key] != $msg) {
            $this->valueChanged($key,$this->curValues[$key],$msg);
        }
        switch($key) {

        }
    }
    private function valueChanged($key,$old,$new)
    {
        
    }
    private function handleValueChange($oldValue, $newValue) {
        echo "Обрабатываем изменение: " . json_encode($oldValue) . 
             " -> " . json_encode($newValue) . "\n";
        
        // Пример действий при изменении значения
        if (is_numeric($newValue) && $newValue > 30) {
            echo "🔴 Температура превысила 30°C!\n";
        } elseif ($newValue === 'alert') {
            echo "🚨 Получен сигнал тревоги!\n";
        }
        
        // Добавьте свою логику здесь
    }
}

// Запуск программы
try {
    $dotenv = Dotenv\Dotenv::createImmutable(__DIR__);
    $dotenv->load();
    $watcher = new MQTTWatcher();
    $watcher->startWatching();
} catch (Exception $e) {
    echo "Ошибка: " . $e->getMessage() . "\n";
}
