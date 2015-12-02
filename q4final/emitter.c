#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_LENGTH 1024

int main(int argc, char* argv[]){

  //Controle des arguments du programme
  if (argc != 6){
    fprintf(stderr , "usage : %s <fichier_a_envoyer> <fichier_recu> <adr_IP_dist> <port_dist> [<port_local>]\n", argv[0]);
    exit(0);
  }

  //Clear console
  system("clear");
  printf("----- CLIENT UDP -----\n");

  //Déclaration des variables
  int fd;							//Descripteur du socket
  struct sockaddr_in adr; 		//Adresse distante
  struct sockaddr_in adrLocale;	//Adresse locale
  char buffer[BUFFER_LENGTH];		//buffer
  char tmpEmBuff[BUFFER_LENGTH-2];//buffer
  char tmpRecBuff[BUFFER_LENGTH-2];//buffer
  int output_fd;					//Descripteur du fichier receveur
  int input_fd;					//Descripteur du fichier à lire
  int nbRecv;						//Check variable
  int nb_lu;						//Check variable
  int reception;					//Check variable
  int emission;					//Check variable
  int connected;					//Check variable
  fd_set rfds;					//Ensemble des descripteur
  struct timeval timer;			//Timer pour le select
  int seq;						//Numéro de sequence
  int ack;						//Numéro d'acquittement
  int i;
  int readable;         //POur savoir si on change le buffer ou non 

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
  adrLocale.sin_port = htons(atoi(argv[5]));		//numéro de port choisi
  socklen_t addrlen = sizeof(adr);  		//longueur de l'adresse
  printf("-> Préparation terminée !\n");

  //Attache de la socket
  printf("\nAttache de la socket ...\n");
  if (bind(fd, (struct sockaddr*)&adrLocale, sizeof(adrLocale)) == -1){
    perror("-> Echec de l'attachement, fin du programme.\n\n");
    exit(0);
  }else{
    printf("-> Succes de l'attachement !\n");
  }

  //Préparation de l'adresse distante
  printf("\nPréparation de l'adresse distante ...\n");
  adr.sin_family = AF_INET;						//Type de protocol
  inet_pton(AF_INET, argv[3], & (adr.sin_addr)); 	//Adresse ip du serveur
  adr.sin_port = htons(atoi(argv[4]));			//numéro de port choisi
  printf("-> Préparation terminée !\n");

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

  //Envoi et reception des fichiers
  printf("\nDébut des échanges ...\n");

  //Envoi d'un datagramme de connection vide
  printf("\nPhase de connexion ...\n");
  connected = 0;
  if(sendto(fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen) == -1){
    perror("-> Echec de la phase de connexion, fin du programme.\n\n");
    exit(0);
  }else{
    //Attente du retour du serveur
    printf("-> Attente du retour serveur ...\n");

     while (1 == 1){
        //Ajout dans la liste du descripteur de la socket
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        //Initialisation du timer pour les acquittement
        timer.tv_sec = 1;
        timer.tv_usec = 0;

        int nbFd = fd+1;

        int res = select(nbFd, &rfds, NULL, NULL, &timer);

        //Cas de l'erreur du select
      if (res == -1){
        perror("-> Erreur dans la reception du retour serveur, fin de programme.\n\n");
        exit(1);
      }//Cas du timeout
      else if (res == 0){
        //Renvoie du message
        printf("-> timeout, renvoi du message vide ...\n");
        sendto (fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen);
      }//Cas de la reception d'un ack
      else{
        if (recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen) == -1){
          perror("-> Echec de la phase de connexion, fin du programme.\n\n");
          exit(0);
        }else{
          printf("-> Succes de la connexion !\n");
          connected = 1;
          break;
        }
      }
    }
  }
  emission = 1;
  reception = 1;
  readable = 1;
  seq = 0;
  ack = 0;
  printf("-> Fin de la phase de connexion.\n");

  while (1 == 1){
    //Ajout dans la liste du descripteur de la socket
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    //Initialisation du timer pour les acquittement
    timer.tv_sec = 0;
    timer.tv_usec = 100000;

    int nbFd = fd+1;

    if (connected == 1){
      //Préparation de l'envoi
      if (readable == 1){
        nb_lu = read(input_fd, tmpEmBuff, BUFFER_LENGTH-2);

        for (i = 0 ; i < BUFFER_LENGTH-2 ; i++){
          buffer[i+2] = tmpEmBuff[i];
        }
      }
      

      if (nb_lu == -1){
        printf("-> Echec de la lecture d'un bloc, fin du programme.\n\n");
      }else{
        buffer[0] = seq+'0';
        buffer[1] = ack+'0';
        printf("-> Envoi d'un bloc (taille : %d, seq : %c, ack : %c)...\n",nb_lu+2, buffer[0], buffer[1]);
        sendto (fd, &buffer, nb_lu+2, 0, (struct sockaddr*) &adr, addrlen);
        
        if (readable == 1)  
          seq = (seq+1)%2;
        if (nb_lu == 0){
          printf("-> Envoi terminé\n");
          emission = 0;
        }
      }
    }

    int res = select(nbFd, &rfds, NULL, NULL, &timer);

    if (res == -1){
      perror("-> Erreur dans la reception du retour serveur, fin de programme.\n\n");
      exit(1);
    }//Cas du timeout
    else if (res == 0){
      //Renvoie du message
      printf("-> TIMEOUT, renvoi du message  (taille : %d, seq : %c)\n",nb_lu+2, buffer[0]);
      readable = 0;
    }//Cas de la reception d'un ack
    else{
        nbRecv = recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen);
        //Récupération du buffer utile
        for (i = 0 ; i < BUFFER_LENGTH-2 ; i++){
          tmpRecBuff[i] = buffer[i+2];
        }

        if (nbRecv == 0 && connected == 0){
          printf("-> Renvoi du retour serveur ...\n");
          sendto(fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen);
        }

        if (nbRecv == 2 && connected == 1){
          printf("-> Reception termné.\n");
          reception = 0;
        }
        if ( nbRecv == -1){
          perror("-> Echec de la reception, fin du programme.\n\n");
        }else if (nbRecv > 0){
          printf("\x1b[34m-> Reception d'un bloc (taille : %d, seq : %c, ack : %c) \x1b[37m\n",nbRecv-2, buffer[0], buffer[1]);
          
          
          if ((buffer[0]-'0') == ack){  //Cas bon
            ack = (ack+1)%2;
            write(output_fd, tmpRecBuff, nbRecv-2);
            readable = 1;
          }else{
            readable = 0;
          }

          //Ouverture de l'envoi si on reçois une tram > 0
          if (connected == 0){
            printf("-> Succes de la connexion !\n");
            printf("-> Fin de la phase de connexion.\n");
            connected = 1;
          }

          
        }
    }    

    if(emission == 0 && reception == 0){
      buffer[0] = seq+'0';
      buffer[1] = ack+'0';
      printf("-> Envoi d'un bloc (taille : %d, seq : %c, ack : %c)...\n",nb_lu, buffer[0], buffer[1]);
      sendto (fd, &buffer, nb_lu+2, 0, (struct sockaddr*) &adr, addrlen);
      printf("-> Fin de communication.\n");
      break;
    }
    //sleep(3);
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
