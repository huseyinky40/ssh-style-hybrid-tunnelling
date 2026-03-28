#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <arpa/inet.h>  
#include <sys/socket.h> 
#include <sys/select.h> 
#include <errno.h>      
#include "protocol.h"
#include "rsa.h"
#include "aes.h"

#define MAX_CLIENTS 10

struct ClientState {
    int socket;
    char name[32];
    FILE *file_ptr;
    uint8_t aes_key[16];
    int has_key;
    uint32_t last_frame_id;
};

struct ClientState clients[MAX_CLIENTS]; 
RSA_KeyPair server_rsa;                  

void send_response(int socket, int type) {
    Frame resp;
    memset(&resp, 0, sizeof(Frame));
    resp.msg_type = type;
    resp.payload_size = 0;
    send(socket, &resp, sizeof(Frame), 0);
}

void process_packet(int index, Frame *frame) {
    int sd = clients[index].socket;
    char *c_name = clients[index].name;

    if (frame->msg_type == TYPE_HANDSHAKE_REQ) {
        strncpy(clients[index].name, (char*)frame->data, 31);
        printf(">> [HANDSHAKE] Hoş geldin '%s'! Sana RSA anahtarımı gönderiyorum...\n", c_name);

        Frame resp;
        memset(&resp, 0, sizeof(Frame));
        resp.msg_type = TYPE_HANDSHAKE_RES;
        sprintf((char*)resp.data, "%lld %lld", server_rsa.n, server_rsa.e);
        resp.payload_size = strlen((char*)resp.data);
        send(sd, &resp, sizeof(Frame), 0);
        return;
    }

    if (frame->msg_type == TYPE_HANDSHAKE_KEY) {
        long long *encrypted_ptr = (long long*)frame->data;
        for(int i=0; i<16; i++) {
            clients[index].aes_key[i] = (uint8_t)rsa_decrypt(encrypted_ptr[i], server_rsa.d, server_rsa.n);
        }
        clients[index].has_key = 1;
        printf(">> [SECURITY] '%s' için AES oturum anahtarı başarıyla çözüldü ve kuruldu.\n", c_name);
        send_response(sd, TYPE_ACK);
        return;
    }

    if (clients[index].has_key == 0) return;

    uint32_t calc_crc = calculate_crc32(frame->data, frame->payload_size);
    if (calc_crc != frame->crc) {
        printf(">> [SABOTAJ!] '%s' tarafından gelen Frame %u bozulmuş! Reddediliyor (NACK).\n", c_name, frame->frame_id);
        send_response(sd, TYPE_NACK);
        return;
    }

    send_response(sd, TYPE_ACK); 
    clients[index].last_frame_id = frame->frame_id;

    if (frame->payload_size > 0) {
        perform_aes_decrypt(frame->data, frame->payload_size, clients[index].aes_key);
    }

    switch(frame->msg_type) {
        case TYPE_MSG:
            frame->data[frame->payload_size] = '\0';
            printf("\n>>> [%s Mesajı]: %s\n\n", c_name, frame->data);
            break;

        case TYPE_FILE_START:
        {
            char filename[300];
            snprintf(filename, sizeof(filename), "alinan_%s_%s", c_name, (char*)frame->data);
            clients[index].file_ptr = fopen(filename, "wb");
            if(clients[index].file_ptr) printf(">> [FILE] '%s' dosyası alınmaya başlanıyor...\n", filename);
            break;
        }

        case TYPE_FILE_DATA:
            if(clients[index].file_ptr) {
                fwrite(frame->data, 1, frame->payload_size, clients[index].file_ptr);
                printf("\r>> [%s] Frame %u disk üzerine yazıldı...", c_name, frame->frame_id);
                fflush(stdout);
            }
            break;

        case TYPE_FILE_END:
            if(clients[index].file_ptr) { 
                fclose(clients[index].file_ptr); 
                clients[index].file_ptr = NULL; 
                printf("\n>> [SUCCESS] '%s' transferi başarıyla tamamlandı.\n", c_name); 
            }
            break;
    }
}

int main() {
    generate_rsa_keys(&server_rsa);
    
    int master_socket, new_sock, addrlen, max_sd, sd;
    struct sockaddr_in address;
    fd_set readfds;

    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    
    bind(master_socket, (struct sockaddr *)&address, sizeof(address));
    listen(master_socket, 5);
    
    printf(">> [START] SSH Tunnel Server Aktif. Port: %d. İstemciler bekleniyor...\n", SERVER_PORT);

    for (int i=0; i<MAX_CLIENTS; i++) clients[i].socket = 0;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (int i=0; i<MAX_CLIENTS; i++) {
            sd = clients[i].socket;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_socket, &readfds)) {
            addrlen = sizeof(address);
            new_sock = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            
            for (int i=0; i<MAX_CLIENTS; i++) {
                if (clients[i].socket == 0) { 
                    clients[i].socket = new_sock; 
                    clients[i].has_key = 0;
                    printf(">> [BAGLANTI] Yeni bir istemci (Soket %d) tünele giriş yaptı.\n", new_sock); 
                    break; 
                }
            }
        }

        for (int i=0; i<MAX_CLIENTS; i++) {
            sd = clients[i].socket;
            if (FD_ISSET(sd, &readfds)) {
                Frame frame;
                int n = recv(sd, &frame, sizeof(Frame), 0);
                if (n <= 0) { 
                    printf(">> [AYRILIK] '%s' (Soket %d) tünelden ayrıldı.\n", clients[i].name, sd);
                    close(sd); 
                    clients[i].socket = 0;
                    if(clients[i].file_ptr) fclose(clients[i].file_ptr);
                } else {
                    process_packet(i, &frame);
                }
            }
        }
    }
    return 0;
}