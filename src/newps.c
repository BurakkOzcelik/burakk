#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

// Sabitler
#define SERVER_PORT 12345
#define MAX_DATA_SIZE 1024

// TLV formatındaki veriyi işlemek için bir fonksiyon
void ProcessTLVData(const char *tlvData) {
    char *ptr = strtok(tlvData, "\t");
    while (ptr != NULL) {
        printf("Veri: %s\n", ptr);
        ptr = strtok(NULL, "\t");
    }
}

int main() {
    // Sunucu ve istemci soketleri, adres yapıları ve uzunluk değişkenleri tanımla
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    // Sunucu soketini oluştur
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Sunucu soketi oluşturulamadı");
        exit(EXIT_FAILURE);
    }

    // Sunucu adresini yapılandır
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    // Sunucu soketini belirtilen porta bağla
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Sunucu soketi bağlanamadı");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // İstemci bağlantılarını dinlemeye başla
    if (listen(serverSocket, 5) == -1) {
        perror("İstemci bağlantıları kabul edilemedi");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("[*] Sunucu %d portunu dinliyor...\n", SERVER_PORT);

    while (1) {
        // İstemci bağlantılarını kabul et
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSocket == -1) {
            perror("İstemci bağlantısı kabul edilemedi");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }

        printf("[*] İstemci bağlantısı kabul edildi: %s\n", inet_ntoa(clientAddress.sin_addr));

        char data[MAX_DATA_SIZE];

        while (1) {
            // İstemciden TLV formatında veriyi al
            ssize_t receivedBytes = recv(clientSocket, data, sizeof(data), 0);
            if (receivedBytes <= 0) {
                if (receivedBytes == 0) {
                    printf("Bağlantı kapatıldı\n");
                } else {
                    perror("recv hatası");
                }
                break;
            }

            data[receivedBytes] = '\0';

            // TLV formatındaki veriyi işle
            ProcessTLVData(data);

            // İstemciye cevap gönder
            printf("Cevap: ");
            fgets(data, sizeof(data), stdin);
            send(clientSocket, data, strlen(data), 0);

            // "exit" komutunu kontrol et
            if (strcmp(data, "exit\n") == 0) {
                printf("Sunucu kapatılıyor...\n");
                close(clientSocket);  // Bağlantıyı kapat
                close(serverSocket);  // Sunucu soketini kapat
                exit(EXIT_SUCCESS);    // Programı normal şekilde sonlandır
            }
        }

        // İstemci soketini kapat
        close(clientSocket);
    }

    return 0;
}
