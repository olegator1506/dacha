<?php

namespace EmailToTelegram;

use Exception;
use GuzzleHttp\Client;
use GuzzleHttp\Exception\GuzzleException;
use Monolog\Logger;

class EmailToTelegramParser {
    private string $telegramBotToken;
    private string $telegramChatId;
    private string $imapHost;
    private string $imapUsername;
    private string $imapPassword;
    private bool $markAsRead;
    private int $maxTextLength;
    private Logger $logger;
    private Client $httpClient;
    private ?object $mailbox = null;

    public function __construct(array $config, Logger $logger) {
        $this->telegramBotToken = $config['telegram_bot_token'];
        $this->telegramChatId = $config['telegram_chat_id'];
        $this->imapHost = $config['imap_host'];
        $this->imapUsername = $config['imap_username'];
        $this->imapPassword = $config['imap_password'];
        $this->markAsRead = $config['mark_as_read'] ?? true;
        $this->maxTextLength = $config['max_text_length'] ?? 1000;
        $this->logger = $logger;
        
        $this->httpClient = new Client([
            'timeout' => 30,
            'connect_timeout' => 10,
        ]);
    }

    public function processEmails(): void {
        try {
            $this->connectToMailbox();
            $emails = $this->getUnreadEmails();
            
            if (empty($emails)) {
                $this->logger->info('No new emails found');
                return;
            }
            
            $this->logger->info('Found ' . count($emails) . ' new emails');
            $this->processEmailBatch($emails);
            
        } catch (Exception $e) {
            $this->logger->error('Error processing emails: ' . $e->getMessage());
            throw $e;
        } finally {
            $this->closeMailbox();
        }
    }

    private function connectToMailbox(): void {
        $this->logger->debug('Connecting to mailbox: ' . $this->imapHost);
        
        $this->mailbox = imap_open(
            $this->imapHost, 
            $this->imapUsername, 
            $this->imapPassword,
            0, // flags
            1  // retries
        );

        if (!$this->mailbox) {
            throw new Exception('Failed to connect to mailbox: ' . imap_last_error());
        }

        $this->logger->info('Successfully connected to mailbox');
    }

    private function getUnreadEmails(): array {
        $emails = imap_search($this->mailbox, 'UNSEEN');
        return $emails ?: [];
    }

    private function processEmailBatch(array $emails): void {
        foreach ($emails as $emailNumber) {
            try {
                $this->processSingleEmail($emailNumber);
            } catch (Exception $e) {
                $this->logger->error("Error processing email #{$emailNumber}: " . $e->getMessage());
                continue;
            }
        }
    }

    private function processSingleEmail(int $emailNumber): void {
        $overview = imap_fetch_overview($this->mailbox, $emailNumber, 0);
        $subject = $this->decodeSubject($overview[0]->subject ?? '');
        $from = $overview[0]->from ?? 'Unknown';
        $date = $overview[0]->date ?? '';

        $this->logger->info("Processing email: {$subject} from {$from}");
        if($subject != Config::get('ALARM_SUBJECT')) {
            $this->logger->warning("Wrong message subject '{$subject}'. Message skipped");
            return;
        }
        $structure = imap_fetchstructure($this->mailbox, $emailNumber);
        
        $textContent = '';
        $images = [];
        
        $this->parseEmailStructure($emailNumber, $structure, $textContent, $images);
        if(empty($textContent) || empty($images)) {
            $this->logger->info("Empty message or no image. Skipped");
            return;
        }
        $this->sendToTelegram($subject, $textContent, $images, $from, $date);

        if ($this->markAsRead) {
            imap_setflag_full($this->mailbox, $emailNumber, "\\Seen");
        }
    }

    private function parseEmailStructure(
        int $emailNumber, 
        object $structure, 
        string &$textContent, 
        array &$images, 
        string $partNumber = ''
    ): void {
        if ($structure->type === 1) { // multipart
            foreach ($structure->parts as $index => $part) {
                $this->parseEmailStructure(
                    $emailNumber, 
                    $part, 
                    $textContent, 
                    $images, 
                    $partNumber . ($partNumber ? '.' : '') . ($index + 1)
                );
            }
        } else {
            $this->processEmailPart($emailNumber, $structure, $partNumber, $textContent, $images);
        }
    }

    private function processEmailPart(
        int $emailNumber, 
        object $structure, 
        string $partNumber, 
        string &$textContent, 
        array &$images
    ): void {
        $data = imap_fetchbody($this->mailbox, $emailNumber, $partNumber ?: '1');
        $data = $this->decodeData($data, $structure->encoding);

        switch ($structure->type) {
            case 0: // text
                $this->processTextPart($structure, $data, $textContent);
                break;
            case 5: // image
            case 2: // attachment that might be image
                $this->processImagePart($structure, $data, $images);
                break;
        }
    }

    private function processTextPart(object $structure, string $data, string &$textContent): void {
        $subtype = strtoupper($structure->subtype ?? 'PLAIN');
        
        if ($subtype === 'PLAIN') {
            $textContent .= trim($data) . "\n\n";
        } elseif ($subtype === 'HTML') {
            $cleanText = strip_tags($data);
            $cleanText = html_entity_decode($cleanText);
            $textContent .= trim($cleanText) . "\n\n";
        }
    }

    private function processImagePart(object $structure, string $data, array &$images): void {
        $filename = $this->getAttachmentFilename($structure);
        $mimeType = $this->getMimeType($structure);

        if ($this->isImageMimeType($mimeType)) {
            $images[] = [
                'filename' => $filename ?: 'image_' . time() . '.' . strtolower($structure->subtype ?? 'jpg'),
                'data' => $data,
                'mime_type' => $mimeType
            ];
            
            $this->logger->debug("Found image attachment: {$filename}");
        }
    }

    private function getAttachmentFilename(object $structure): string {
        $filename = '';

        if (!empty($structure->dparameters)) {
            foreach ($structure->dparameters as $param) {
                if (strtoupper($param->attribute) === 'FILENAME') {
                    $filename = $this->decodeMimeString($param->value);
                    break;
                }
            }
        }

        if (!$filename && !empty($structure->parameters)) {
            foreach ($structure->parameters as $param) {
                if (strtoupper($param->attribute) === 'NAME') {
                    $filename = $this->decodeMimeString($param->value);
                    break;
                }
            }
        }

        return $filename;
    }

    private function decodeData(string $data, int $encoding): string {
        return match ($encoding) {
            3 => base64_decode($data),
            4 => quoted_printable_decode($data),
            1, 2 => $data,
            default => $data,
        };
    }

    private function decodeSubject(string $subject): string {
        $decoded = imap_mime_header_decode($subject);
        $result = '';
        
        foreach ($decoded as $part) {
            $result .= $part->text;
        }
        
        return $this->decodeMimeString($result);
    }

    private function decodeMimeString(string $string): string {
        return iconv_mime_decode($string, 0, 'UTF-8');
    }

    private function getMimeType(object $structure): string {
        $typeMap = [
            'JPEG' => 'image/jpeg',
            'JPG' => 'image/jpeg',
            'PNG' => 'image/png',
            'GIF' => 'image/gif',
            'BMP' => 'image/bmp',
            'WEBP' => 'image/webp',
        ];

        $subtype = strtoupper($structure->subtype ?? 'JPEG');
        return $typeMap[$subtype] ?? 'image/jpeg';
    }

    private function isImageMimeType(string $mimeType): bool {
        return str_starts_with($mimeType, 'image/');
    }

    private function sendToTelegram(string $subject, string $text, array $images, string $from, string $date): void {
        $message = $this->formatTelegramMessage($subject, $text, $from, $date);

        try {
            if (!empty($images)) {
                $this->sendPhotoToTelegram($message, $images);
            } else {
                $this->sendTextToTelegram($message);
            }
            
            $this->logger->info('Message successfully sent to Telegram');
            
        } catch (Exception $e) {
            $this->logger->error('Failed to send message to Telegram: ' . $e->getMessage());
            throw $e;
        }
    }

    private function formatTelegramMessage(string $subject, string $text, string $from, string $date): string {
        $formattedDate = date('Y-m-d H:i:s', strtotime($date));
        $truncatedText = mb_substr(trim($text), 0, $this->maxTextLength);
        
        if (mb_strlen($text) > $this->maxTextLength) {
            $truncatedText .= '...';
        }

        return <<<MSG
📧 <b>{$subject}</b>
<b>Текст:</b>
{$truncatedText}
MSG;
    }

    private function sendTextToTelegram(string $message): void {
        $url = "https://api.telegram.org/bot{$this->telegramBotToken}/sendMessage";
        
        $response = $this->httpClient->post($url, [
            'form_params' => [
                'chat_id' => $this->telegramChatId,
                'text' => $message,
                'parse_mode' => 'HTML',
            ],
        ]);

        if ($response->getStatusCode() !== 200) {
            throw new Exception('Telegram API returned non-200 status code');
        }
    }

    private function sendPhotoToTelegram(string $caption, array $images): void {
        // Send first image with caption
        $firstImage = $images[0];
        $this->sendSinglePhoto($firstImage, $caption);

        // Send remaining images without caption
        for ($i = 1; $i < count($images); $i++) {
            $this->sendSinglePhoto($images[$i]);
        }
    }

    private function sendSinglePhoto(array $image, string $caption = ''): void {
        $url = "https://api.telegram.org/bot{$this->telegramBotToken}/sendPhoto";
        
        $options = [
            'multipart' => [
                [
                    'name' => 'chat_id',
                    'contents' => $this->telegramChatId,
                ],
                [
                    'name' => 'photo',
                    'contents' => $image['data'],
                    'filename' => $image['filename'],
                ]
            ]
        ];

        if ($caption) {
            $options['multipart'][] = [
                'name' => 'caption',
                'contents' => $caption,
            ];
            $options['multipart'][] = [
                'name' => 'parse_mode',
                'contents' => 'HTML',
            ];
        }

        $response = $this->httpClient->post($url, $options);

        if ($response->getStatusCode() !== 200) {
            throw new Exception('Failed to send photo to Telegram');
        }
    }

    private function closeMailbox(): void {
        if ($this->mailbox) {
            imap_close($this->mailbox);
            $this->mailbox = null;
            $this->logger->debug('Mailbox connection closed');
        }
    }

    public function __destruct() {
        $this->closeMailbox();
    }
}