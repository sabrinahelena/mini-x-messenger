﻿// client_receiver.cpp
// Cliente receptor do mini-X Messenger
// Conecta ao servidor, se identifica e exibe mensagens recebidas (privadas ou broadcast).

#include "../include/message.hpp"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <cstring>

// Função principal: conecta ao servidor e recebe mensagens
void connect_and_receive(const char* ip, int port, int my_id) {
    // Inicializa a biblioteca Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Falha ao inicializar Winsock\n";
        return;
    }

    // Cria o socket TCP
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Erro ao criar socket: " << WSAGetLastError() << "\n";
        WSACleanup();
        return;
    }

    // Preenche a estrutura de endereço do servidor
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Tenta conectar ao servidor
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Erro ao conectar: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return;
    }

    // Prepara e envia mensagem de identificação (OI)
    Message msg{};
    msg.type = 0;  // OI
    msg.orig_uid = my_id;
    msg.dest_uid = 0;
    msg.text_len = 0;
    memset(msg.text, 0, 141);

    char buffer[sizeof(Message)];
    msg.serialize(buffer);
    send(sock, buffer, sizeof(buffer), 0);

    // Aguarda resposta do servidor (OI ou ERRO)
    recv(sock, buffer, sizeof(buffer), 0);
    msg.deserialize(buffer);

    if (msg.type == 3) {
        // Recebeu mensagem de erro do servidor
        std::cerr << "Erro do servidor: " << msg.text << "\n";
        closesocket(sock);
        WSACleanup();
        return;
    }
    if (msg.type != 0) {
        // Não recebeu confirmação de identificação
        std::cerr << "Erro: não recebeu OI do servidor.\n";
        closesocket(sock);
        WSACleanup();
        return;
    }

    std::cout << "Conectado ao servidor! Aguardando mensagens...\n";

    // Loop principal: recebe e exibe mensagens do servidor
    while (true) {
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            std::cerr << "Conexão encerrada.\n";
            break;
        }
        msg.deserialize(buffer);

        if (msg.type == 2) { // Mensagem normal
            std::string priv = (msg.dest_uid == my_id && msg.dest_uid != 0) ? " (privada)" : "";
            std::cout << "Mensagem de " << msg.orig_uid << priv << ": " << msg.text << "\n";
        }
    }

    closesocket(sock);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: ./client_receiver <id> <ip> <porta>\n";
        return 1;
    }

    int my_id = atoi(argv[1]);
    const char* ip = argv[2];
    int port = atoi(argv[3]);

    connect_and_receive(ip, port, my_id);

    return 0;
}