#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wininet.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

// Declarações das funções externas
typedef struct {
    int version;
    int size;
    unsigned char *modules;
} QRCode;

extern QRCode* qr_encode(const char *text);
extern void qr_free(QRCode *qrcode);
extern int qr_get_module(QRCode *qrcode, int x, int y);
extern int save_qr_as_bmp(QRCode *qrcode, const char *filename);
extern char* shorten_url(const char *long_url);

// Variáveis globais
static HWND hEdit, hGenerateBtn, hShortenBtn, hSaveBtn, hStatus, hCheckShorten;
static HWND hOriginalUrlText, hShortenedUrlText, hCopyOriginalBtn, hCopyShortenedBtn;
static QRCode *currentQR = NULL;
static char originalURL[1024] = "";
static char shortenedURL[1024] = "";
static BOOL programStarted = FALSE;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            programStarted = TRUE; // Marcar que o programa já iniciou
            
            HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                   DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
            
            CreateWindow("STATIC", "URL para encurtar e gerar QR Code:", 
                WS_CHILD | WS_VISIBLE, 20, 20, 300, 20, hwnd, NULL, NULL, NULL);
            
            hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", 
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                20, 45, 540, 25, hwnd, NULL, NULL, NULL);
            SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hCheckShorten = CreateWindow("BUTTON", "Encurtar link antes de gerar QR", 
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                20, 80, 250, 20, hwnd, NULL, NULL, NULL);
            SendMessage(hCheckShorten, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hCheckShorten, BM_SETCHECK, BST_CHECKED, 0);
            
            hShortenBtn = CreateWindow("BUTTON", "Apenas Encurtar Link", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 110, 150, 35, hwnd, (HMENU)3, NULL, NULL);
            SendMessage(hShortenBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hGenerateBtn = CreateWindow("BUTTON", "Gerar QR Code", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                180, 110, 150, 35, hwnd, (HMENU)1, NULL, NULL);
            SendMessage(hGenerateBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hSaveBtn = CreateWindow("BUTTON", "Salvar QR Code", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                340, 110, 150, 35, hwnd, (HMENU)2, NULL, NULL);
            SendMessage(hSaveBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            EnableWindow(hSaveBtn, FALSE);
            
            CreateWindow("STATIC", "Informações dos Links:", 
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 160, 540, 20, hwnd, NULL, NULL, NULL);
            
            CreateWindow("STATIC", "URL Original:", 
                WS_CHILD | WS_VISIBLE, 20, 190, 100, 20, hwnd, NULL, NULL, NULL);
            
            hOriginalUrlText = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "Digite uma URL acima", 
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                120, 190, 350, 25, hwnd, NULL, NULL, NULL);
            SendMessage(hOriginalUrlText, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hCopyOriginalBtn = CreateWindow("BUTTON", "Copiar", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                480, 190, 80, 25, hwnd, (HMENU)4, NULL, NULL);
            SendMessage(hCopyOriginalBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            EnableWindow(hCopyOriginalBtn, FALSE);
            
            CreateWindow("STATIC", "URL Encurtada:", 
                WS_CHILD | WS_VISIBLE, 20, 225, 100, 20, hwnd, NULL, NULL, NULL);
            
            hShortenedUrlText = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "Aguardando encurtamento", 
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                120, 225, 350, 25, hwnd, NULL, NULL, NULL);
            SendMessage(hShortenedUrlText, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hCopyShortenedBtn = CreateWindow("BUTTON", "Copiar", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                480, 225, 80, 25, hwnd, (HMENU)5, NULL, NULL);
            SendMessage(hCopyShortenedBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            EnableWindow(hCopyShortenedBtn, FALSE);
            
            hStatus = CreateWindow("STATIC", "Digite uma URL e clique em Gerar QR Code", 
                WS_CHILD | WS_VISIBLE,
                20, 260, 540, 30, hwnd, NULL, NULL, NULL);
            
            SetWindowText(hEdit, "https://www.exemplo.com");
            break;
        }
        
        case WM_COMMAND: {
            // Só processar comandos se o programa já estiver iniciado
            if (!programStarted) break;
            
            int cmd = LOWORD(wParam);
            
            if (cmd == 4) { // Copiar URL Original
                if (strlen(originalURL) > 0 && OpenClipboard(hwnd)) {
                    EmptyClipboard();
                    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, strlen(originalURL) + 1);
                    if (hGlob) {
                        char *pGlob = (char*)GlobalLock(hGlob);
                        strcpy(pGlob, originalURL);
                        GlobalUnlock(hGlob);
                        SetClipboardData(CF_TEXT, hGlob);
                    }
                    CloseClipboard();
                    MessageBox(hwnd, "URL original copiada!", "Sucesso", MB_OK);
                }
                break;
            }
            
            if (cmd == 5) { // Copiar URL Encurtada
                if (strlen(shortenedURL) > 0 && OpenClipboard(hwnd)) {
                    EmptyClipboard();
                    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, strlen(shortenedURL) + 1);
                    if (hGlob) {
                        char *pGlob = (char*)GlobalLock(hGlob);
                        strcpy(pGlob, shortenedURL);
                        GlobalUnlock(hGlob);
                        SetClipboardData(CF_TEXT, hGlob);
                    }
                    CloseClipboard();
                    MessageBox(hwnd, "URL encurtada copiada!", "Sucesso", MB_OK);
                }
                break;
            }
            
            char text[1024];
            GetWindowText(hEdit, text, sizeof(text));
            
            if (cmd == 3 || cmd == 1) { // Apenas Encurtar ou Gerar QR Code
                if (strlen(text) == 0) {
                    MessageBox(hwnd, "Por favor, digite uma URL!", "Erro", MB_ICONERROR);
                    break;
                }
            }
            
            if (strlen(text) > 0) {
                strcpy(originalURL, text);
            }
            
            if (cmd == 3) { // Apenas Encurtar
                SetWindowText(hStatus, "Encurtando URL...");
                UpdateWindow(hwnd);
                
                char *short_url = shorten_url(text);
                if (short_url) {
                    strcpy(shortenedURL, short_url);
                    SetWindowText(hOriginalUrlText, originalURL);
                    SetWindowText(hShortenedUrlText, shortenedURL);
                    SetWindowText(hEdit, short_url);
                    free(short_url);
                    SetWindowText(hStatus, "URL encurtada com sucesso!");
                    EnableWindow(hCopyOriginalBtn, TRUE);
                    EnableWindow(hCopyShortenedBtn, TRUE);
                } else {
                    strcpy(shortenedURL, text);
                    SetWindowText(hStatus, "Erro ao encurtar URL. Usando URL original.");
                }
            }
            else if (cmd == 1) { // Gerar QR Code
                SetWindowText(hStatus, "Processando...");
                UpdateWindow(hwnd);
                
                if (currentQR) {
                    qr_free(currentQR);
                    currentQR = NULL;
                }
                
                char *url_to_encode = text;
                
                if (SendMessage(hCheckShorten, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    SetWindowText(hStatus, "Encurtando URL...");
                    UpdateWindow(hwnd);
                    
                    char *short_url = shorten_url(text);
                    if (short_url) {
                        strcpy(shortenedURL, short_url);
                        url_to_encode = short_url;
                    } else {
                        strcpy(shortenedURL, text);
                    }
                } else {
                    strcpy(shortenedURL, text);
                }
                
                SetWindowText(hStatus, "Gerando QR Code...");
                UpdateWindow(hwnd);
                
                currentQR = qr_encode(url_to_encode);
                
                if (SendMessage(hCheckShorten, BM_GETCHECK, 0, 0) == BST_CHECKED && url_to_encode != text) {
                    free(url_to_encode);
                }
                
                if (currentQR) {
                    SetWindowText(hOriginalUrlText, originalURL);
                    SetWindowText(hShortenedUrlText, shortenedURL);
                    SetWindowText(hStatus, "QR Code gerado com sucesso!");
                    EnableWindow(hSaveBtn, TRUE);
                    EnableWindow(hCopyOriginalBtn, TRUE);
                    EnableWindow(hCopyShortenedBtn, TRUE);
                    InvalidateRect(hwnd, NULL, TRUE);
                } else {
                    SetWindowText(hStatus, "Erro ao gerar QR Code!");
                }
            }
            else if (cmd == 2) { // Salvar QR Code
                if (!currentQR) break;
                
                OPENFILENAME ofn;
                char szFile[260] = "qrcode.bmp";
                
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "BMP\0*.bmp\0Todos\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                
                if (GetSaveFileName(&ofn)) {
                    if (save_qr_as_bmp(currentQR, szFile)) {
                        char status[512];
                        sprintf(status, "QR Code salvo: %s", szFile);
                        SetWindowText(hStatus, status);
                    } else {
                        SetWindowText(hStatus, "Erro ao salvar arquivo!");
                    }
                }
            }
            break;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (currentQR) {
                int startX = 150; // Centralizado horizontalmente
                int startY = 320; // Posição mais abaixo
                int moduleSize = 8; // Módulos maiores para melhor leitura
                
                HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
                HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
                
                // Desenhar borda branca maior
                int qrDisplaySize = currentQR->size * moduleSize;
                RECT borderRect = {
                    startX - 15, 
                    startY - 15, 
                    startX + qrDisplaySize + 15, 
                    startY + qrDisplaySize + 15
                };
                FillRect(hdc, &borderRect, whiteBrush);
                
                // Desenhar QR Code com módulos maiores
                for (int y = 0; y < currentQR->size; y++) {
                    for (int x = 0; x < currentQR->size; x++) {
                        HBRUSH brush = qr_get_module(currentQR, x, y) ? blackBrush : whiteBrush;
                        
                        RECT rect = {
                            startX + x * moduleSize,
                            startY + y * moduleSize,
                            startX + (x + 1) * moduleSize,
                            startY + (y + 1) * moduleSize
                        };
                        
                        FillRect(hdc, &rect, brush);
                    }
                }
                
                // Adicionar texto informativo abaixo do QR Code
                char infoText[100];
                sprintf(infoText, "QR Code: %dx%d pixels", currentQR->size, currentQR->size);
                TextOut(hdc, startX, startY + qrDisplaySize + 20, infoText, strlen(infoText));
                
                DeleteObject(blackBrush);
                DeleteObject(whiteBrush);
            }
            
            EndPaint(hwnd, &ps);
            break;
        }
        
        case WM_DESTROY: {
            if (currentQR) qr_free(currentQR);
            PostQuitMessage(0);
            break;
        }
        
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "QRCodeGenerator";
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Gerador de QR Code + Encurtador de Links",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
                              650, 900, NULL, NULL, hInstance, NULL); // Janela maior
    
    if (hwnd == NULL) return 0;
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}