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
	if (argc != 4){
		fprintf(stderr , "usage : %s <fichier_a_envoyer> <fichier_recu> <port_local>\n", argv[0]);
		exit(0);
	}

	//Clear console
	system("clear");
	printf("----- SERVEUR UDP -----\n");

	//Déclaration des variables
	int fd;							//Descripteur de la socket
	struct sockaddr_in adr; 		//Adresse distante
	struct sockaddr_in adrLocale; 	//Adresse locale
	int output_fd;					//Descripteur du fichier receveur
	int input_fd;					//Descripteur du fichier à lire
	char buffer[BUFFER_LENGTH];		//Le buffer recevant le message
	char tmpBuffer[BUFFER_LENGTH-1];//buffer - sequence et ack
	char bufToWrite[BUFFER_LENGTH-1];//Le buffer à écrire
	char bufAck[1];					//buffer pour l'acquittement
	char ipRecu[INET_ADDRSTRLEN];	//La chaine contenant l'adresse src
	int nbRecv;						//Check variable
	int nb_lu;						//Check variable
	int reception;					//Check variable
	int emission;					//Check variable
	int seqAck;						//Compteur pour le numéro de sequence/ acquittement
	fd_set rfds;					//Ensemble des descripteur
  	struct timeval timer;			//Timer pour le select

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
	adrLocale.sin_family = AF_INET;					//Protocol
	adrLocale.sin_addr.s_addr = htonl(INADDR_ANY);	//Adresse IP distante
	adrLocale.sin_port = htons(atoi(argv[3]));		//numéro de port choisi
	socklen_t addrlen = sizeof(adrLocale);  	//Longeur de l'adresse
	printf("-> Préparation terminée !\n");

	//Attache de la socket
	printf("\nAttache de la socket ...\n");
	if (bind(fd, (struct sockaddr*)&adrLocale, sizeof(adrLocale)) == -1){
		perror("-> Echec de l'attachement, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de l'attachement !\n");
	}

	//Préparation de l'adresse distante reception d'un datagramme vide
	printf("\nPhase de connexion ...\n");
	if (recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen) == -1){
		perror("-> Echec de la phase de connexion, fin du programme.\n\n");
		exit(0);
	}else{
		inet_ntop(AF_INET ,&adrLocale.sin_addr, ipRecu, INET_ADDRSTRLEN);		//Convertion de l'adresse ip recu
		printf("-> Préparation de l'adresse distante ...\n");
		adr.sin_family = AF_INET;						//Type de protocol
		inet_pton(AF_INET, ipRecu, & (adr.sin_addr)); 	//Adresse ip du client
		adr.sin_port = adrLocale.sin_port;				//Numéro du client
		printf("--> Préparation terminée !\n");
		//Envoi du retour au client
		printf("--> Envoi du retour ...\n");
		if (sendto(fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen) == -1){
			perror("---> Echec de l'envoi, fin du programme.\n");
			exit(0);
		}
	}
	emission = 1;
	reception = 1;
	printf("-> Fin de la phase de connexion.\n");

	//Ouverture du fichier à lire
	printf("\nOuverture du fichier à lire ...\n");
	input_fd = open(argv[1], O_RDONLY);
	if(input_fd == -1){	
		perror("-> Echec de l'ouverture, fin du programme.\n"); 
		exit(0); 		
	}else{
		printf("-> Succes de l'ouverture !\n");
	}

	//Ouverture du fichier recepteur
	printf("\nCréation du fichier receveur ...\n");
	output_fd = open(argv[2], O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
	if(output_fd == -1){	
		perror("-> Echec de la création, fin du programme.\n\n");
		exit(0); 		
	}else{
		printf("-> Succes de la création !\n");
	}

	printf("\nEnvoi et reception des fichiers ...\n");
	seqAck = 0;	//Initialisation du compteur à zero
	while (1 == 1){
		//Initialisation du select
		//Ajout dans la liste du descripteur de la socket
	    FD_ZERO(&rfds);
	    FD_SET(fd, &rfds);
	    int nbFd = fd+1;

	    //Initialisation du timer pour les acquittement
	    timer.tv_sec = 1;
	    timer.tv_usec = 0;

		if (emission == 1){
			//Création du buffer contenant la place pour le numéro de sequence et acquittement
			//nb_lu = read(input_fd, buffer, BUFFER_LENGTH);
			nb_lu = read(input_fd, tmpBuffer, BUFFER_LENGTH-1);
			sprintf(buffer, "%d", seqAck);	//Ajout du numéro de sequence
      		int i;
      		for (i = 0 ; i < BUFFER_LENGTH-1; i++){
      			buffer[i+1] = tmpBuffer[i];
      		}
      		printf("\n");

			if (nb_lu == -1){
				printf("-> Echec de la lecture d'un bloc, fin du programme.\n\n");
			}else{
				printf("-> Envoi d'un bloc (taille : %d) (seq = %c) ...\n",nb_lu, buffer[0]);
				sendto (fd, &buffer, nb_lu+1, 0, (struct sockaddr*) &adr, addrlen);
				if (nb_lu == 0)
					emission = 0;
				//routine d'attente de l'ack
				else{
					while (1 == 1){ //A voir ce que l'on doit faire ici car les deux s'interbloques ...
						//Select pour l'attente d'un message avec res
            			int res = select(nbFd, &rfds, NULL, NULL, &timer);
            			//sleep(2);
            			//Cas de l'erreur du select
            			if (res == -1){
            				perror("-> Erreur dans la reception de l'acquittement, fin de programme.\n\n");
            				exit(1);
            			}//Cas du timeout
            			else if (res == 0){
            				//Renvoie du message
            				printf("-> timeout, renvoi du message ...\n");
            				sendto (fd, &buffer, nb_lu+1, 0, (struct sockaddr*) &adr, addrlen);
            			}//Cas de la reception d'un ack
            			else{
            				//Vérification de l'ack
            				recvfrom(fd, &bufAck, 1, 0, (struct sockaddr*) &adrLocale, &addrlen);
            				printf("-> Reception acquittement %c\n",bufAck[0]);
            				if (bufAck[0] == buffer[0]){	//Si l'ack est le bon
            					printf("--> Acquittement correcte\n");
				                seqAck = (seqAck+1)%2;		//On peux passer à la suite
				                break;
				            }else{	//Sinon on renvoi le buffer
				            	printf("--> Mauvais acquittement renvoi du bloc ...\n");
				                sendto (fd, &buffer, nb_lu+1, 0, (struct sockaddr*) &adr, addrlen);
				            }
            			}

					}
				}
			}
		}
		
		if (reception == 1){
			nbRecv = recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen);
			if (nbRecv == 1){
				reception = 0;
			}
			if ( nbRecv == -1){
				perror("-> Echec de la reception, fin du programme.\n\n");
			}else{
				printf("-> Reception d'un bloc (taille : %d)...\n",nbRecv);
				bufAck[0] = buffer[0];
				printf("-> Envoi de l'acquittement %c\n", bufAck[0]);
				sendto (fd, &bufAck, 1, 0, (struct sockaddr*) &adr, addrlen);
				
				int i;
				for (i = 0; i < BUFFER_LENGTH-1; i++){
					bufToWrite[i] = buffer[i+1];
				}

				write(output_fd, bufToWrite, nbRecv-1);
			}
		}

		if(emission == 0 && reception == 0){
			break;
			printf("-> Fin de reception.\n");
		}
	}
	
	//Fermeture de la socket
	printf("\n\nFermeture de la socket ...\n");
	if (close(fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}

	//Fermeture du fichier
	printf("\nFermeture des fichiers ...\n");
	if (close(input_fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}
	if (close(output_fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}

	printf("\n\nFin du programme.\n\n");

	return 1;
}