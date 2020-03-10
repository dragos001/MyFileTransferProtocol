#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

#define MAXIM 500

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

char *password(char *pass)
{
  for (int i = 0; i < strlen(pass); i++)
    pass[i] += 10;

  return pass;
}

int main(int argc, char *argv[])
{
  int sd;                    // descriptorul de socket
  struct sockaddr_in server; // structura folosita pentru conectare
                             // mesajul trimis
  char mesaj[MAXIM];
  char raspuns[MAXIM];

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
  {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  /* stabilim portul */
  port = atoi(argv[2]);

  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons(port);

  /* ne conectam la server */
  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect().\n");
    return errno;
  }
  printf("Foloseste comanda \033[0;32m login\033[0m pentru a te conecta!\n");
  do
  {
    printf("\nComanda: ");
    fflush(stdout);
    bzero(mesaj, MAXIM);
    if (read(0, mesaj, MAXIM) < 0)
    {
      perror("[clinet]Eroare la read() de la server \n");
      close(sd);
      return errno;
    }
    mesaj[strlen(mesaj) - 1] = 0; //scoatem new line
    if (strcmp(mesaj, "help") == 0)
    {
      if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
      {
        perror("[client]Eroare la write() de la server.\n");
        close(sd);
        return errno;
      }
      bzero(raspuns, MAXIM);
      if (read(sd, raspuns, MAXIM) < 0)
      {
        perror("[clinet]Eroare la read() de la server \n");
        close(sd);
        return errno;
      }
      printf("%s\n", raspuns);
    }
    else if (strcmp(mesaj, "quit") == 0)
    {
      if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
      {
        perror("[client]Eroare la write() de la server.\n");
        close(sd);
        return errno;
      }
      close(sd);
      return 0;
    }
    else if (strcmp(mesaj, "login") == 0)
    {
      if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
      {
        perror("[client]Eroare la write() de la server.\n");
        close(sd);
        return errno;
      }

      if (read(sd, raspuns, MAXIM) < 0)
      {
        perror("[clinet]Eroare la read() de la server \n");
        close(sd);
        return errno;
      }
      if (strcmp(raspuns, "user:") == 0)
      {
        char utilizator[30] = "";
        printf("%s ", raspuns);

        fflush(stdout);
        bzero(mesaj, MAXIM);
        if (read(0, mesaj, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }
        mesaj[strlen(mesaj) - 1] = 0; //scoatem new line
        strcat(utilizator, mesaj);
        strcat(utilizator, ":");
        printf("Password: ");

        fflush(stdout);
        bzero(mesaj, MAXIM);
        if (read(0, mesaj, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }
        mesaj[strlen(mesaj) - 1] = 0; //scoatem new line
        strcat(utilizator, password(mesaj));
        if (write(sd, utilizator, strlen(utilizator) + 1) <= 0)
        {
          perror("[client]Eroare la write() de la server.\n");
          close(sd);
          return errno;
        }
        if (read(sd, raspuns, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }

        printf("%s\n", raspuns);
      }
      else
      {

        printf("Esti deja conectat!\n");
      }
    }
    else if (strcmp(mesaj, "show") == 0)
    {
      if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
      {
        perror("[client]Eroare la write() de la server.\n");
        close(sd);
        return errno;
      }
      bzero(raspuns, MAXIM);
      if (read(sd, raspuns, MAXIM) <= 0)
      {
        perror("[client]Eroare la read() de la server.\n");
        close(sd);
        return errno;
      }
      printf("%s\n", raspuns);
    }
    else if (strcmp(mesaj, "schimba") == 0)
    {
      if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
      {
        perror("[client]Eroare la write() de la server.\n");
        close(sd);
        return errno;
      }
      bzero(raspuns, MAXIM);
      if (read(sd, raspuns, MAXIM) <= 0)
      {
        perror("[client]Eroare la read() de la server.\n");
        close(sd);
        return errno;
      }

      if (strcmp(raspuns, "Error1") == 0)
      {
        printf("Trebuie sa fii logat pentru a avea acces la comanda!\n");
      }
      else
      {
        char fisier[100] = "";

        fflush(stdout);
        bzero(mesaj, MAXIM);
        printf("\n Ce fisier vrei sa redenumesti? :\n");
        if (read(0, mesaj, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }
        mesaj[strlen(mesaj) - 1] = 0; //scoatem new line
        strcat(fisier, mesaj);
        strcat(fisier, ":");
        printf("In ce vrei sa schimbi fisierul: \n");

        fflush(stdout);
        bzero(mesaj, MAXIM);
        if (read(0, mesaj, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }
        mesaj[strlen(mesaj) - 1] = 0; //scoatem new line
        strcat(fisier, mesaj);
        if (write(sd, fisier, strlen(fisier) + 1) <= 0)
        {
          perror("[client]Eroare la write() de la server.\n");
          close(sd);
          return errno;
        }
        if (read(sd, raspuns, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }
        printf("%s", raspuns);
      }
    }
    else if (strcmp("sterge", mesaj) == 0)
    {
      if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
      {
        perror("[client]Eroare la write() de la server.\n");
        close(sd);
        return errno;
      }
      bzero(raspuns, MAXIM);
      if (read(sd, raspuns, MAXIM) <= 0)
      {
        perror("[client]Eroare la read() de la server.\n");
        close(sd);
        return errno;
      }

      if (strcmp(raspuns, "Error1") == 0)
      {
        printf("Trebuie sa fii logat pentru a avea acces la comanda!\n");
      }
      else
      {

        fflush(stdout);
        bzero(mesaj, MAXIM);
        printf("\n Ce fisier vrei sa stergi? :\n");
        if (read(0, mesaj, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }
        mesaj[strlen(mesaj) - 1] = 0;
        if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
        {
          perror("[client]Eroare la write() de la server.\n");
          close(sd);
          return errno;
        }
        bzero(raspuns, MAXIM);
        if (read(sd, raspuns, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }
        printf("%s", raspuns);
      }
    }
    else if (strcmp("descarca", mesaj) == 0)
    {
      if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
      {
        perror("[client]Eroare la write() de la server.\n");
        close(sd);
        return errno;
      }
      bzero(raspuns, MAXIM);
      if (read(sd, raspuns, MAXIM) <= 0)
      {
        perror("[client]Eroare la read() de la server.\n");
        close(sd);
        return errno;
      }

      if (strcmp(raspuns, "Error1") == 0)
      {
        printf("Trebuie sa fii logat pentru a avea acces la comanda!\n");
      }
      else
      {

        fflush(stdout);
        bzero(mesaj, MAXIM);
        printf("\n Ce fisier vrei sa descarci? :\n");
        if (read(0, mesaj, MAXIM) < 0)
        {
          perror("[clinet]Eroare la read() de la server \n");
          close(sd);
          return errno;
        }
        fflush(stdout);
        mesaj[strlen(mesaj) - 1] = 0;
        if (write(sd, mesaj, strlen(mesaj) + 1) <= 0)
        {
          perror("[client]Eroare la write() de la server.\n");
          close(sd);
          return errno;
        }

        if (read(sd, raspuns, MAXIM) <= 0)
        {
          perror("[client]Eroare la read() de la server.\n");
          close(sd);
          return errno;
        }

        if (strcmp("OK", raspuns) == 0)
        {

          char pack[100];
          strcpy(pack, "/home/dragos/Desktop/");
          strcat(pack, mesaj);
          char buff[MAXIM];
          FILE *fp;
          fp = fopen(pack, "w");
          if (fp == NULL)
          {
            printf("Error opening file!");
          }
          else
          {
            read(sd, buff, MAXIM);
            fprintf(fp, "%s", buff);
          }
        }
        else
        {
          printf("%s", raspuns);
        }
      }
    }
    else
    {
      printf("Comanda nu exista! Foloseste comanda \033[0;32m help\033[0m pentru a vedea comenzile\n");
    }
  } while (strcmp(mesaj, "quit") != 0);

  close(sd);

  return 0;
}
