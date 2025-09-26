#ifndef QRCODE_GENERATOR_H
#define QRCODE_GENERATOR_H

typedef struct {
    int version;
    int size;
    unsigned char *modules;
} QRCode;

// Funções QR Code
QRCode* qr_encode(const char *text);
void qr_free(QRCode *qrcode);
int qr_get_module(QRCode *qrcode, int x, int y);
int save_qr_as_bmp(QRCode *qrcode, const char *filename);

// Funções encurtador
char* shorten_url(const char *long_url);
char* http_get_request(const char *url);
char* http_post_request(const char *url, const char *data);
char* shorten_url_offline(const char *long_url);

#endif