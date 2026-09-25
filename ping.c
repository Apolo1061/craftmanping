#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>

void limpiar3(char *texto) {
    int i = 0;
    int j = 0;
    for(i = 0; texto[i] != '\0'; ) {
        if((unsigned char)texto[i] == 0xC2 && (unsigned char)texto[i+1] == 0xA7 && texto[i+2] != '\0') {
            i += 3;
        }
        else if(texto[i] == '&' && texto[i+1] != '\0') {
            i += 2;
        }
        else {
            texto[j++] = texto[i++];
        }
    }
    texto[j] = '\0';
}

static const unsigned char RAKNET_MAGIC[] = {
    0x00, 0xff, 0xff, 0x00, 0xfe, 0xfe, 0xfe, 0xfe,
    0xfd, 0xfd, 0xfd, 0xfd, 0x12, 0x34, 0x56, 0x78
};

void query(char *address) {
    char ip_str[256];
    int port = 19132;

    char *colon = strchr(address, ':');
    if(colon) {
        strncpy(ip_str, address, colon - address);
        ip_str[colon - address] = '\0';
        port = atoi(colon + 1);
    }else {
        strcpy(ip_str, address);
    }

    struct sockaddr_in server_addr;
    struct hostent *he;
    struct timeval tv;
    unsigned char buffer[4096];

    if((he = gethostbyname(ip_str)) == NULL) {
        perror("Gethostbyname: ");
        return;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);

    system("clear");

    printf("    Send packet --> %s:%d \n", ip_str, port);

    unsigned char pkt[33];
    pkt[0] = 0x01;

    uint64_t send_time = (uint64_t)time(NULL) * 1000ULL;
    for(int i = 0; i < 8; i++) {
        pkt[1 + i] = (send_time >> (56 - i*8)) & 0xFF;
    }

    memcpy(&pkt[9], RAKNET_MAGIC, 16);

    srand((unsigned int)time(NULL));
    uint64_t client_guid = ((uint64_t)rand() << 32) | (uint64_t)rand();
    for(int i = 0; i < 8; i++) {
        pkt[25 + i] = (client_guid >> (56 - i*8)) & 0xFF;
    }

    sendto(sock, pkt, sizeof(pkt), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&from_addr, &from_len);
    if (n < 0) {
        fprintf(stderr, "Error: Timeout\n");
        close(sock);
        return;
    }
    buffer[n] = '\0';

    printf("Respuesta de: %s\n", inet_ntoa(from_addr.sin_addr));

    if(buffer[0] != 0x1c) {
        printf("Respuesta invalida\n");
        close(sock);
        return;
    }

    int offset = 1;
    offset += 8;
    uint64_t server_guid = 0;
    for(int i = 0; i < 8; i++) {
        server_guid = (server_guid << 8) | buffer[offset + i];
    }
    offset += 8;
    offset += 16;

    uint16_t str_len = (buffer[offset] << 8) | buffer[offset + 1];
    offset += 2;

    char server_id_str[1024];
    int copy_len = str_len < sizeof(server_id_str) - 1 ? str_len : sizeof(server_id_str) - 1;
    memcpy(server_id_str, &buffer[offset], copy_len);
    server_id_str[copy_len] = '\0';

    char *campos[16];
    int num_campos = 0;
    char *tok = strtok(server_id_str, ";");
    while(tok != NULL && num_campos < 16) {
        campos[num_campos++] = tok;
        tok = strtok(NULL, ";");
    }

    const char *edition   = num_campos > 0 ? campos[0] : "none";
    char *motd            = num_campos > 1 ? campos[1] : "none";
    const char *protocol  = num_campos > 2 ? campos[2] : "none";
    const char *nplayers  = num_campos > 3 ? campos[3] : "0";
    const char *mplayers  = num_campos > 4 ? campos[4] : "0";
    const char *server_id = num_campos > 5 ? campos[5] : "none";
    char *submotd         = num_campos > 6 ? campos[6] : "";
    const char *gamemode  = num_campos > 7 ? campos[7] : "none";
    const char *gm_num    = num_campos > 8 ? campos[8] : "none";

    limpiar3(motd);
    if(strlen(submotd)) limpiar3(submotd);

    printf("Edition: %s\n", edition);
    printf("MOTD: %s\n", motd);
    if(strlen(submotd))
        printf("Sub-MOTD: %s\n", submotd);
    printf("Protocolo: %s\n", protocol);
    printf("Jugadores: %s/%s\n", nplayers, mplayers);
    if(strcmp(gamemode, "none") != 0 || strcmp(gm_num, "none") != 0)
        printf("Gamemode: %s (%s)\n", gamemode, gm_num);
    printf("Server GUID: %llu\n", (unsigned long long)server_guid);
    if(strcmp(server_id, "none") != 0)
        printf("Server ID: %s\n", server_id);

    close(sock);
}

int main() {
    char data[256];
    printf("Se debe usar como IP:PORT (1.1.1.1:19132)\n");
    printf("> ");
    if(scanf("%255s", data) == 1) {
        query(data);
    }

    return 0;
}
