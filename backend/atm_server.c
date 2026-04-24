#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

typedef struct {
    int cardNo;
    int pin;
    char name[100];
    float balance;
    char status[10];
} User;

typedef struct {
    int cardNo;
    char type[20];
    float amount;
    float balanceAfter;
    char date[30];
} Transaction;

User users[1000];
Transaction transactions[10000];
int userCount = 0;
int transCount = 0;

// Function declarations
void saveUsers();
void loadUsers();
void loadTransactions();
void saveTransactions();
void addTransaction(int cardNo, const char* type, float amount, float balanceAfter);
int findUser(int cardNo, int pin);
int findUserByCard(int cardNo);
void sendResponse(SOCKET client, const char* json);
void handle(SOCKET client, char* req);

void loadUsers() {
    FILE *f = fopen("../data/users.txt", "r");
    if (!f) {
        // Create fresh users
        userCount = 3;
        users[0] = (User){1001, 101, "Muhammad Talha", 50000.00, "Active"};
        users[1] = (User){1002, 91, "Farwah Shakeel", 75000.00, "Active"};
        users[2] = (User){1003, 100, "Abeera Naim", 30000.00, "Active"};
        saveUsers();
        printf("Created new users file!\n");
        return;
    }
    
    fscanf(f, "%d\n", &userCount);
    for(int i = 0; i < userCount; i++) {
        fscanf(f, "%d|%d|%[^|]|%f|%[^\n]\n", 
               &users[i].cardNo, &users[i].pin, users[i].name,
               &users[i].balance, users[i].status);
        printf("Loaded: %d - %s - Balance: %.2f\n", users[i].cardNo, users[i].name, users[i].balance);
    }
    fclose(f);
}

void saveUsers() {
    FILE *f = fopen("../data/users.txt", "w");
    fprintf(f, "%d\n", userCount);
    for(int i = 0; i < userCount; i++) {
        fprintf(f, "%d|%d|%s|%.2f|%s\n", 
                users[i].cardNo, users[i].pin, users[i].name,
                users[i].balance, users[i].status);
    }
    fclose(f);
    printf("Saved users data!\n");
}

void loadTransactions() {
    FILE *f = fopen("../data/transactions.txt", "r");
    if (!f) return;
    
    fscanf(f, "%d\n", &transCount);
    for(int i = 0; i < transCount; i++) {
        fscanf(f, "%d|%[^|]|%f|%f|%[^\n]\n", 
               &transactions[i].cardNo, transactions[i].type,
               &transactions[i].amount, &transactions[i].balanceAfter,
               transactions[i].date);
    }
    fclose(f);
}

void saveTransactions() {
    FILE *f = fopen("../data/transactions.txt", "w");
    fprintf(f, "%d\n", transCount);
    for(int i = 0; i < transCount; i++) {
        fprintf(f, "%d|%s|%.2f|%.2f|%s\n", 
                transactions[i].cardNo, transactions[i].type,
                transactions[i].amount, transactions[i].balanceAfter,
                transactions[i].date);
    }
    fclose(f);
}

void addTransaction(int cardNo, const char* type, float amount, float balanceAfter) {
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    char date[30];
    sprintf(date, "%02d/%02d/%d %02d:%02d:%02d",
            local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
            local->tm_hour, local->tm_min, local->tm_sec);
    
    transactions[transCount].cardNo = cardNo;
    strcpy(transactions[transCount].type, type);
    transactions[transCount].amount = amount;
    transactions[transCount].balanceAfter = balanceAfter;
    strcpy(transactions[transCount].date, date);
    transCount++;
    saveTransactions();
}

int findUser(int cardNo, int pin) {
    for(int i = 0; i < userCount; i++) {
        if(users[i].cardNo == cardNo && users[i].pin == pin && strcmp(users[i].status, "Active") == 0) {
            printf("Found user: %s (Card: %d)\n", users[i].name, users[i].cardNo);
            return i;
        }
    }
    printf("User not found: Card=%d, PIN=%d\n", cardNo, pin);
    return -1;
}

int findUserByCard(int cardNo) {
    for(int i = 0; i < userCount; i++) {
        if(users[i].cardNo == cardNo && strcmp(users[i].status, "Active") == 0) {
            printf("Found user by card: %s (Card: %d)\n", users[i].name, users[i].cardNo);
            return i;
        }
    }
    printf("User not found by card: %d\n", cardNo);
    return -1;
}

void sendResponse(SOCKET client, const char* json) {
    char response[32768];
    sprintf(response,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n%s",
        (int)strlen(json), json);
    send(client, response, strlen(response), 0);
}

void handle(SOCKET client, char* req) {
    if (strncmp(req, "OPTIONS", 7) == 0) {
        char response[] =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
        send(client, response, strlen(response), 0);
        return;
    }

    char method[10], path[256];
    sscanf(req, "%s %s", method, path);
    
    printf("\n=== New Request ===\n");
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);

    if(strcmp(method, "POST") == 0 && strcmp(path, "/api/login") == 0) {
        char* body = strstr(req, "\r\n\r\n");
        if(body) {
            body += 4;
            int cardNo = 0, pin = 0;
            sscanf(body, "{\"cardNo\":%d,\"pin\":%d}", &cardNo, &pin);
            printf("Login attempt - Card: %d, PIN: %d\n", cardNo, pin);
            
            int idx = findUser(cardNo, pin);
            if(idx != -1) {
                char json[512];
                sprintf(json, "{\"success\":true,\"name\":\"%s\",\"cardNo\":%d,\"balance\":%.2f}",
                        users[idx].name, users[idx].cardNo, users[idx].balance);
                printf("Login successful: %s\n", users[idx].name);
                sendResponse(client, json);
            } else {
                sendResponse(client, "{\"success\":false,\"message\":\"Invalid card number or PIN\"}");
                printf("Login failed!\n");
            }
        }
    }
    
    else if(strcmp(method, "GET") == 0 && strncmp(path, "/api/balance", 12) == 0) {
        int cardNo = 0;
        char* q = strstr(path, "?card=");
        if(q) cardNo = atoi(q + 6);
        printf("Balance check - Card: %d\n", cardNo);
        
        int idx = findUserByCard(cardNo);
        if(idx != -1) {
            char json[256];
            sprintf(json, "{\"success\":true,\"balance\":%.2f}", users[idx].balance);
            printf("Balance: %.2f for %s\n", users[idx].balance, users[idx].name);
            sendResponse(client, json);
        } else {
            sendResponse(client, "{\"success\":false,\"message\":\"Account not found\"}");
        }
    }
    
    else if(strcmp(method, "POST") == 0 && strcmp(path, "/api/withdraw") == 0) {
        char* body = strstr(req, "\r\n\r\n");
        if(body) {
            body += 4;
            int cardNo = 0;
            float amount = 0;
            sscanf(body, "{\"cardNo\":%d,\"amount\":%f}", &cardNo, &amount);
            printf("Withdraw - Card: %d, Amount: %.2f\n", cardNo, amount);
            
            int idx = findUserByCard(cardNo);
            if(idx != -1) {
                if(amount <= 0) {
                    sendResponse(client, "{\"success\":false,\"message\":\"Invalid amount\"}");
                    return;
                }
                if(amount > users[idx].balance) {
                    sendResponse(client, "{\"success\":false,\"message\":\"Insufficient balance\"}");
                    return;
                }
                if(amount > 25000) {
                    sendResponse(client, "{\"success\":false,\"message\":\"Daily limit is ₹25,000\"}");
                    return;
                }
                
                users[idx].balance -= amount;
                saveUsers();
                addTransaction(cardNo, "Withdraw", amount, users[idx].balance);
                
                char json[256];
                sprintf(json, "{\"success\":true,\"newBalance\":%.2f,\"message\":\"Withdrawal successful\"}", users[idx].balance);
                printf("Withdrawal successful! New balance: %.2f for %s\n", users[idx].balance, users[idx].name);
                sendResponse(client, json);
                return;
            }
            sendResponse(client, "{\"success\":false,\"message\":\"Account not found\"}");
        }
    }
    
    else if(strcmp(method, "POST") == 0 && strcmp(path, "/api/deposit") == 0) {
        char* body = strstr(req, "\r\n\r\n");
        if(body) {
            body += 4;
            int cardNo = 0;
            float amount = 0;
            sscanf(body, "{\"cardNo\":%d,\"amount\":%f}", &cardNo, &amount);
            printf("Deposit - Card: %d, Amount: %.2f\n", cardNo, amount);
            
            int idx = findUserByCard(cardNo);
            if(idx != -1) {
                if(amount <= 0) {
                    sendResponse(client, "{\"success\":false,\"message\":\"Invalid amount\"}");
                    return;
                }
                if(amount > 50000) {
                    sendResponse(client, "{\"success\":false,\"message\":\"Max deposit per transaction is ₹50,000\"}");
                    return;
                }
                
                users[idx].balance += amount;
                saveUsers();
                addTransaction(cardNo, "Deposit", amount, users[idx].balance);
                
                char json[256];
                sprintf(json, "{\"success\":true,\"newBalance\":%.2f,\"message\":\"Deposit successful\"}", users[idx].balance);
                printf("Deposit successful! New balance: %.2f for %s\n", users[idx].balance, users[idx].name);
                sendResponse(client, json);
                return;
            }
            sendResponse(client, "{\"success\":false,\"message\":\"Account not found\"}");
        }
    }
    
    else if(strcmp(method, "GET") == 0 && strncmp(path, "/api/history", 12) == 0) {
        int cardNo = 0;
        char* q = strstr(path, "?card=");
        if(q) cardNo = atoi(q + 6);
        printf("History request - Card: %d\n", cardNo);
        
        char json[131072] = "{\"transactions\":[";
        int first = 1;
        
        for(int i = transCount - 1; i >= 0; i--) {
            if(transactions[i].cardNo == cardNo) {
                if(!first) strcat(json, ",");
                char trans[1024];
                sprintf(trans,
                    "{\"type\":\"%s\",\"amount\":%.2f,\"balanceAfter\":%.2f,\"date\":\"%s\"}",
                    transactions[i].type, transactions[i].amount, 
                    transactions[i].balanceAfter, transactions[i].date);
                strcat(json, trans);
                first = 0;
            }
        }
        strcat(json, "]}");
        sendResponse(client, json);
    }
    
    else if(strcmp(method, "POST") == 0 && strcmp(path, "/api/changepin") == 0) {
        char* body = strstr(req, "\r\n\r\n");
        if(body) {
            body += 4;
            int cardNo = 0, oldPin = 0, newPin = 0;
            sscanf(body, "{\"cardNo\":%d,\"oldPin\":%d,\"newPin\":%d}", &cardNo, &oldPin, &newPin);
            
            int idx = findUser(cardNo, oldPin);
            if(idx != -1) {
                if(newPin < 0 || newPin > 9999) {
                    sendResponse(client, "{\"success\":false,\"message\":\"PIN must be 4 digits\"}");
                    return;
                }
                users[idx].pin = newPin;
                saveUsers();
                addTransaction(cardNo, "PIN Changed", 0, users[idx].balance);
                sendResponse(client, "{\"success\":true,\"message\":\"PIN changed successfully\"}");
                return;
            }
            sendResponse(client, "{\"success\":false,\"message\":\"Invalid card number or PIN\"}");
        }
    }
    
    else {
        sendResponse(client, "{\"error\":\"Not found\"}");
    }
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    
    system("mkdir ..\\data 2>nul");
    
    loadUsers();
    loadTransactions();

    printf("\n========================================\n");
    printf("     ATM SIMULATION SERVER RUNNING!\n");
    printf("     Port: %d\n", PORT);
    printf("========================================\n");
    printf("     Default Accounts:\n");
    printf("     Card: 1001 | PIN: 101 | Muhammad Talha (₹50000)\n");
    printf("     Card: 1002 | PIN: 91  | Farwah Shakeel (₹75000)\n");
    printf("     Card: 1003 | PIN: 100 | Abeera Naim (₹30000)\n");
    printf("========================================\n");
    printf("     ✅ Server Ready!\n");
    printf("     Press Ctrl+C to stop\n");
    printf("========================================\n\n");

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if(bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("ERROR: Port %d is busy! Close other programs and try again.\n", PORT);
        printf("Press any key to exit...\n");
        getchar();
        return 1;
    }

    listen(server, 5);
    printf("🌐 Server listening on http://localhost:%d\n", PORT);
    printf("📱 Open frontend/index.html in browser\n\n");

    while(1) {
        struct sockaddr_in client;
        int len = sizeof(client);
        SOCKET clientSocket = accept(server, (struct sockaddr*)&client, &len);

        if(clientSocket != INVALID_SOCKET) {
            char buffer[32768];
            int bytes = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
            if(bytes > 0) {
                buffer[bytes] = '\0';
                handle(clientSocket, buffer);
            }
            closesocket(clientSocket);
        }
    }

    closesocket(server);
    WSACleanup();
    return 0;
}