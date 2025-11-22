#pragma once
#include <string>
#include <cstring>

struct Employee {
    int num;
    char name[10];
    double hours;
};

enum class OpType {
    READ_REQ,
    MODIFY_REQ,
    READ_DONE,
    MODIFY_SUBMIT,
    CONNECT,
    DISCONNECT
};

struct Message {
    int client_pid;
    OpType type;
    int record_id;
    Employee data;
    bool success;
    char error_msg[64];

    static Message Error(int pid, const std::string& text) {
        Message m;
        m.client_pid = pid;
        m.success = false;
        std::strncpy(m.error_msg, text.c_str(), 63);
        return m;
    }
};

const std::string SERVER_PIPE = "\\\\.\\pipe\\lab5_server_pipe";
const std::string DATA_FILE = "data.bin";