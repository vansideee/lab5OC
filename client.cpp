#include "common.hpp"
#include "PipeWrapper.hpp"
#include <iostream>
#include <windows.h>

std::string getMyPipePath(int pid) {
    return "\\\\.\\pipe\\client_" + std::to_string(pid);
}

Message rpcCall(int pid, Message msg) {
    msg.client_pid = pid;
    std::string my_path = getMyPipePath(pid);
    PipeWrapper listener = PipeWrapper::CreateServer(my_path);

    {
        PipeWrapper sender = PipeWrapper::ConnectTo(SERVER_PIPE);
        if (!sender.isOpen()) {
            std::cout << "Server down!\n";
            exit(1);
        }
        sender.Write(msg);
    }

    Message response;
    if (listener.WaitForClient()) {
        listener.Read(response);
    }
    return response;
}

void sendNotify(int pid, Message msg) {
    msg.client_pid = pid;
    PipeWrapper sender = PipeWrapper::ConnectTo(SERVER_PIPE);
    if (sender.isOpen()) sender.Write(msg);
}

int main() {
    int pid = GetCurrentProcessId();
    std::cout << "Client started. PID: " << pid << "\n";

    Message req;
    req.type = OpType::CONNECT;
    sendNotify(pid, req);

    while (true) {
        std::cout << "\n[PID " << pid << "] 1.Modify 2.Read 3.Exit > ";
        int ch;
        std::cin >> ch;
        if (ch == 3) break;

        std::cout << "ID: ";
        int id; std::cin >> id;

        req.record_id = id;

        if (ch == 1) {
            req.type = OpType::MODIFY_REQ;
            Message resp = rpcCall(pid, req);

            if (resp.success) {
                std::cout << "Current: " << resp.data.name << ", " << resp.data.hours << "\n";
                std::cout << "New Hours: "; std::cin >> resp.data.hours;

                resp.type = OpType::MODIFY_SUBMIT;
                sendNotify(pid, resp);
            }
            else {
                std::cout << "Error: " << resp.error_msg << "\n";
            }
        }
        else if (ch == 2) {
            req.type = OpType::READ_REQ;
            Message resp = rpcCall(pid, req);

            if (resp.success) {
                std::cout << "DATA: " << resp.data.num << " " << resp.data.name << "\n";
                std::cout << "Reading... Press Enter.";
                std::cin.ignore(); std::cin.get();

                req.type = OpType::READ_DONE;
                sendNotify(pid, req);
            }
            else {
                std::cout << "Error: " << resp.error_msg << "\n";
            }
        }
    }

    req.type = OpType::DISCONNECT;
    sendNotify(pid, req);
    return 0;
}