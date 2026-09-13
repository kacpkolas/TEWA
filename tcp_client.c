#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"


int main()
{
    int sockfd, status;
    struct sockaddr_in addr;

    char text[255];
    
    struct weaponData data;

//tworzenie socketu
    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd<0){
        perror("socket");
        return sockfd;
    }
//ustawianie adresu portu
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    inet_aton("127.0.0.1", &addr.sin_addr);
//polaczenie z serwerem
    status = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    if(status){
        perror("connect");
        close(sockfd);
        return status;
    }
    //sprintf(text, "Hello from the client\n Put your device's data here:\nx=");
    printf("%s","Put your device's data here:\ndescription=\n");
    scanf("%s", data.desc);
    printf("%s", "id=\n");
    scanf("%d",&data.weapon_id);
    printf("%s", "x=\n");
    scanf("%lf", &data.x);
    printf("%s","y=\n");
    scanf("%lf", &data.y);

    //wysylanie do serwera
   // send(sockfd, text, strlen(text), 0);
    
    send(sockfd, &data, sizeof(data),0); //dane o urzadzeniu
//odbieranie z serwera
    status = recv(sockfd, text, sizeof(text), 0);
    if(status){
        printf("%s", text);
    }

    close(sockfd);
    return 0;
}