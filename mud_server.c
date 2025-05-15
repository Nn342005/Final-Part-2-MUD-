#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 12345
#define MAXLINE 1024

typedef struct Room {
    char* name;
    char* description;
    int north, south, east, west;
    int hasItem;
} Room;

Room rooms[10];
int currentRoom = 0;

void initRooms() {
    rooms[0] = (Room){ "Cave Entrance", "You're at the entrance of a dark cave.", -1, 1, -1, -1, 0 };
    rooms[1] = (Room){ "Hallway", "A narrow hallway with torches.", 0, 2, -1, -1, 0 };
    rooms[2] = (Room){ "Forked Path", "You see paths to the east and west.", -1, -1, 3, 4, 0 };
    rooms[3] = (Room){ "Treasure Room", "You found the item! A shiny golden key.", -1, -1, -1, 2, 1 };
    rooms[4] = (Room){ "Trap Room", "A trap! Nothing here.", -1, -1, -1, -1, 0 };
    rooms[5] = (Room){ "Old Library", "Dusty bookshelves and cobwebs.", -1, 6, -1, -1, 0 };
    rooms[6] = (Room){ "Dining Hall", "Broken tables and rotting food.", 5, 7, -1, -1, 0 };
    rooms[7] = (Room){ "Dungeon Cell", "Rusty bars and chains. Spooky!", 6, 8, -1, -1, 0 };
    rooms[8] = (Room){ "Hidden Alcove", "You feel a chill. Something watches you.", 7, 9, -1, -1, 0 };
    rooms[9] = (Room){ "Connector", "This room links to deeper parts of the dungeon.", 8, -1, -1, -1, 0 };
}

void sendMQTT(const char* message) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mosquitto_pub -h localhost -t mud/room -m \"%s\"", message);
    system(cmd);
}

void movePlayer(char direction) {
    int nextRoom = -1;
    switch (direction) {
        case 'N': case 'n': nextRoom = rooms[currentRoom].north; break;
        case 'S': case 's': nextRoom = rooms[currentRoom].south; break;
        case 'E': case 'e': nextRoom = rooms[currentRoom].east; break;
        case 'W': case 'w': nextRoom = rooms[currentRoom].west; break;
    }

    if (nextRoom != -1) {
        currentRoom = nextRoom;
        sendMQTT(rooms[currentRoom].description);
        if (rooms[currentRoom].hasItem) {
            sendMQTT("\xf0\x9f\x8e\x89 You found the item! Game over!");
        }
    } else {
        sendMQTT("\xe2\x9d\x8c You can't go that way.");
    }
}

int main() {
    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;

    initRooms();
    currentRoom = 0; // Start in Cave Entrance
    sendMQTT(rooms[currentRoom].description);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    socklen_t len = sizeof(cliaddr);
    while (1) {
        int n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Received command: %s\n", buffer);
            movePlayer(buffer[0]);
        }
    }

    close(sockfd);
    return 0;
}
