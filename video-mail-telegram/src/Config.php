<?php

namespace EmailToTelegram;

use Dotenv\Dotenv;

class Config {
    private static ?array $config = null;

    public static function load(string $path): void {
        $dotenv = Dotenv::createImmutable($path);
        $dotenv->load();
        $dotenv->required([
            'EMAIL_IMAP_HOST',
            'EMAIL_IMAP_USERNAME', 
            'EMAIL_IMAP_PASSWORD',
            'TELEGRAM_BOT_TOKEN',
            'TELEGRAM_CHAT_ID'
        ]);
    }

    public static function get(string $key, $default = null) {
        return $_ENV[$key] ?? $default;
    }

    public static function getAll(): array {
        return [
            'imap_host' => self::get('EMAIL_IMAP_HOST'),
            'imap_username' => self::get('EMAIL_IMAP_USERNAME'),
            'imap_password' => self::get('EMAIL_IMAP_PASSWORD'),
            'mark_as_read' => self::get('EMAIL_MARK_AS_READ', 'true') === 'true',
            'max_text_length' => (int) self::get('EMAIL_MAX_TEXT_LENGTH', '1000'),
            'telegram_bot_token' => self::get('TELEGRAM_BOT_TOKEN'),
            'telegram_chat_id' => self::get('TELEGRAM_CHAT_ID'),
            'alarm_subject' => self::get('ALARM_SUBJECT'),
        ];
    }
}