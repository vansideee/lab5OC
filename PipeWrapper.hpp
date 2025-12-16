#pragma once
#include "common.hpp"
#include <windows.h>
#include <iostream>

class PipeWrapper {
    HANDLE handle_;
    bool is_server_side_;

public:
    PipeWrapper() : handle_(INVALID_HANDLE_VALUE), is_server_side_(false) {}

    ~PipeWrapper() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            if (is_server_side_) DisconnectNamedPipe(handle_);
            CloseHandle(handle_);
        }
    }

    static PipeWrapper CreateServer(const std::string& name) {
        PipeWrapper pipe;
        pipe.handle_ = CreateNamedPipeA(
            name.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            sizeof(Message), sizeof(Message),
            0, NULL
        );
        pipe.is_server_side_ = true;
        return pipe;
    }

    static PipeWrapper ConnectTo(const std::string& name) {
        PipeWrapper pipe;
        while (true) {
            pipe.handle_ = CreateFileA(
                name.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0, NULL, OPEN_EXISTING,
                0, NULL
            );

            if (pipe.handle_ != INVALID_HANDLE_VALUE) break;

            if (GetLastError() == ERROR_PIPE_BUSY) {
                WaitNamedPipeA(name.c_str(), 5000);
            }
            else {
                break;
            }
        }
        pipe.is_server_side_ = false;
        return pipe;
    }

    bool WaitForClient() {
        if (!is_server_side_ || handle_ == INVALID_HANDLE_VALUE) return false;
        bool connected = ConnectNamedPipe(handle_, NULL) ?
            true : (GetLastError() == ERROR_PIPE_CONNECTED);
        return connected;
    }

    template<typename T>
    bool Write(const T& data) {
        if (handle_ == INVALID_HANDLE_VALUE) return false;
        DWORD bytesWritten;
        return WriteFile(handle_, &data, sizeof(T), &bytesWritten, NULL);
    }

    template<typename T>
    bool Read(T& data) {
        if (handle_ == INVALID_HANDLE_VALUE) return false;
        DWORD bytesRead;
        return ReadFile(handle_, &data, sizeof(T), &bytesRead, NULL);
    }

    bool isOpen() const { return handle_ != INVALID_HANDLE_VALUE; }

    void Close() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            if (is_server_side_) DisconnectNamedPipe(handle_);
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }
};