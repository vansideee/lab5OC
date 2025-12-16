#include "common.hpp"
#include "LockManager.hpp"
#include "FileRepo.hpp"
#include "PipeWrapper.hpp"
#include <list>
#include <vector>
#include <iostream>
#include <windows.h> 

struct QueuedReq {
    Message msg;
};

void sendToClient(int pid, Message& msg) {
    std::string client_pipe_name = "\\\\.\\pipe\\client_" + std::to_string(pid);
    PipeWrapper pipe = PipeWrapper::ConnectTo(client_pipe_name);
    if (pipe.isOpen()) {
        pipe.Write(msg);
    }
}

void spawnClient() {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char cmd[] = "client.exe";

    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE,
        NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        std::cerr << "Failed to launch client! Error: " << GetLastError() << "\n";
    }
}

int main() {
    FileRepository db(DATA_FILE);
    std::vector<Employee> seed = {
        {10, "Ivanov", 40.5}, {20, "Petrov", 30.0}, {30, "Sidorov", 50.0}
    };
    db.seedData(seed);
    db.printAll();

    LockManager locks;
    std::list<QueuedReq> queue;

    std::cout << "Enter number of clients to spawn: ";
    int num_clients;
    std::cin >> num_clients;

    for (int i = 0; i < num_clients; i++) {
        spawnClient();
    }

    std::cout << "Server listening on " << SERVER_PIPE << "...\n";

    int active_clients = num_clients;

    while (active_clients > 0) {
        PipeWrapper server = PipeWrapper::CreateServer(SERVER_PIPE);

        if (server.WaitForClient()) {
            Message msg;
            if (server.Read(msg)) {
                int id = msg.record_id;

                if (msg.type == OpType::DISCONNECT) {
                    active_clients--;
                    std::cout << "[-] Client " << msg.client_pid << " disconnected.\n";
                }
                else if (msg.type == OpType::CONNECT) {
                }
                else if (msg.type == OpType::READ_REQ) {
                    if (locks.tryRead(id)) {
                        if (db.get(id, msg.data)) {
                            msg.success = true;
                            std::cout << "DEBUG: Client " << msg.client_pid << " READING " << id << "\n";
                        }
                        else {
                            msg = Message::Error(msg.client_pid, "ID not found");
                            locks.finishRead(id);
                        }
                        sendToClient(msg.client_pid, msg);
                    }
                    else {
                        std::cout << "WAIT: Client " << msg.client_pid << " queued for READ " << id << "\n";
                        queue.push_back({ msg });
                    }
                }
                else if (msg.type == OpType::MODIFY_REQ) {
                    if (locks.tryWrite(id)) {
                        if (db.get(id, msg.data)) {
                            msg.success = true;
                            std::cout << "DEBUG: Client " << msg.client_pid << " MODIFYING " << id << "\n";
                        }
                        else {
                            msg = Message::Error(msg.client_pid, "ID not found");
                            locks.finishWrite(id);
                        }
                        sendToClient(msg.client_pid, msg);
                    }
                    else {
                        std::cout << "WAIT: Client " << msg.client_pid << " queued for MODIFY " << id << "\n";
                        queue.push_back({ msg });
                    }
                }
                else if (msg.type == OpType::READ_DONE) {
                    locks.finishRead(id);
                    std::cout << "DEBUG: Client " << msg.client_pid << " done reading " << id << "\n";
                }
                else if (msg.type == OpType::MODIFY_SUBMIT) {
                    db.update(msg.data);
                    locks.finishWrite(id);
                    std::cout << "DEBUG: Client " << msg.client_pid << " done modifying " << id << "\n";
                }

                if (msg.type == OpType::READ_DONE || msg.type == OpType::MODIFY_SUBMIT) {
                    auto it = queue.begin();
                    while (it != queue.end()) {
                        Message& qm = it->msg;
                        bool can_exec = (qm.type == OpType::READ_REQ)
                            ? locks.tryRead(qm.record_id)
                            : locks.tryWrite(qm.record_id);
                        if (can_exec) {
                            if (db.get(qm.record_id, qm.data)) qm.success = true;
                            else qm = Message::Error(qm.client_pid, "ID lost");

                            std::cout << "RESUME: Client " << qm.client_pid << " extracted\n";
                            sendToClient(qm.client_pid, qm);
                            it = queue.erase(it);
                        }
                        else {
                            ++it;
                        }
                    }
                }
            }
        }
        server.Close();
    }

    db.printAll();
    std::cout << "Server finished. Press Enter.\n";
    std::cin.ignore(); std::cin.get();
    return 0;
}