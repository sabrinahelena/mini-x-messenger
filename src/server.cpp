#include "../include/message.hpp"
#include <iostream>
#include <vector>
#include <winsock2.h> // Deve vir antes de windows.h
#include <ws2tcpip.h>
#include <windows.h>
#include <cstring>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 20
#define MAX_EXIBIDORES 10
#define MAX_ENVIADORES 10

int client_sockets[MAX_CLIENTS];
int client_ids[MAX_CLIENTS];
int total_clients = 0;

int exibidor_sockets[MAX_EXIBIDORES];
int exibidor_ids[MAX_EXIBIDORES];
int total_exibidores = 0;

int enviador_sockets[MAX_ENVIADORES];
int enviador_ids[MAX_ENVIADORES];
int total_enviadores = 0;

int periodic_flag = 0;
time_t start_time;
HANDLE hTimer = NULL;

// Callback do timer do Windows
VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    periodic_flag = 1;
}

void broadcast_info() {
    Message msg{};
    msg.type = 2;
    msg.orig_uid = 0;  // servidor
    msg.dest_uid = 0;
    time_t now = time(nullptr);
    int elapsed = static_cast<int>(now - start_time);
    std::string info = "Servidor ativo. Exibidores: " + std::to_string(total_exibidores) + ", uptime: " + std::to_string(elapsed) + "s";
    strncpy_s(msg.text, info.c_str(), 140);
    msg.text_len = strlen(msg.text);
    char buffer[sizeof(Message)];
    msg.serialize(buffer);
    for (int i = 0; i < total_exibidores; ++i) {
        if (exibidor_sockets[i] != INVALID_SOCKET) {
            send(exibidor_sockets[i], buffer, sizeof(buffer), 0);
        }
    }
}

void setup_server(int port, SOCKET& server_socket) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Falha ao inicializar Winsock\n";
        exit(1);
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Erro ao criar socket: " << WSAGetLastError() << "\n";
        WSACleanup();
        exit(1);
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Erro no bind: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }

    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        std::cerr << "Erro no listen: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }

    std::cout << "Servidor iniciado na porta " << port << "\n";
}

void accept_connections(SOCKET server_socket) {
    fd_set read_fds;
    char buffer[sizeof(Message)];
    start_time = time(nullptr);

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        for (int i = 0; i < total_clients; ++i) {
            if (client_sockets[i] != INVALID_SOCKET) {
                FD_SET(client_sockets[i], &read_fds);
            }
        }

        timeval timeout{0, 100000}; // 100ms timeout para checar periodic_flag
        int activity = select(0, &read_fds, nullptr, nullptr, &timeout);

        if (activity == SOCKET_ERROR) {
            std::cerr << "Erro no select: " << WSAGetLastError() << "\n";
            break;
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            SOCKET new_socket = accept(server_socket, nullptr, nullptr);
            if (new_socket != INVALID_SOCKET && total_clients < MAX_CLIENTS) {
                client_sockets[total_clients] = new_socket;
                client_ids[total_clients] = -1;  // ainda não identificado
                total_clients++;
                std::cout << "Novo cliente conectado.\n";
            }
        }

        for (int i = 0; i < total_clients; ++i) {
            SOCKET sd = client_sockets[i];
            if (sd != INVALID_SOCKET && FD_ISSET(sd, &read_fds)) {
                int bytes = recv(sd, buffer, sizeof(buffer), 0);
                if (bytes <= 0) {
                    closesocket(sd);
                    client_sockets[i] = INVALID_SOCKET;
                    client_ids[i] = -1;
                    std::cout << "Cliente desconectado.\n";
                } else {
                    Message msg{};
                    msg.deserialize(buffer);

                    if (msg.type == 0) {  // OI
                        int id = msg.orig_uid;
                        bool erro = false;
                        int tipo = 0; // 1=exibidor, 2=enviador
                        if (id >= 1 && id <= 999) tipo = 1;
                        else if (id >= 1001 && id <= 1999) tipo = 2;
                        else erro = true;
                        // Verifica duplicidade e limites
                        if (!erro && tipo == 1) {
                            if (total_exibidores >= MAX_EXIBIDORES) erro = true;
                            for (int k = 0; k < total_exibidores; ++k) {
                                if (exibidor_ids[k] == id) erro = true;
                            }
                            for (int k = 0; k < total_enviadores; ++k) {
                                if (enviador_ids[k] == id + 1000) erro = true;
                            }
                        } else if (!erro && tipo == 2) {
                            if (total_enviadores >= MAX_ENVIADORES) erro = true;
                            for (int k = 0; k < total_enviadores; ++k) {
                                if (enviador_ids[k] == id) erro = true;
                            }
                        }
                        if (erro) {
                            Message errMsg{};
                            errMsg.type = 3; // ERRO
                            errMsg.orig_uid = 0;
                            errMsg.dest_uid = id;
                            strcpy_s(errMsg.text, "ERRO: ID duplicado ou limite atingido");
                            errMsg.text_len = strlen(errMsg.text);
                            char errBuf[sizeof(Message)];
                            errMsg.serialize(errBuf);
                            send(sd, errBuf, sizeof(errBuf), 0);
                            closesocket(sd);
                            client_sockets[i] = INVALID_SOCKET;
                            client_ids[i] = -1;
                            std::cout << "Cliente rejeitado (ID: " << id << ")\n";
                        } else {
                            client_ids[i] = id;
                            send(sd, buffer, sizeof(buffer), 0);
                            if (tipo == 1) {
                                exibidor_sockets[total_exibidores] = sd;
                                exibidor_ids[total_exibidores] = id;
                                total_exibidores++;
                            } else if (tipo == 2) {
                                enviador_sockets[total_enviadores] = sd;
                                enviador_ids[total_enviadores] = id;
                                total_enviadores++;
                            }
                            std::cout << "Cliente " << id << " identificado (tipo " << tipo << ")\n";
                        }
                    } else if (msg.type == 2) {  // MSG
                        // Valida origem
                        if (msg.orig_uid != client_ids[i]) {
                            std::cout << "MSG com orig_uid inválido de cliente " << client_ids[i] << "\n";
                            continue;
                        }
                        if (msg.dest_uid == 0) {
                            // Envia para todos exibidores
                            for (int j = 0; j < total_exibidores; ++j) {
                                if (exibidor_sockets[j] != INVALID_SOCKET) {
                                    send(exibidor_sockets[j], buffer, sizeof(buffer), 0);
                                }
                            }
                        } else {
                            // Envia só para exibidor de dest_uid
                            for (int j = 0; j < total_exibidores; ++j) {
                                if (exibidor_ids[j] == msg.dest_uid && exibidor_sockets[j] != INVALID_SOCKET) {
                                    send(exibidor_sockets[j], buffer, sizeof(buffer), 0);
                                    break;
                                }
                            }
                        }
                    } else if (msg.type == 1) {  // TCHAU
                        // Remove dos arrays de exibidor/enviador
                        int id = client_ids[i];
                        for (int k = 0; k < total_exibidores; ++k) {
                            if (exibidor_ids[k] == id) {
                                exibidor_sockets[k] = INVALID_SOCKET;
                                exibidor_ids[k] = -1;
                            }
                        }
                        for (int k = 0; k < total_enviadores; ++k) {
                            if (enviador_ids[k] == id) {
                                enviador_sockets[k] = INVALID_SOCKET;
                                enviador_ids[k] = -1;
                            }
                        }
                        closesocket(sd);
                        client_sockets[i] = INVALID_SOCKET;
                        client_ids[i] = -1;
                        std::cout << "Cliente " << msg.orig_uid << " saiu.\n";
                    }
                }
            }
        }

        if (periodic_flag) {
            broadcast_info();
            periodic_flag = 0;
        }
    }
}

void cleanup() {
    if (hTimer) {
        DeleteTimerQueueTimer(NULL, hTimer, NULL);
    }
    WSACleanup();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: ./server <porta>\n";
        return 1;
    }

    int port = atoi(argv[1]);
    SOCKET server_socket;

    if (!CreateTimerQueueTimer(&hTimer, NULL, TimerRoutine,
        NULL, 0, 60000, WT_EXECUTEDEFAULT)) {
        std::cerr << "Erro ao criar timer\n";
        return 1;
    }

    setup_server(port, server_socket);
    
    // Registra função de cleanup
    std::atexit(cleanup);

    accept_connections(server_socket);

    // Limpa antes de sair
    cleanup();
    
    return 0;
}

