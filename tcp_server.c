#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct weaponData{
    int weapon_id;
    char desc[255];    
    double x,y;
    char device_ip[30];
    };


int main()
{
    int sockfd, clientfd, status, opt=1;
    struct sockaddr_in addr, client_addr;
    char text[255];
    socklen_t addr_len = sizeof(client_addr);

    struct weaponData data;

    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd<0){
        perror("socket");
        return sockfd;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    status = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    if(status){
        perror("bind");
        close(sockfd);
        return status;
    }

    status = listen(sockfd, 1);
    if(status){
        perror("listen");
        close(sockfd);
        return status;
    }

    clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
    if (clientfd < 0){
        perror("accept");
        close(sockfd);
        return status;
    }

    printf("Client port: %d, Client IP addr: %s\n", client_addr.sin_port, inet_ntoa(client_addr.sin_addr));

    status = recv(clientfd, &data, sizeof(data), 0);   
    data.device_ip[30] = *inet_ntoa(client_addr.sin_addr);
    if(status){
        printf("Description: %s\n ID: %d\n x parameter: %lf\n y parameter: %lf\n IP address: %d",data.desc, data.weapon_id, data.x, data.y, data.device_ip[30]);
        sprintf(text,"Device added succesfully");
    }

    //sprintf(text, "The server says hello\n");
    send(clientfd, text, strlen(text), 0);

    close(clientfd);
    close(sockfd);
    return 0;
}