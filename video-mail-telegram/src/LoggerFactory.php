<?php

namespace EmailToTelegram;

use Monolog\Logger;
use Monolog\Handler\StreamHandler;
use Monolog\Handler\RotatingFileHandler;
use Monolog\Formatter\LineFormatter;

class LoggerFactory {
    public static function create(): Logger {
        $logLevel = self::getLogLevel();
        $logChannel = $_ENV['LOG_CHANNEL'] ?? 'email_parser';
        
        $logger = new Logger($logChannel);
        
        $formatter = new LineFormatter(
            "[%datetime%] %channel%.%level_name%: %message% %context% %extra%\n",
            'Y-m-d H:i:s'
        );
        
        $streamHandler = new StreamHandler('php://stdout', $logLevel);
        $streamHandler->setFormatter($formatter);
        $logger->pushHandler($streamHandler);
        
        $fileHandler = new RotatingFileHandler(
            __DIR__ . '/../logs/email_parser.log',
            7, // keep 7 days
            $logLevel
        );
        $fileHandler->setFormatter($formatter);
        $logger->pushHandler($fileHandler);
        
        return $logger;
    }
    
    private static function getLogLevel(): int {
        $level = strtoupper($_ENV['LOG_LEVEL'] ?? 'INFO');
        
        return match ($level) {
            'DEBUG' => Logger::DEBUG,
            'INFO' => Logger::INFO,
            'NOTICE' => Logger::NOTICE,
            'WARNING' => Logger::WARNING,
            'ERROR' => Logger::ERROR,
            'CRITICAL' => Logger::CRITICAL,
            'ALERT' => Logger::ALERT,
            'EMERGENCY' => Logger::EMERGENCY,
            default => Logger::INFO,
        };
    }
}