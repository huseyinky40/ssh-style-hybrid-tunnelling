#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include "protocol.h"
#include "rsa.h"

uint8_t session_key[16];
uint32_t global_frame_counter = 0;
char client_name[32];

void generate_session_key() {
    srand(time(NULL) ^ (getpid() << 8)); 
    for(int i=0; i<16; i++) {
        session_key[i] = rand() % 256;
    }
}

void send_frame_reliable(int sock, Frame *frame) {
    Frame response;
    int attempt = 0;
    frame->frame_id = global_frame_counter++;
    
    while(1) {
        send(sock, frame, sizeof(Frame), 0); 
        
        int n = recv(sock, &response, sizeof(Frame), 0); 
        
        if (n <= 0) { printf("\n!! [%s] Baglantisi koptu !!\n", client_name); exit(1); } 

        if (response.msg_type == TYPE_ACK) {
            break;
        } 
        else if (response.msg_type == TYPE_NACK) {
            attempt++; 
            printf("\r>> [%s - HATA] Paket %u bozuldu, tekrar gonderiliyor... (Deneme: %d)", 
                    client_name, frame->frame_id, attempt);
            fflush(stdout);
            usleep(50000);
        }
    }
}

void perform_handshake(int sock) {
    printf(">> [%s] Handshake baslatiliyor...\n", client_name);
    
    Frame req; 
    memset(&req, 0, sizeof(Frame));
    req.msg_type = TYPE_HANDSHAKE_REQ; 
    strncpy((char*)req.data, client_name, MAX_PAYLOAD - 1);
    req.payload_size = strlen(client_name);
    send(sock, &req, sizeof(Frame), 0); 

    Frame res; 
    recv(sock, &res, sizeof(Frame), 0); 
    long long n, e; 
    sscanf((char*)res.data, "%lld %lld", &n, &e);

    generate_session_key(); 
    Frame key_frame; 
    memset(&key_frame, 0, sizeof(Frame));
    key_frame.msg_type = TYPE_HANDSHAKE_KEY; 
    
    long long *encrypted_ptr = (long long*)key_frame.data;
    for(int i=0; i<16; i++) {
        encrypted_ptr[i] = rsa_encrypt(session_key[i], e, n);
    }
    
    key_frame.payload_size = 16 * sizeof(long long); 
    key_frame.crc = calculate_crc32(key_frame.data, key_frame.payload_size); 
    
    send_frame_reliable(sock, &key_frame); 
    printf(">> [%s] Guvenli Handshake Tamamlandi.\n", client_name);
}

void send_msg(int sock, char *text) {
    Frame frame; 
    memset(&frame, 0, sizeof(Frame));
    frame.msg_type = TYPE_MSG;
    strncpy((char*)frame.data, text, MAX_PAYLOAD - 1); 
    frame.payload_size = strlen((char*)frame.data);
    
    perform_aes_encrypt(frame.data, frame.payload_size, session_key); 
    frame.crc = calculate_crc32(frame.data, frame.payload_size);    
    
    send_frame_reliable(sock, &frame);
    printf(">> Mesaj iletildi (ID: %u).\n", global_frame_counter - 1);
}

void send_file(int sock, char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) { printf(">> HATA: Dosya bulunamadi: %s\n", filename); return; }

    Frame frame;
    memset(&frame, 0, sizeof(Frame));
    frame.msg_type = TYPE_FILE_START;
    snprintf((char*)frame.data, MAX_PAYLOAD, "%s", filename);
    frame.payload_size = strlen((char*)frame.data);
    perform_aes_encrypt(frame.data, frame.payload_size, session_key); 
    frame.crc = calculate_crc32(frame.data, frame.payload_size);    
    send_frame_reliable(sock, &frame); 
    
    uint8_t buffer[MAX_PAYLOAD];
    size_t bytesRead;

    printf(">> '%s' gonderiliyor...\n", filename);
    while ((bytesRead = fread(buffer, 1, MAX_PAYLOAD, fp)) > 0) {
        memset(&frame, 0, sizeof(Frame));
        frame.msg_type = TYPE_FILE_DATA;
        frame.payload_size = bytesRead;
        memcpy(frame.data, buffer, bytesRead);
        
        perform_aes_encrypt(frame.data, frame.payload_size, session_key);
        frame.crc = calculate_crc32(frame.data, frame.payload_size);
        
        send_frame_reliable(sock, &frame);
    }
    
    memset(&frame, 0, sizeof(Frame));
    frame.msg_type = TYPE_FILE_END;
    frame.payload_size = 0;
    frame.crc = calculate_crc32(frame.data, 0);
    send_frame_reliable(sock, &frame);
    
    printf(">> Dosya transferi tamamlandi.\n");
    fclose(fp);
}

int main(int argc, char **argv) {
    if (argc < 4) { 
        printf("Kullanim: %s <ip> <port> <istemci_ismi>\n", argv[0]); 
        return 1; 
    }
    
    strncpy(client_name, argv[3], sizeof(client_name) - 1);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        perror("Baglanti hatasi"); return -1; 
    }

    perform_handshake(sock); 

    char input[256];
    printf("\n--- GUVENLI ILETISIM KONSOLU [%s] ---\n", client_name);
    while (1) {
        printf("\n[%s] Komut > ", client_name);
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "baglantikapa") == 0) break;
        else if (strncmp(input, "msg ", 4) == 0) send_msg(sock, input + 4);
        else if (strncmp(input, "file ", 5) == 0) send_file(sock, input + 5);
        else printf(">> Gecersiz komut! (msg <yazi> veya file <dosya_adi> kullan)\n");
    }

    close(sock); 
    return 0;
}