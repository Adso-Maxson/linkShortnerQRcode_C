#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

// Função para codificar URL (simplificada)
static char* url_encode(const char *str) {
    if (!str) return NULL;
    
    int len = strlen(str);
    char *encoded = malloc(len * 3 + 1);
    if (!encoded) return NULL;
    
    int j = 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = str[i];
        
        if ((c >= 'A' && c <= 'Z') || 
            (c >= 'a' && c <= 'z') || 
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            encoded[j++] = c;
        } else if (c == ' ') {
            encoded[j++] = '+';
        } else {
            sprintf(encoded + j, "%%%02X", c);
            j += 3;
        }
    }
    encoded[j] = '\0';
    return encoded;
}

// Requisição HTTP GET simples
static char* http_get_request(const char *url) {
    HINTERNET hInternet = NULL;
    HINTERNET hUrl = NULL;
    char *response = NULL;
    DWORD bytesRead;
    char buffer[1024];
    DWORD totalBytesRead = 0;
    
    hInternet = InternetOpen("QRCodeGenerator/1.0", 
                            INTERNET_OPEN_TYPE_PRECONFIG, 
                            NULL, NULL, 0);
    if (!hInternet) {
        return NULL;
    }
    
    DWORD timeout = 8000;
    InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    hUrl = InternetOpenUrl(hInternet, url, NULL, 0, 
                          INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return NULL;
    }
    
    response = malloc(4096);
    if (!response) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return NULL;
    }
    response[0] = '\0';
    
    BOOL readResult;
    do {
        readResult = InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead);
        if (readResult && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            
            if (totalBytesRead + bytesRead < 4095) {
                strcat(response, buffer);
                totalBytesRead += bytesRead;
            } else {
                break;
            }
        }
    } while (readResult && bytesRead > 0);
    
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    
    if (totalBytesRead > 0) {
        return response;
    } else {
        free(response);
        return NULL;
    }
}

// Encurtador offline
static char* shorten_url_offline(const char *long_url) {
    if (!long_url || strlen(long_url) == 0) return NULL;
    
    unsigned long hash = 5381;
    const char *ptr = long_url;
    while (*ptr) {
        hash = ((hash << 5) + hash) + *ptr;
        ptr++;
    }
    
    char *short_url = malloc(64);
    if (!short_url) return NULL;
    
    sprintf(short_url, "https://ex.co/%lX", hash % 0xFFFF);
    return short_url;
}

// Encurtador principal
char* shorten_url(const char *long_url) {
    if (!long_url || strlen(long_url) == 0) {
        return NULL;
    }
    
    // Se a URL já é curta, retornar como está
    if (strlen(long_url) <= 50) {
        char *result = malloc(strlen(long_url) + 1);
        if (result) strcpy(result, long_url);
        return result;
    }
    
    // Tentar TinyURL
    char *encoded_url = url_encode(long_url);
    if (!encoded_url) return shorten_url_offline(long_url);
    
    char api_url[4096];
    sprintf(api_url, "http://tinyurl.com/api-create.php?url=%s", encoded_url);
    free(encoded_url);
    
    char *result = http_get_request(api_url);
    
    if (result && strlen(result) > 0) {
        if (strncmp(result, "http://", 7) == 0 || strncmp(result, "https://", 8) == 0) {
            return result;
        } else {
            free(result);
        }
    }
    
    // Se falhar, usar offline
    return shorten_url_offline(long_url);
}