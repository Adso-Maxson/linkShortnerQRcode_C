#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef struct {
    int version;
    int size;
    unsigned char *modules;
} QRCode;

// Função para definir módulo com verificação de limites
static void set_module(QRCode *qrcode, int x, int y, int value) {
    if (x >= 0 && y >= 0 && x < qrcode->size && y < qrcode->size) {
        qrcode->modules[y * qrcode->size + x] = value;
    }
}

// Desenhar padrão de encontrar completo
static void draw_finder_pattern(QRCode *qrcode, int x, int y) {
    // Quadrado externo 7x7 preto
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            set_module(qrcode, x + i, y + j, 1);
        }
    }
    
    // Quadrado interno 5x5 branco
    for (int i = 1; i < 6; i++) {
        for (int j = 1; j < 6; j++) {
            set_module(qrcode, x + i, y + j, 0);
        }
    }
    
    // Quadrado central 3x3 preto
    for (int i = 2; i < 5; i++) {
        for (int j = 2; j < 5; j++) {
            set_module(qrcode, x + i, y + j, 1);
        }
    }
}

// Codificação melhorada de dados
static void encode_data_improved(QRCode *qrcode, const char *text) {
    int size = qrcode->size;
    int text_len = strlen(text);
    int data_index = 0;
    
    // Padrão de codificação simples mas funcional
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Pular áreas reservadas (finder patterns, timing, etc.)
            if ((x < 9 && y < 9) || 
                (x > size - 9 && y < 9) || 
                (x < 9 && y > size - 9) ||
                (x > size - 9 && y > size - 9) ||
                (y == 6) || (x == 6)) {
                continue;
            }
            
            if (data_index < text_len * 8) {
                int char_index = data_index / 8;
                int bit_index = data_index % 8;
                int bit = (text[char_index] >> (7 - bit_index)) & 1;
                set_module(qrcode, x, y, bit);
                data_index++;
            }
        }
    }
}

// Função principal de codificação QR melhorada
QRCode* qr_encode(const char *text) {
    int text_len = strlen(text);
    if (text_len == 0) return NULL;
    
    // Usar versão 3 para maior capacidade
    int version = 3;
    int size = 29; // Tamanho fixo para versão 3
    
    QRCode *qrcode = malloc(sizeof(QRCode));
    if (!qrcode) return NULL;
    
    qrcode->version = version;
    qrcode->size = size;
    qrcode->modules = calloc(size * size, sizeof(unsigned char));
    
    if (!qrcode->modules) {
        free(qrcode);
        return NULL;
    }
    
    // Preencher tudo com branco primeiro
    for (int i = 0; i < size * size; i++) {
        qrcode->modules[i] = 0;
    }
    
    // 1. Padrões de encontrar nos três cantos
    draw_finder_pattern(qrcode, 0, 0);           // Superior esquerdo
    draw_finder_pattern(qrcode, 0, size - 7);    // Inferior esquerdo  
    draw_finder_pattern(qrcode, size - 7, 0);    // Superior direito
    
    // 2. Padrões de timing
    for (int i = 8; i < size - 8; i++) {
        if (i % 2 == 0) {
            set_module(qrcode, i, 6, 1); // Horizontal
            set_module(qrcode, 6, i, 1); // Vertical
        }
    }
    
    // 3. Codificar dados
    encode_data_improved(qrcode, text);
    
    return qrcode;
}

// ... (resto das funções permanece igual)
void qr_free(QRCode *qrcode) {
    if (qrcode) {
        if (qrcode->modules) free(qrcode->modules);
        free(qrcode);
    }
}

int qr_get_module(QRCode *qrcode, int x, int y) {
    if (!qrcode || !qrcode->modules || x < 0 || y < 0 || x >= qrcode->size || y >= qrcode->size) {
        return 0;
    }
    return qrcode->modules[y * qrcode->size + x];
}

int save_qr_as_bmp(QRCode *qrcode, const char *filename) {
    if (!qrcode || !filename) return 0;
    
    int moduleSize = 15; // Aumentado para melhor legibilidade
    int border = 8;      // Borda maior
    int imageSize = (qrcode->size + border * 2) * moduleSize;
    
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;
    
    int bytesPerPixel = 3;
    int stride = ((imageSize * bytesPerPixel + 3) / 4) * 4;
    int imageDataSize = stride * imageSize;
    
    bmfh.bfType = 0x4D42;
    bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageDataSize;
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = imageSize;
    bmih.biHeight = imageSize;
    bmih.biPlanes = 1;
    bmih.biBitCount = 24;
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = imageDataSize;
    bmih.biXPelsPerMeter = 2835;
    bmih.biYPelsPerMeter = 2835;
    bmih.biClrUsed = 0;
    bmih.biClrImportant = 0;
    
    FILE *file = fopen(filename, "wb");
    if (!file) return 0;
    
    fwrite(&bmfh, sizeof(bmfh), 1, file);
    fwrite(&bmih, sizeof(bmih), 1, file);
    
    unsigned char *pixelData = calloc(imageDataSize, 1);
    if (!pixelData) {
        fclose(file);
        return 0;
    }
    
    // Fundo branco
    for (int i = 0; i < imageDataSize; i++) {
        pixelData[i] = 255;
    }
    
    // Desenhar QR Code
    for (int y = 0; y < imageSize; y++) {
        for (int x = 0; x < imageSize; x++) {
            int qrX = (x / moduleSize) - border;
            int qrY = (y / moduleSize) - border;
            
            int isModule = 0;
            if (qrX >= 0 && qrY >= 0 && qrX < qrcode->size && qrY < qrcode->size) {
                isModule = qr_get_module(qrcode, qrX, qrY);
            }
            
            int pixelIndex = (imageSize - 1 - y) * stride + x * bytesPerPixel;
            
            if (isModule) {
                pixelData[pixelIndex] = 0;
                pixelData[pixelIndex + 1] = 0;
                pixelData[pixelIndex + 2] = 0;
            }
        }
    }
    
    fwrite(pixelData, imageDataSize, 1, file);
    free(pixelData);
    fclose(file);
    
    return 1;
}