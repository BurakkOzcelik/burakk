#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "cJSON.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define MAX_DATA_SIZE 1024

// JSON dosyasından okunan veriyi işleme fonksiyonu
void ProcessJSONData(const char *jsonData) {
    // cJSON_Parse ile JSON verisini parse et
    cJSON *root = cJSON_Parse(jsonData);
    if (root == NULL) {
        fprintf(stderr, "JSON parse hatası: %s\n", cJSON_GetErrorPtr());
        free(jsonData);
        exit(EXIT_FAILURE);
    }

    // "posCihaziVerileri" anahtarındaki diziyi al
    cJSON *posDeviceData = cJSON_GetObjectItemCaseSensitive(root, "posCihaziVerileri");
    if (!cJSON_IsArray(posDeviceData)) {
        fprintf(stderr, "'posCihaziVerileri' bir dizi değil\n");
        cJSON_Delete(root);
        free(jsonData);
        exit(EXIT_FAILURE);
    }

    cJSON *posDeviceItem;

    // Her bir öğeyi döngü ile işle
    cJSON_ArrayForEach(posDeviceItem, posDeviceData) {
        cJSON *date = cJSON_GetObjectItemCaseSensitive(posDeviceItem, "tarih");
        cJSON *time = cJSON_GetObjectItemCaseSensitive(posDeviceItem, "saat");
        cJSON *receiptNo = cJSON_GetObjectItemCaseSensitive(posDeviceItem, "fis_no");
        cJSON *product = cJSON_GetObjectItemCaseSensitive(posDeviceItem, "urun");
        cJSON *VAT = cJSON_GetObjectItemCaseSensitive(posDeviceItem, "TOPKDV");
        cJSON *total = cJSON_GetObjectItemCaseSensitive(posDeviceItem, "TOPLAM");

        // Verileri kullanarak istediğin işlemleri gerçekleştir
        printf("Tarih: %s, Saat: %s, Fis No: %s, Urun: %s, TOPKDV: %.2f, TOPLAM: %.2f\n",
               date->valuestring, time->valuestring, receiptNo->valuestring, product->valuestring,
               VAT->valuedouble, total->valuedouble);

        // İstemci soketini oluştur
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            perror("İstemci soketi oluşturulamadı");
            cJSON_Delete(root);
            free(jsonData);
            exit(EXIT_FAILURE);
        }

        // Sunucu adresini yapılandır
        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
        serverAddress.sin_port = htons(SERVER_PORT);

        // Sunucuya bağlan
        if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            perror("Sunucuya bağlanılamadı");
            close(clientSocket);
            cJSON_Delete(root);
            free(jsonData);
            exit(EXIT_FAILURE);
        }

        printf("[*] Sunucuya bağlanıldı\n");

        // TLV formatına çevirme
        char tlvBuffer[MAX_DATA_SIZE];
        sprintf(tlvBuffer, "%s\t%s\t%s\t%s\t%.2f\t%.2f", date->valuestring, time->valuestring,
                receiptNo->valuestring, product->valuestring, VAT->valuedouble, total->valuedouble);

        // TLV verisini sunucuya gönderme
        send(clientSocket, tlvBuffer, strlen(tlvBuffer), 0);

        // Sunucudan cevap al
        char buffer[MAX_DATA_SIZE];
        ssize_t receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (receivedBytes <= 0) {
            perror("Veri alınamadı");
            close(clientSocket);
            cJSON_Delete(root);
            free(jsonData);
            exit(EXIT_FAILURE);
        }

        buffer[receivedBytes] = '\0';
        printf("Alınan Cevap: %s\n", buffer);

        // Soketi kapat
        close(clientSocket);
    }

    // cJSON nesnesini temizle
    cJSON_Delete(root);
    free(jsonData);
}

int main() {
    // JSON dosyasından veriyi oku
    FILE *jsonFile = fopen("sample.json", "r");
    if (!jsonFile) {
        perror("JSON dosyası açılamadı");
        return 1;
    }

    fseek(jsonFile, 0, SEEK_END);
    long fileSize = ftell(jsonFile);
    fseek(jsonFile, 0, SEEK_SET);

    char *jsonData = (char *)malloc(fileSize + 1);
    fread(jsonData, 1, fileSize, jsonFile);
    fclose(jsonFile);

    // JSON verisini sonlandırıcı ile işaretle
    jsonData[fileSize] = '\0';

    // JSON verisini işle
    ProcessJSONData(jsonData);

    return 0;
}
