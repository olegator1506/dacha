<?php

require_once __DIR__ . '/vendor/autoload.php';

use EmailToTelegram\Config;
use EmailToTelegram\EmailToTelegramParser;
use EmailToTelegram\LoggerFactory;

try {
    // Load configuration
    Config::load(__DIR__);
    
    // Create logger
    $logger = LoggerFactory::create();
    
    // Create and run parser
    $parser = new EmailToTelegramParser(Config::getAll(), $logger);
    $parser->processEmails();
    
    $logger->info('Email processing completed successfully');
    
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
    exit(1);
}