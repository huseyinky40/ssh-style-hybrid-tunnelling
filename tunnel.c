#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <arpa/inet.h>  
#include <sys/socket.h> 
#include <sys/select.h> 
#include <time.h>       

#define BUF_SIZE 8192            
#define NOISE_PROBABILITY 10     

int error_triggered = 0; 

void inject_noise(char *buffer, int len) {
    unsigned char msg_type = (unsigned char)buffer[4];
    if (msg_type >= 10 && msg_type <= 12) return;

    if ((rand() % 100) < NOISE_PROBABILITY) {
        if (len > 16) {
            int target_byte = 12 + (rand() % 4); 
            buffer[target_byte] ^= 0xFF;         
            error_triggered = 1;                 
            
            printf("\n[TUNEL] !!! GURULTU ENJEKTE EDILDI !!! -> Frame bozuldu.\n");
            fflush(stdout); 
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Kullanim: %s <dinleme_portu> <hedef_ip> <hedef_port>\n", argv[0]);
        return 1;
    }

    srand(time(NULL)); 
    int listen_port = atoi(argv[1]);   
    const char *target_ip = argv[2];   
    int target_port = atoi(argv[3]);   

    int s_listen = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(s_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
    
    struct sockaddr_in l_addr;
    memset(&l_addr, 0, sizeof(l_addr));
    l_addr.sin_family = AF_INET;
    l_addr.sin_addr.s_addr = INADDR_ANY; 
    l_addr.sin_port = htons(listen_port);
    
    bind(s_listen, (struct sockaddr *)&l_addr, sizeof(l_addr));
    listen(s_listen, 5); 

    printf(">> TUNEL AKTIF: %d portu dinleniyor...\n", listen_port);
    printf(">> HEDEF SUNUCU: %s:%d\n", target_ip, target_port);

    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);
    int fd_client = accept(s_listen, (struct sockaddr *)&c_addr, &c_len);
    printf(">> Client tunele baglandi.\n");

    int fd_target = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in t_addr;
    memset(&t_addr, 0, sizeof(t_addr));
    t_addr.sin_family = AF_INET;
    t_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &t_addr.sin_addr);

    if (connect(fd_target, (struct sockaddr *)&t_addr, sizeof(t_addr)) < 0) {
        perror("Hedef sunucuya baglanilamadi");
        return 1;
    }
    printf(">> Hedef sunucu ile baglanti kuruldu. Veri akisi basliyor...\n");

    char buf[BUF_SIZE];
    close(s_listen); 

    while (1) {
        fd_set read_fds; 
        FD_ZERO(&read_fds);
        FD_SET(fd_client, &read_fds); 
        FD_SET(fd_target, &read_fds); 

        int max_fd = (fd_client > fd_target) ? fd_client : fd_target;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(fd_client, &read_fds)) {
            int n = recv(fd_client, buf, BUF_SIZE, 0); 
            if (n <= 0) break;
            
            inject_noise(buf, n);
            
            if (error_triggered) {
                printf(">> Bozuk paket server'a iletildi, server'in tepkisi bekleniyor...\n");
                error_triggered = 0; 
            }
            send(fd_target, buf, n, 0); 
        }

        if (FD_ISSET(fd_target, &read_fds)) {
            int n = recv(fd_target, buf, BUF_SIZE, 0); 
            if (n <= 0) break;
            send(fd_client, buf, n, 0); 
        }
    }

    close(fd_client);
    close(fd_target);
    printf(">> Tunel kapatildi.\n");
    return 0;
}