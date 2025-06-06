﻿// client_sender.cpp
// Cliente emissor do mini-X Messenger
// Conecta ao servidor, se identifica e permite enviar mensagens para receptores específicos ou para todos (broadcast).

#include "../include/message.hpp"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <cstring>

// Função principal de conexão e envio de mensagens
void connect_to_server(const char* ip, int port, int my_id) {
    // Inicializa a biblioteca Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Falha ao inicializar Winsock\n";
        return;
    }

    // Cria o socket TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket error");
        return;
    }

    // Preenche a estrutura de endereço do servidor
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Tenta conectar ao servidor
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect error");
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
        return;
    }
    if (msg.type != 0) {
        // Não recebeu confirmação de identificação
        std::cerr << "Erro: não recebeu OI do servidor.\n";
        closesocket(sock);
        return;
    }

    std::cout << "Conectado ao servidor!\n";

    // Loop principal para envio de mensagens
    while (true) {
        std::cout << "Destino (0 para todos, ou ID): ";
        std::cin >> msg.dest_uid;
        std::cin.ignore();
        std::cout << "Mensagem: ";
        std::cin.getline(msg.text, 141);

        msg.type = 2;  // MSG
        msg.orig_uid = my_id;
        msg.text_len = strlen(msg.text);

        msg.serialize(buffer);
        send(sock, buffer, sizeof(buffer), 0);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: ./client_sender <id> <ip> <porta>\n";
        return 1;
    }

    int my_id = atoi(argv[1]);
    const char* ip = argv[2];
    int port = atoi(argv[3]);

    connect_to_server(ip, port, my_id);

    WSACleanup();
    return 0;
}