#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_LENGTH 1024

int main(int argc, char* argv[]){

	//controle des arguments
	if (argc != 3){
		fprintf(stderr , "usage : %s <fichier_recu> <port_local>\n", argv[0]);
		exit(0);
	}

	//Clear console
	system("clear");
	printf("----- SERVEUR UDP -----\n");

	//Déclaration des variables
	int fd;							//Descripteur de la socket
	struct sockaddr_in adr; 		//Adresse distante
	int output_fd;					//Descripteur du fichier receveur
	char buffer[BUFFER_LENGTH];		//Le buffer recevant le message
	char ipRecu[INET_ADDRSTRLEN];	//La chaine contenant l'adresse src
	int nbRecv;						//Check variable

	//Création du socket
	printf("\nCréation du socket ...\n");
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1){
		perror("-> Echec de la création, fin du programme.\n\n");
		exit(0); 
	}else{
		printf("-> Succes de la création !\n");
	}

	//Préparation de l'adresse locale
	printf("\nPréparation de l'adresse locale ...\n");
	adr.sin_family = AF_INET;					//Protocol
	adr.sin_addr.s_addr = htonl(INADDR_ANY);	//Adresse IP distante
	adr.sin_port = htons(atoi(argv[2]));		//numéro de port choisi
	printf("-> Préparation terminée !\n");

	//Attache de la socket
	printf("\nAttache de la socket ...\n");
	if (bind(fd, (struct sockaddr*)&adr, sizeof(adr)) == -1){
		perror("-> Echec de l'attachement, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de l'attachement !\n");
	}

	//Ouverture du fichier
	printf("\nCréation du fichier receveur ...\n");
	output_fd = open(argv[1], O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
	if(output_fd == -1){	
		perror("-> Echec de la création, fin du programme.\n\n");
		exit(0); 		
	}else{
		printf("-> Succes de la création !\n");
	}

	//Reception du fichier
	socklen_t addrlen = sizeof(adr);  	//Longeur de l'adresse
	printf("\nReception du fichier ...\n");
	while (1 == 1){
		nbRecv = recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adr, &addrlen);
		if ( nbRecv == -1){
			perror("-> Echec de la reception, fin du programme.\n\n");
		}else{
			printf("-> Reception d'un bloc (taille : %d)...\n",(int)sizeof(buffer));
			write(output_fd, buffer, nbRecv);
			if (nbRecv != BUFFER_LENGTH){
				printf("-> Fin de la reception");
				break;		
			}
		}
	}
	inet_ntop(AF_INET ,&adr.sin_addr, ipRecu, INET_ADDRSTRLEN);		//Convertion de l'adresse ip recu
	printf("\nLe fichier à été entierement tranféré de la part de : %s : %d\n\n",ipRecu, ntohs(adr.sin_port));
	
	//Fermeture de la socket
	printf("\nFermeture de la socket ...\n");
	if (close(fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}

	//Fermeture du fichier
	printf("\nFermeture du fichier ...\n");
	if (close(output_fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}



	printf("\nFin du programme.\n\n");

	return 1;
}