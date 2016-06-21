#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>


#define portnumber 8888



int main(int argc, char *argv[]){
	int sockfd, new_fd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	int sin_size;
	char hello[] = "Hello Are you Fine?\n";

	char buffer[2048];

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "Socker error: %s\n", strerror(errno));
		exit(1);
	}

	bzero(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(portnumber);

	if(bind(sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1){
		fprintf(stderr, "Bind error: %s\n\a", strerror(errno));
		exit(1);
	}

	if(listen(sockfd, 5) == -1){
		fprintf(stderr, "Listen error: %s\n\a", strerror(errno));
		exit(1);
	}

	while(1){
		sin_size = sizeof(struct sockaddr_in);
		if((new_fd = accept(sockfd, (struct sockaddr*)(&client_addr),(socklen_t*)&sin_size)) == -1){
			fprintf(stderr, "Accept error: %s\n\a", strerror(errno));
			exit(1);
		}
		fprintf(stderr, "Server get connection from %s: ", inet_ntoa(client_addr.sin_addr));

		int nbytes = read(new_fd, buffer, 1024);
		buffer[nbytes] = '\0';
		fprintf(stderr, "%d bytes. ==> \n", nbytes);
		for(int i = 0; i < nbytes; i ++){
			fprintf(stderr, " %d ", buffer[i]);
		}
		fprintf(stderr, "\n");
		close(new_fd);
	}

	close(sockfd);


	return 0;
}
